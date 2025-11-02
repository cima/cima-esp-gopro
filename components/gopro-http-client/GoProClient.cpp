#include <gopro/GoProClient.h>
#include <system/Log.h>
#include <functional>
#include <memory>
#include <cJSON.h>

#include <esp_tls.h>
#include <esp_log.h> //To obtain timestampe for measuring timeouts

namespace gopro {

    static const char *TAG = "GoProCLient"; //TODO inline once my log is used
    static const uint32_t INACTIVE_STOP_TIME = 0xffffffff;
    static const uint32_t SHORT_RECORDING_SECONDS = 40;

    static const char *RECORDING_STATUS_KEY = "8";
    static const char *RECORDING_DURATION_SECONDS_KEY = "13";

    cima::system::Log GoProClient::LOG(TAG);

    esp_http_client_config_t GoProClient::config = {
        .host = "10.5.5.9",
        .transport_type = HTTP_TRANSPORT_OVER_TCP,
    };

    bool GoProClient::connect() {
        config.event_handler = GoProClient::receive_wrapper;
        return true;
    }

    GoProStatus GoProClient::requestStatus() {
        std::unique_lock<std::mutex> lock(clientMutex); 

        gopro::GoProStatus status = {
            .overall = false
        };

        if( ! isNetworkUp()) {
            LOG.debug("Network is down - exiting status request");
            return status;
        }

        //config.path = "/bacpac/se";
        config.path = "/gp/gpControl/status";
        config.query = "t=gizmolikespizza";//FIXME password
        config.user_data = this;
        config.is_async = false;

        //TODO tohle asi do connectu
        esp_http_client_handle_t client = esp_http_client_init(&config);

        local_response_len = 0;
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            int httpStatus = esp_http_client_get_status_code(client);
            LOG.debug("HTTP GET Status = %d, content_length = %d",// PRId64,
                httpStatus,
                esp_http_client_get_content_length(client));
                if (httpStatus == 200) {
                    //TODO if status == 200 decode json and read $.status.13 and convert to bool
                    gopro::GoProStatus goProStatus = decodeJsonBodyToStatus(client);
                    status.recordingStatus = goProStatus.recordingStatus;
                    status.recordingDuration = goProStatus.recordingDuration;
                }
            status.overall = true;
        } else {
            LOG.error("HTTP GET request failed: %s", esp_err_to_name(err));
        }
        LOG.debug("Output len after return %d", local_response_len);
        //FIXME want to get rid of this or use my own log wrapper
        ESP_LOG_BUFFER_HEX_LEVEL(TAG, local_response_buffer, local_response_len, ESP_LOG_DEBUG);

        esp_http_client_cleanup(client);

        return status;
    }

    // Define a custom deleter for cJSON
    struct cJSONDeleter {
        void operator()(cJSON* ptr) const {
            if (ptr) {
                cJSON_Delete(ptr);
            }
        }
    };

    GoProStatus GoProClient::decodeJsonBodyToStatus(esp_http_client_handle_t client){
        // see https://github.com/KonradIT/goprowifihack/blob/master/HERO4/CameraStatus.md

        local_response_buffer[
            local_response_len < MAX_HTTP_OUTPUT_BUFFER 
                ? local_response_len 
                : MAX_HTTP_OUTPUT_BUFFER] = 0;

        LOG.debug("Response end at: %d", 
                local_response_len < MAX_HTTP_OUTPUT_BUFFER 
                ? local_response_len 
                : MAX_HTTP_OUTPUT_BUFFER);
        LOG.debug("Response: %s", local_response_buffer);

        // NOTE: below line is not intended for runtime debugging but is very handy for development debugging
        //ESP_LOG_BUFFER_HEX_LEVEL(TAG, local_response_buffer, local_response_len, ESP_LOG_INFO);

        std::unique_ptr<cJSON, cJSONDeleter> root = std::unique_ptr<cJSON, cJSONDeleter>(cJSON_Parse(local_response_buffer));

        cJSON *statusObj = root ? cJSON_GetObjectItem(root.get(), "status") : nullptr;

        cJSON *recordingStatusObj = statusObj ? cJSON_GetObjectItem(statusObj, RECORDING_STATUS_KEY) : nullptr;
        cJSON *recodingDurationObj = statusObj ? cJSON_GetObjectItem(statusObj, RECORDING_DURATION_SECONDS_KEY) : nullptr;

        //TODO just a development debugging block
        if ( ! root || ! statusObj || ! recordingStatusObj || ! recodingDurationObj){
            LOG.error("UNEXPECTED: Some JSON object is missing (%d, %d, %d, %d). %s", 
                root ? 1 : 0,
                statusObj ? 1 : 0,
                recordingStatusObj ? 1 : 0,
                recodingDurationObj ? 1 : 0,
                local_response_buffer
            );
        }

        return {
            .recordingStatus = recordingStatusObj ? recordingStatusObj->valueint : -1,
            .recordingDuration = recodingDurationObj ? recodingDurationObj->valueint : -1
        };
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
        LOG.debug("Output len after return %d", local_response_len);
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
        config.buffer_size = MAX_HTTP_OUTPUT_BUFFER;

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
        LOG.debug("Output len after return %d", local_response_len);
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
                LOG.debug("HTTP_EVENT_ERROR");
                break;
            case HTTP_EVENT_ON_CONNECTED:
                LOG.debug("HTTP_EVENT_ON_CONNECTED");
                break;
            // I believe GO Pro's API doesn't redirect
            case HTTP_EVENT_REDIRECT:
                LOG.debug("HTTP_EVENT_REDIRECT");
                esp_http_client_set_header(evt->client, "From", "user@example.com");
                esp_http_client_set_header(evt->client, "Accept", "text/html");
                esp_http_client_set_redirection(evt->client);
                break;
            case HTTP_EVENT_HEADER_SENT:
                LOG.debug("HTTP_EVENT_HEADER_SENT");
                break;
            case HTTP_EVENT_ON_HEADER:
                LOG.debug("HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
                break;
            case HTTP_EVENT_ON_DATA:
                LOG.debug("HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
                // Clean the buffer in case of a new request
                if (output_len == 0) {
                    // we are just starting to copy the output data into the use
                    memset(local_response_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);
                }
                /*
                *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
                *  However, event handler can also be used in case chunked encoding is used.
                */
                //if (!esp_http_client_is_chunked_response(evt->client)) 
                {
                    // If user_data buffer is configured, copy the response into the buffer
                    int copy_len = 0;
                    if (sizeof(local_response_buffer)) {
                        // The last byte in local_response_buffer is kept for the NULL character in case of out-of-bound access.
                        //*
                        int copy_len = std::min(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                        if (copy_len > 0) {
                            LOG.debug("output_len = %d, copy_len = %d, evt->data_len = %d", output_len, copy_len, evt->data_len);
                            memcpy(local_response_buffer + output_len, evt->data, copy_len);

                        }
                         //   */
                        if (esp_http_client_is_chunked_response(evt->client)) {
                            int chunkLength;
                            esp_http_client_get_chunk_length(evt->client, &chunkLength);
                            LOG.debug("It is chunked & chunk size is: %d", chunkLength);
                            LOG.debug("It is chunked & singalized size is: %d", evt->data_len);
                            
                        } else {
                            LOG.debug("It is absolute & size is: %d", evt->data_len);
                        }

                        LOG.debug("Write addr = %x",local_response_buffer + output_len); 
                        LOG.debug("Available space = %d", MAX_HTTP_OUTPUT_BUFFER - output_len);
                        LOG.debug("output_len = %d", output_len);

                        /*
                        int copy_len;
                        if ((copy_len = esp_http_client_read_response(evt->client, 
                            local_response_buffer + output_len, 
                            MAX_HTTP_OUTPUT_BUFFER - output_len))) {
                                //TODO store read data
                                
                            output_len += copy_len;
                        }
                        */

            
                        output_len += copy_len;
                    } 
                    /*
                    else {
                        int content_len = esp_http_client_get_content_length(evt->client);
                        if (output_buffer == NULL) {
                            // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                            output_buffer = (char *) calloc(content_len + 1, sizeof(char));
                            output_len = 0;
                            if (output_buffer == NULL) {
                                LOG.error("Failed to allocate memory for output buffer");
                                return ESP_FAIL;
                            }
                        }
                        copy_len = std::min(evt->data_len, (content_len - output_len));
                        if (copy_len) {
                            memcpy(output_buffer + output_len, evt->data, copy_len);
                        }
                    }
                        */
                    
                }

                break;
            case HTTP_EVENT_ON_FINISH:
                LOG.debug("HTTP_EVENT_ON_FINISH");
                if (output_buffer != NULL) {
                    // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                    // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                    free(output_buffer);
                    output_buffer = NULL;
                }
                LOG.debug("Output len at finish %d", output_len);
                local_response_len = output_len;
                output_len = 0;
                break;
            case HTTP_EVENT_DISCONNECTED:
                LOG.debug("HTTP_EVENT_DISCONNECTED");
                int mbedtls_err = 0;
                esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
                if (err != 0) {
                    LOG.error("Last esp error code: 0x%x", err);
                    LOG.error("Last mbedtls failure: 0x%x", mbedtls_err);
                }
                if (output_buffer != NULL) {
                    free(output_buffer);
                    output_buffer = NULL;
                }
                output_len = 0;
                break;
        }
        return ESP_OK;
    }
}