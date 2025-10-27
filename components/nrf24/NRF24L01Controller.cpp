#include <nrf24/NRF24L01Controller.h>
#include <mirf.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace nrf24 {
    const cima::system::Log NRF24L01Controller::LOG("NRF24L01Controller");

    bool NRF24L01Controller::init(){
        LOG.info("Radio init");
        
        Nrf24_init(&dev);
        uint8_t payload = 32;
        uint8_t channel = CONFIG_RADIO_CHANNEL;
        Nrf24_config(&dev, channel, payload);

        // Set my own address using 5 characters
        esp_err_t ret = Nrf24_setRADDR(&dev, (uint8_t *)"GOPRO");
        if (ret != ESP_OK) {
            LOG.error(pcTaskGetName(NULL), "nrf24l01 not installed: Receiver address not set. %d", ret);
            ready = false;
            return false;
        }

        ret = Nrf24_setTADDR(&dev, (uint8_t *)"GOPRO");
        if (ret != ESP_OK) {
            LOG.error(pcTaskGetName(NULL), "nrf24l01 not installed: Transmitter address not set. %d", ret);
            ready = false;
            return false;
        }

        advancedSettings(&dev);

        // Print settings
        Nrf24_printDetails(&dev);

        ready = true;
        return ready;
    };

    bool NRF24L01Controller::broadcastMessage(uint8_t *data) {
        //Nrf24_sendNoAck(&dev, data);
        Nrf24_send(&dev, data);

        if (Nrf24_isSend(&dev, 1000)) {
            LOG.info("Send successful.");
            return true;
        }
        
        LOG.error("Send failed.");
        return false;
    }

    bool NRF24L01Controller::receiveMessage(uint8_t *data){
        // Wait for received data
        if (Nrf24_dataReady(&dev)) {
            Nrf24_getData(&dev, data);
            //ESP_LOGI(pcTaskGetName(NULL), "Got data:%s", data);
            //ESP_LOG_BUFFER_HEXDUMP(pcTaskGetName(NULL), buf, payload, ESP_LOG_INFO);

            return true;
        }
        return false;
    }

    void NRF24L01Controller::advancedSettings(NRF24_t * dev) {
#if CONFIG_ADVANCED
        LOG.info("Setting advanced properties");

#if CONFIG_RF_RATIO_2M
        LOG.info("Set RF Data Ratio to 2MBps");
        Nrf24_SetSpeedDataRates(dev, 1);
#endif // CONFIG_RF_RATIO_2M

#if CONFIG_RF_RATIO_1M
        LOG.info("Set RF Data Ratio to 1MBps");
        Nrf24_SetSpeedDataRates(dev, 0);
#endif // CONFIG_RF_RATIO_2M

#if CONFIG_RF_RATIO_250K
        LOG.info("Set RF Data Ratio to 250KBps");
        Nrf24_SetSpeedDataRates(dev, 2);
#endif // CONFIG_RF_RATIO_2M

        LOG.info("CONFIG_RETRANSMIT_DELAY=%d", CONFIG_RETRANSMIT_DELAY);
        Nrf24_setRetransmitDelay(dev, CONFIG_RETRANSMIT_DELAY);
#endif // CONFIG_ADVANCED
    }
}