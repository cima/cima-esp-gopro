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

#include <system/network/WifiManager.h>

#include <system/ExecutionLimiter.h>

#include <system/ButtonController.h>
#include <system/PWMDriver.h>

#include <gopro/GoProClient.h>
#include <system/network/Rf433Controller.h>

cima::system::Log logger("main");

cima::system::network::WifiManager wifiManager;

cima::Agent agent;

cima::system::ButtonController buttonController(GPIO_NUM_0);

const gpio_num_t WARM_LIGHT_MOSFET_DRIVER_GPIO = GPIO_NUM_26;
cima::system::PWMDriver warmLightMosfetDriver(WARM_LIGHT_MOSFET_DRIVER_GPIO, LEDC_CHANNEL_0, true);

const gpio_num_t COLD_LIGHT_MOSFET_DRIVER_GPIO = GPIO_NUM_27;
cima::system::PWMDriver coldLightMosfetDriver(COLD_LIGHT_MOSFET_DRIVER_GPIO, LEDC_CHANNEL_1, true);

gopro::GoProClient goProClient;
cima::system::network::Rf433Controller rf433Controller(GPIO_NUM_13);

::cima::system::ExecutionLimiter limiter(std::chrono::seconds(1));

extern "C" void app_main(void) { 

    logger.init();
    logger.info("ESP32 Device");
    logger.info("Initializing...");

    agent.initFlashStorage();

    if(agent.mountFlashFileSystem()){
        agent.cat("/spiffs/sheep.txt");
    }

    agent.setupNetwork(wifiManager);

    wifiManager.registerNetworkUpHandler([&](){goProClient.setNetworkUp();});
    wifiManager.registerNetworkDownHandler([&](){goProClient.setNetworkDown();});

    wifiManager.start();

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

    agent.registerToMainLoop(std::bind(&cima::system::network::Rf433Controller::handleData, &rf433Controller));

    agent.registerToMainLoop([&](){ 
        if( ! limiter.canExecute()) {
            return;
        }
        goProClient.requestStatus();
        goProClient.stopExpiredRecording();
    });

    logger.info(" > Main loop");
    agent.mainLoop();
}