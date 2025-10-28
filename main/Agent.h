#pragma once

#include <string>
#include <list>
#include <map>
#include <functional>

#include <system/Log.h>
#include <system/network/WifiManager.h>
#include <system/PWMDriver.h>

#include "LightGroupService.h"

namespace cima {
    class Agent {
        static const system::Log LOGGER;

        std::list<std::function<void()>> mainLoopFunctions;

        bool keepRunning = true;

        uint32_t lastRfEventTime;

        public:

            static std::string FLASH_FILESYSTEM_MOUNT_PATH;

            void welcome(std::string &visitorName);
            void cat(const std::string &filename);

            void initFlashStorage();
            bool mountFlashFileSystem();
            void setupNetwork(system::network::WifiManager &wifiManager);
            std::list<system::network::WifiCredentials> readWifiCredentials();

            void handleRfButton(
                std::function<void()> toggleCallback, 
                std::function<void()> upCallback, 
                std::function<void()> downCallback, 
                int protocol, long command
            );

            void mainLoop();
            void registerToMainLoop(std::function<void()> function);
            void stop();
    };

    class StatusLight {
        unsigned long long untilTicks = 0;
        unsigned int value = 0;

        cima::system::PWMDriver &light;

    public:
        StatusLight(cima::system::PWMDriver &light);
        virtual ~StatusLight() = default;

        void refresh();
        void setValueForMs(unsigned int value, unsigned long long durationMillis);
    };
}