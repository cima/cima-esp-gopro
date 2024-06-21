#pragma once

#include <string>
#include <list>
#include <map>
#include <functional>

#include <system/Log.h>
#include <system/network/WifiManager.h>

#include "LightGroupService.h"

namespace cima {
    class Agent {
        static const system::Log LOGGER;

        std::list<std::function<void()>> mainLoopFunctions;

        bool keepRunning = true;

        public:

            static std::string FLASH_FILESYSTEM_MOUNT_PATH;

            void welcome(std::string &visitorName);
            void cat(const std::string &filename);

            void initFlashStorage();
            bool mountFlashFileSystem();
            void setupNetwork(system::network::WifiManager &wifiManager);
            std::list<system::network::WifiCredentials> readWifiCredentials();

            void mainLoop();
            void registerToMainLoop(std::function<void()> function);
            void stop();
    };
}