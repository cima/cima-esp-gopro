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

cima::system::Log logger("main");

cima::system::network::WifiManager wifiManager;

cima::Agent agent;

cima::system::ButtonController buttonController(GPIO_NUM_0);

const gpio_num_t WARM_LIGHT_MOSFET_DRIVER_GPIO = GPIO_NUM_26;
cima::system::PWMDriver warmLightMosfetDriver(WARM_LIGHT_MOSFET_DRIVER_GPIO, LEDC_CHANNEL_0, true);

const gpio_num_t COLD_LIGHT_MOSFET_DRIVER_GPIO = GPIO_NUM_27;
cima::system::PWMDriver coldLightMosfetDriver(COLD_LIGHT_MOSFET_DRIVER_GPIO, LEDC_CHANNEL_1, true);

gopro::GoProClient goProClient;


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

    // TODO try connect in a loop as gopro is wierd with when WIFI is acecsible
    wifiManager.start();

    goProClient.connect();

    agent.registerToMainLoop([&](){ 
        if( ! limiter.canExecute()) {
            return;
        }
        logger.info("Whatever"); 
        goProClient.requestStatus();
        
        });

    logger.info(" > Main loop");
    agent.mainLoop();
}