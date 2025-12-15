

#include "global.hpp" 
#include <Arduino.h> 
//definizione di tutto l'hpp

SemaphoreHandle_t xSerialMutex = NULL; 
SemaphoreHandle_t xUIMutex = NULL;
QueueHandle_t xObdDataQueue = NULL; 
SemaphoreHandle_t xReconnMutex = NULL;
volatile bool is_reconnect_needed = false;


volatile int ui_color = 0xFFFFFF; 
volatile int ui_index = 1;

Preferences preferences;

float rpm(long raw_val) {
    return (float)raw_val / 4.0; 
}

float boost(long raw_val) {
    return (((float)raw_val / 100.0) - 1.0); 
}

const DataTypes_t OBDScreens[] = {
    {
        .bitmap_file = "/boost.bin",
        .type = GAUGE,
        .decimals = 0,
        .obd_code = "0C", 
        .min = 0.0,
        .max = 5400.0,
        .resbytes = 2,
        .interpretation = rpm
    },
    {
        .bitmap_file = "/boost.bin",
        .type = GAUGE,
        .decimals = 2,
        .obd_code = "0B", 
        .min = -0.5,
        .max = 2.0,
        .resbytes = 1,
        .interpretation = boost
    },
    {
        .bitmap_file = "/gforce.bin",
        .type = GAUGE,
        .decimals = 2,
        .obd_code = "0B", 
        .min = -0.2,
        .max = 2.0,
        .resbytes = 1,
        .interpretation = boost
    }
};

const int TOTAL_BITMAPS = sizeof(OBDScreens) / sizeof(OBDScreens[0]);

void saveState(const char* state, int val) {
    preferences.begin("infotainment", false); 
    preferences.putInt(state, val);
    preferences.end();
}

int loadState(const char* state) {
    preferences.begin("infotainment", true); 
    int ret = preferences.getInt(state, 0); 
    preferences.end();
    
    if (strcmp(state, "color") == 0 && ret == 0) {
        return 0xFFFFFF;
    }
    return ret;
}