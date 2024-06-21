#pragma once

#include <esp_http_client.h>
#include <mutex>
#include <atomic>

#include <system/Log.h>

namespace gopro {
    class GoProClient {
    public:
        static const int MAX_HTTP_OUTPUT_BUFFER = 2048;
    private:

        static cima::system::Log LOG;

        // Buffer to store response of http request from event handler
        char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};

        // Stores number of bytes read for entire request
        int local_response_len = 0;

        // Stores number of bytes read during event handling
        int output_len = 0;

        esp_http_client_handle_t *httpClient;

        static esp_http_client_config_t config;

        std::mutex clientMutex;

        std::atomic<bool> networkUp{false};

        std::atomic<uint32_t> stopAfterTime{0};

    public:

        bool connect();

        bool requestStatus();

        void setNetworkUp() { 
            networkUp.store(true);
        };
        void setNetworkDown() { 
            networkUp.store(false);
        };
        bool isNetworkUp() {
            return networkUp.load();
        }

        void toggleShortRecording();
        void startRecording();
        void stopRecording();

        void stopExpiredRecording();

    private:
        esp_err_t receiveClientEvent(esp_http_client_event_t *evt);

        static esp_err_t receive_wrapper(esp_http_client_event_t *evt) {
            return ((GoProClient *)evt->user_data)->receiveClientEvent(evt);
        }
    };
}