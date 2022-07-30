#include "sdkconfig.h"
#include <stdlib.h>
#include <string>
#include <stdio.h>
#include <stdarg.h>

#include <esp_log.h>


extern "C" void app_main(void) { 
    std::string message("Gizmo Likes Pizza.");
    ESP_LOGD("CIMA", "%s hu hu hu", message.c_str());
}