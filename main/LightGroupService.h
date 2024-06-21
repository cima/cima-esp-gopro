#pragma once

#include <system/PWMDriver.h>

namespace cima {
    class LightGroupService {

        static cima::system::Log LOGGER;

        system::PWMDriver ledDriver;

        bool ready = false;

    public:
        LightGroupService(system::PWMDriver &ledDriver);
        
        void loop();
        void setReady(bool isReady);

        void demoLoop();
    };

    typedef std::map<std::string, boost::reference_wrapper<cima::LightGroupService>> LightGroupMap;
}