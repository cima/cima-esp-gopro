#include "Agent.h"

#include <thread>
#include <chrono>
#include <ctime>  
#include <string>
#include <cstring>
#include <utility>
#include <memory>
#include <fstream>

#include <stdio.h>
#include <cJSON.h>

#include <esp_spiffs.h>
#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>

namespace cima {

    const system::Log Agent::LOGGER("Agent");

    std::string Agent::FLASH_FILESYSTEM_MOUNT_PATH = "/spiffs";

    void Agent::cat(const std::string &filename){
        FILE* f = fopen(filename.c_str(), "r");
        if (f == NULL) {
            LOGGER.error("Failed to open %s", filename.c_str());
            return;
        }

        char buf[512];
        memset(buf, 0, sizeof(buf));
        auto readSize = fread(buf, 1, sizeof(buf), f);
        fclose(f);

        LOGGER.info("Read from %s:\n%.*s", filename.c_str(), readSize, buf);
    }

    void Agent::initFlashStorage(){
        LOGGER.info("Initializing flash storage...");

        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);
    }

    bool Agent::mountFlashFileSystem(){
        esp_vfs_spiffs_conf_t conf = {
            .base_path = FLASH_FILESYSTEM_MOUNT_PATH.c_str(),
            .partition_label = NULL,
            .max_files = 5,
            .format_if_mount_failed = false
        };

        esp_err_t ret = esp_vfs_spiffs_register(&conf);

        if (ret != ESP_OK) {
            if (ret == ESP_FAIL) {
                LOGGER.error("Failed to mount or format filesystem");
            } else if (ret == ESP_ERR_NOT_FOUND) {
                LOGGER.error("Failed to find SPIFFS partition");
            } else {
                LOGGER.error("Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
            }
            return false;
        }

        size_t total = 0, used = 0;
        ret = esp_spiffs_info(NULL, &total, &used);
        if (ret != ESP_OK) {
            LOGGER.error("Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        } else {
            LOGGER.error("Partition size: total: %d, used: %d", total, used);
        }

        return true;
    }

    void Agent::setupNetwork(system::network::WifiManager &wifiManager){
        LOGGER.info(" > WiFi");
        auto credentials = std::move(readWifiCredentials());

        for(auto network : credentials) {
            LOGGER.info("Adding network: %s", network.getSsid().c_str());
            wifiManager.addNetwork(network);
        }
    }

    std::list<system::network::WifiCredentials> Agent::readWifiCredentials(){
        std::ifstream in(FLASH_FILESYSTEM_MOUNT_PATH + "/connectivity/wifi.json");
        std::string wifiJson((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

        std::list<system::network::WifiCredentials> credentials;

        cJSON *root = cJSON_Parse(wifiJson.c_str());
        cJSON *networks = cJSON_GetObjectItem(root, "Networks");
        for (int i = 0 ; i < cJSON_GetArraySize(networks) ; i++){

            cJSON *network = cJSON_GetArrayItem(networks, i);
            auto ssid = cJSON_GetObjectItem(network, "SSID")->valuestring;
            auto passphrase = cJSON_GetObjectItem(network, "password")->valuestring;

            credentials.push_back(system::network::WifiCredentials(std::string(ssid), std::string(passphrase)));
        }

        return credentials;
    }

    void Agent::handleRfButton(
        std::function<void()> toggleCallback, 
        std::function<void()> upCallback, 
        std::function<void()> downCallback, 
        int protocol, long command
    ){
        if (protocol != 1){
            return;
        }

        LOGGER.info("Handling (protocol: %d): %d", protocol, command);

        switch (command) {
            case 260129:
            {
                uint32_t now = esp_log_timestamp();
                if(now - lastRfEventTime < 500){
                    LOGGER.info("Skipping (protocol: %d): %d", protocol, command);    
                    break;
                }
                lastRfEventTime = now;
                toggleCallback();
            }
                break;

            case 260132:
                upCallback();
                break;

            case 260130:
                downCallback();
                break;
        }
    }

    void Agent::mainLoop(){
        while(keepRunning){
            for(auto function : mainLoopFunctions){
                function();
            }

            //TODO safety slowdown to avoid starvation - to be removed once finalized whole concept
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void Agent::registerToMainLoop(std::function<void()> function){
        mainLoopFunctions.push_back(function);
    }

    void Agent::stop(){
        keepRunning = false;
    }
}