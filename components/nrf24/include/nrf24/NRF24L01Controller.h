#pragma once

#include <system/Log.h>
#include <mirf.h>

namespace nrf24 {

    /* 
        Chipselect pins and SPI pins are defined globally in  static way
        using `idf.py menuconfig` and navigating to: nRF24L01 Configuration
    */
    class NRF24L01Controller {
        static const cima::system::Log LOG;

        NRF24_t dev;
        bool ready = false;

        public:
            NRF24L01Controller() = default;
            virtual ~NRF24L01Controller() = default;

            bool init();

            /* @param data max 32 bytes of data. */
            bool broadcastMessage(uint8_t *data);

            /* @param data target buffer where receive message is to be wertitten. */
            bool receiveMessage(uint8_t *data);

        private:
            void advancedSettings(NRF24_t * dev);
    };
}