#include "sdkconfig.h"
#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <stdarg.h>

#include <esp_log.h>


#include <driver/gpio.h>
#include <driver/ledc.h>


#include <cJSON.h>

#include "Agent.h"

#include <system/Log.h>
#include <system/InterruptController.h>

#include <system/network/WifiManager.h>

#include <system/ExecutionLimiter.h>

#include <system/ButtonController.h>
#include <system/PWMDriver.h>

#include <gopro/GoProClient.h>
#include <system/network/Rf433Controller.h>
#include <nrf24/NRF24L01Controller.h>

cima::system::Log logger("main");

cima::system::InterruptController interruptController;

cima::system::network::WifiManager wifiManager;

cima::Agent agent;

cima::system::ButtonController buttonController(GPIO_NUM_0);
cima::system::ButtonController startButtonController(GPIO_NUM_2);
cima::system::ButtonController stopButtonController(GPIO_NUM_15);

cima::system::ButtonController wifiSwitch(GPIO_NUM_21);

cima::system::PWMDriver redLightDriver(GPIO_NUM_12, LEDC_CHANNEL_0, false);
cima::system::PWMDriver greenLightDriver(GPIO_NUM_14, LEDC_CHANNEL_1, false);
cima::system::PWMDriver blueLightDriver(GPIO_NUM_27, LEDC_CHANNEL_2, false);

cima::StatusLight redStatusLight(redLightDriver);
cima::StatusLight greenStatusLight(greenLightDriver);
cima::StatusLight blueStatusLight(blueLightDriver);

gopro::GoProClient goProClient;
cima::system::network::Rf433Controller rf433Controller(GPIO_NUM_13);
nrf24::NRF24L01Controller nRF24L01Controller;

::cima::system::ExecutionLimiter limiter(std::chrono::seconds(1));
::cima::system::ExecutionLimiter fastLimiter(std::chrono::milliseconds(10));

const std::string GO_PRO_MESAGE_REC_SHORT = "GP:Toggle_short";
const std::string GO_PRO_MESAGE_REC_START = "GP:REC:start";
const std::string GO_PRO_MESAGE_REC_STOP = "GP:REC:stop";
const std::string GO_PRO_MESAGE_STATUS_OK = "GP:status:OK";
const std::string GO_PRO_MESAGE_STATUS_RECORDING = "GP:status:REC";
const std::string GO_PRO_MESAGE_STATUS_FAIL = "GP:status:FAIL";

extern "C" void app_main(void) { 

    logger.init();
    logger.info("ESP32 Device");
    logger.info("Initializing...");

    agent.initFlashStorage();

    if(agent.mountFlashFileSystem()){
        agent.cat("/spiffs/sheep.txt");
    }

    interruptController.enableInterrupts();
    interruptController.createDefaultEventLoop();

    agent.setupNetwork(wifiManager);

    wifiManager.registerNetworkUpHandler([&](){goProClient.setNetworkUp();});
    wifiManager.registerNetworkDownHandler([&](){goProClient.setNetworkDown();});

    wifiSwitch.initButton();

    //TODO runtime based truring wifi on/off is difficult -> postponing
    /*
    wifiSwitch.addUpHandler([&](){
        logger.info("WiFi switch is UP - WiFi will be started.");
        if (!wifiManager.isStarted()){
            logger.info("WiFi manager started - ignoring click.");
        } else {
            wifiManager.start();
        }
    });
    wifiSwitch.addDownHandler([&](){
        logger.info("WiFi switch is DOWN - WiFi is stopping.");
        wifiManager.stop();
    });
    */

    if(wifiSwitch.isButtonUp()){
        logger.info("WiFi switch is UP - WiFi will be started.");
        wifiManager.start();
    } else {
        logger.info("WiFi switch is DOWN - WiFi will NOT be started.");
    }

    //redLightDriver.update(8192);
    //greenLightDriver.update(2500);
    //blueLightDriver.update(0);
    blueStatusLight.setValueForMs(8191, 2000);

    goProClient.connect();

    rf433Controller.initRf433();
    rf433Controller.addReceiveHandler([&](int protocol, long value){
        agent.handleRfButton(
            [&](){goProClient.toggleShortRecording();},
            [&](){goProClient.startRecording();},
            [&](){goProClient.stopRecording();},
            protocol, value
        );
    });

    nRF24L01Controller.init(nrf24::NRF24L01Controller::DISABLE_NRF24_AUTO_ACK);

    buttonController.initButton();
    buttonController.addHandler([&](){
        goProClient.toggleShortRecording();

        uint8_t message[32];
        strcpy((char *)message, GO_PRO_MESAGE_REC_SHORT.c_str());
        nRF24L01Controller.broadcastMessage(message);
    });

    startButtonController.initButton();
    startButtonController.addHandler([&](){
        goProClient.startRecording();

        uint8_t message[32];
        strcpy((char *)message, GO_PRO_MESAGE_REC_START.c_str());
        nRF24L01Controller.broadcastMessage(message);
    });

    stopButtonController.initButton();
    stopButtonController.addHandler([&](){
        goProClient.stopRecording();

        uint8_t message[32];
        strcpy((char *)message, GO_PRO_MESAGE_REC_STOP.c_str());
        nRF24L01Controller.broadcastMessage(message);
    });



    agent.registerToMainLoop(std::bind(&cima::system::network::Rf433Controller::handleData, &rf433Controller));
    agent.registerToMainLoop(std::bind(&cima::system::ButtonController::handleClicks, &buttonController));
    agent.registerToMainLoop(std::bind(&cima::system::ButtonController::handleClicks, &startButtonController));
    agent.registerToMainLoop(std::bind(&cima::system::ButtonController::handleClicks, &stopButtonController));
    agent.registerToMainLoop(std::bind(&cima::system::ButtonController::handleClicks, &wifiSwitch));
    
    // Status reporting
    agent.registerToMainLoop([&](){ 
        if( ! limiter.canExecute()) {
            return;
        }

        // Report wifi related status only when wifi is enabled
        if(wifiSwitch.isButtonUp()){

            //Send status over NRF24L01
            uint8_t message[32];
            //FIXME this is blocking and congests main loop -> separate thread
            gopro::GoProStatus status = goProClient.requestStatus();
            logger.debug("Status.overall: %s", status.overall ? "true": "false");
            logger.debug("Status.recordingStatus: %d", status.recordingStatus);
            logger.debug("Status.recordingSuration: %d s", status.recordingDuration );
            if (status.overall) {
                if(status.recordingStatus == 1){
                    //blink red
                    redStatusLight.setValueForMs(8191, 500);
                    //broadcast recording message
                    strcpy((char *)message, GO_PRO_MESAGE_STATUS_RECORDING.c_str());
                } else {
                    //blink blue (go pro blinks blue) - locally
                    blueStatusLight.setValueForMs(8191, 500);
                    //broadcast linked message
                    strcpy((char *)message, GO_PRO_MESAGE_STATUS_OK.c_str());
                }
            } else {
                strcpy((char *)message, GO_PRO_MESAGE_STATUS_FAIL.c_str());
            }


            nRF24L01Controller.broadcastMessage(message);

            //Even though it is blocking it worth to stop expired recording
            goProClient.stopExpiredRecording();
        } 
    });

    agent.registerToMainLoop([&](){ 
        if( ! fastLimiter.canExecute()) {
            return;
        }
        uint8_t buf[32];
        if(nRF24L01Controller.receiveMessage(buf)){
            logger.info("Message received: %s", buf);
            if (GO_PRO_MESAGE_REC_SHORT.compare((const char *)buf) == 0){
                goProClient.toggleShortRecording();
            } 
            else if (GO_PRO_MESAGE_REC_START.compare((const char *)buf) == 0){
                goProClient.startRecording();
            }
            else if (GO_PRO_MESAGE_REC_STOP.compare((const char *)buf) == 0){
                goProClient.stopRecording();
            }
            else if (GO_PRO_MESAGE_STATUS_FAIL.compare((const char *)buf) == 0){
                //blink short green - locally
                greenStatusLight.setValueForMs(8191, 100);
            }
            else if (GO_PRO_MESAGE_STATUS_OK.compare((const char *)buf) == 0){
                //blink blue (go pro blibks blue) - locally
                blueStatusLight.setValueForMs(8191, 500);
            }
            else if (GO_PRO_MESAGE_STATUS_RECORDING.compare((const char *)buf) == 0){
                //blink red - locally
                redStatusLight.setValueForMs(8191, 500);
            }
        }

        redStatusLight.refresh();  
        greenStatusLight.refresh();
        blueStatusLight.refresh();
    });

    logger.info(" > Main loop");
    agent.mainLoop();
}