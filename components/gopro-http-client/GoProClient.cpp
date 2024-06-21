#include <gopro/GoProClient.h>
#include <functional>

#include <esp_tls.h>
#include <esp_log.h>

namespace gopro {

    static const char *TAG = "GoProCLient"; //TODO inline once my log is used
    static const uint32_t INACTIVE_STOP_TIME = 0xffffffff;
    static const uint32_t SHORT_RECORDING_SECONDS = 40;

    cima::system::Log GoProClient::LOG(TAG);

    esp_http_client_config_t GoProClient::config = {
        .host = "10.5.5.9",
        .transport_type = HTTP_TRANSPORT_OVER_TCP,
    };

    bool GoProClient::connect() {
        config.event_handler = GoProClient::receive_wrapper;
        return true;
    }

    bool GoProClient::requestStatus() {
        std::unique_lock<std::mutex> lock(clientMutex); 

        if( ! isNetworkUp()) {
            LOG.debug("Network is down - exiting status request");
            return false;
        }

        config.path = "/bacpac/se";
        config.query = "t=gizmolikespizza";//FIXME password
        config.user_data = this;

        //TODO tohle asi do connectu
        esp_http_client_handle_t client = esp_http_client_init(&config);

        local_response_len = 0;
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",// PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
        } else {
            ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        }
        ESP_LOGD(TAG, "Output len after return %d", local_response_len);
        ESP_LOG_BUFFER_HEX(TAG, local_response_buffer, local_response_len);

        esp_http_client_cleanup(client);

        return true;
    }

    void GoProClient::toggleShortRecording(){
        LOG.info("Requesting Starting/increasing a short recording session");

        uint32_t now = esp_log_timestamp();
        uint32_t dueTimestamp = stopAfterTime.load();
        
        stopAfterTime.store(
            (dueTimestamp == INACTIVE_STOP_TIME ? now : dueTimestamp) 
            + SHORT_RECORDING_SECONDS * 1000
        );

        startRecording();
    }

    void GoProClient::startRecording(){
        std::unique_lock<std::mutex> lock(clientMutex); 

        if( ! isNetworkUp()) {
            LOG.debug("Network is down - not starting recording");
            return;
        }
        LOG.info("Starting recording");

        config.path = "/bacpac/SH";
        config.query = "t=gizmolikespizza&p=%01"; //FIXME password
        config.user_data = this;

        //TODO tohle asi do connectu
        esp_http_client_handle_t client = esp_http_client_init(&config);

        local_response_len = 0;
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            LOG.info(TAG, "HTTP GET Status = %d, content_length = %d",// PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
        } else {
            LOG.error(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        }
        ESP_LOGD(TAG, "Output len after return %d", local_response_len);
        ESP_LOG_BUFFER_HEX(TAG, local_response_buffer, local_response_len);

        esp_http_client_cleanup(client);

    }

    void GoProClient::stopRecording(){
        std::unique_lock<std::mutex> lock(clientMutex); 

        if( ! isNetworkUp()) {
            LOG.debug("Network is down - not stopping recording");
            return;
        }
        LOG.info("Stopping recording");

        config.path = "/bacpac/SH";
        config.query = "t=gizmolikespizza&p=%00"; //FIXME password
        config.user_data = this;

        //TODO tohle asi do connectu
        esp_http_client_handle_t client = esp_http_client_init(&config);

        local_response_len = 0;
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            LOG.info(TAG, "HTTP GET Status = %d, content_length = %d",// PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
            stopAfterTime.store(0xffffffff);
        } else {
            LOG.error(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        }
        ESP_LOGD(TAG, "Output len after return %d", local_response_len);
        ESP_LOG_BUFFER_HEX(TAG, local_response_buffer, local_response_len);

        esp_http_client_cleanup(client);
    }

    void GoProClient::stopExpiredRecording() {
        uint32_t now = esp_log_timestamp();
        if (now > stopAfterTime.load()){
            stopRecording();
        }

    }

    /**
     * @brief Handle HTTP serponse and write data into this object internal structures.
     * Mostly stolen from https://github.com/espressif/esp-idf/blob/v5.2.2/examples/protocols/esp_http_client/main/esp_http_client_example.c
     * 
     * @param evt 
     * @return esp_err_t 
     */
    esp_err_t GoProClient::receiveClientEvent(esp_http_client_event_t *evt) {
        static char *output_buffer;  // Buffer to store response of http request from event handler
               
        switch(evt->event_id) {
            case HTTP_EVENT_ERROR:
                ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
                break;
            case HTTP_EVENT_ON_CONNECTED:
                ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
                break;
            case HTTP_EVENT_HEADER_SENT:
                ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
                break;
            case HTTP_EVENT_ON_HEADER:
                ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
                break;
            case HTTP_EVENT_ON_DATA:
                ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
                // Clean the buffer in case of a new request
                if (output_len == 0) {
                    // we are just starting to copy the output data into the use
                    memset(local_response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
                }
                /*
                *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
                *  However, event handler can also be used in case chunked encoding is used.
                */
                if (!esp_http_client_is_chunked_response(evt->client)) {
                    // If user_data buffer is configured, copy the response into the buffer
                    int copy_len = 0;
                    if (local_response_buffer) {
                        // The last byte in local_response_buffer is kept for the NULL character in case of out-of-bound access.
                        copy_len = std::min(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                        if (copy_len) {
                            memcpy(local_response_buffer + output_len, evt->data, copy_len);
                        }
                    } else {
                        int content_len = esp_http_client_get_content_length(evt->client);
                        if (output_buffer == NULL) {
                            // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                            output_buffer = (char *) calloc(content_len + 1, sizeof(char));
                            output_len = 0;
                            if (output_buffer == NULL) {
                                ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                                return ESP_FAIL;
                            }
                        }
                        copy_len = std::min(evt->data_len, (content_len - output_len));
                        if (copy_len) {
                            memcpy(output_buffer + output_len, evt->data, copy_len);
                        }
                    }
                    output_len += copy_len;
                }

                break;
            case HTTP_EVENT_ON_FINISH:
                ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
                if (output_buffer != NULL) {
                    // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                    // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                    free(output_buffer);
                    output_buffer = NULL;
                }
                ESP_LOGD(TAG, "Output len at finish %d", output_len);
                local_response_len = output_len;
                output_len = 0;
                break;
            case HTTP_EVENT_DISCONNECTED:
                ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
                int mbedtls_err = 0;
                esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
                if (err != 0) {
                    ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                    ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
                }
                if (output_buffer != NULL) {
                    free(output_buffer);
                    output_buffer = NULL;
                }
                output_len = 0;
                break;
            /* // I believe GO Pro's API doesn't redirect
            case HTTP_EVENT_REDIRECT:
                ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
                esp_http_client_set_header(evt->client, "From", "user@example.com");
                esp_http_client_set_header(evt->client, "Accept", "text/html");
                esp_http_client_set_redirection(evt->client);
                break;
                */
        }
        return ESP_OK;
    }
}