#include "global.hpp"
#include "screen.hpp"
#include <Arduino.h>
// definizione di tutto l'hpp

SemaphoreHandle_t xSerialMutex = NULL;
SemaphoreHandle_t xUIMutex = NULL;
SemaphoreHandle_t xDataMutex = NULL;
QueueHandle_t xObdDataQueue = NULL;
SemaphoreHandle_t xReconnMutex = NULL;
DataCollector_t liveData = {-1};

SemaphoreHandle_t xBLEMutex = NULL;
volatile float zero_x = 0.0;
volatile float zero_y = 0.0;
volatile float zero_z = 0.0;


// Base startup values for the UI, will be overwritten by the saved states
// Saved states are loaded in setupScreen function
volatile int ui_color = 0xFFFFFF;
volatile int ui_index = 0;
volatile bool ui_update = false;

int err = 0;

Preferences preferences;

// here you need to add more commands
std::map<std::string, OBDCommand_t> obdCommandMap = {
    {PID_RPM, {2, rpm}},
    {PID_BOOST, {1, boost}},
    // Add more PID-command mappings here
};

std::map<int, char *> errorMap = {
    {1, "Tentativo di riconnessione OBD..."},
};

// here you can add more screens
DataTypes_t OBDScreens[] = {
    {.bitmap_file = "/init.bin",
     .decimals = 0,
     .min = -1.0,
     .max = -1.0,
     .drawFunction = drawInit},
    {
        .bitmap_file = "/boost.bin",
        .decimals = 2,
        .min = -0.3,
        .max = 2.0,
        .drawFunction = drawBoost,
    },
    {
        .bitmap_file = "/gforce.bin",
        .decimals = 2,
        .min = 0,
        .max = 2.0,
        .drawFunction = drawAcceleration,
    },
    {
        .bitmap_file = "/pitch.bin",
        .decimals = 0,
        .min = -90,
        .max = 90,
        .drawFunction = drawCarFront,
    },
    {
        .bitmap_file = "/pitch.bin",
        .decimals = 0,
        .min = -90,
        .max = 90,
        .drawFunction = drawPitch,
    },
};

const int TOTAL_BITMAPS = sizeof(OBDScreens) / sizeof(OBDScreens[0]);

void saveState(const char *state, int val)
{
    preferences.begin("infotainment", false);
    preferences.putInt(state, val);
    preferences.end();
}

int loadState(const char *state)
{
    preferences.begin("infotainment", true);
    int ret = preferences.getInt(state, 0);
    preferences.end();

    if (strcmp(state, "color") == 0 && ret == 0)
    {
        return 0xFFFFFF;
    }
    return ret;
}

void setError(int errCode)
{
    const char *msg = errorMap[errCode];
    // Serial.printf("Errore inviato: %s\n", msg);
    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        ui_update = true;
        if (err == 0)
            err = errCode;
        xSemaphoreGive(xUIMutex);
    }
}

int getError()
{
    int ret = 0;
    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        ret = err;
        xSemaphoreGive(xUIMutex);
    }
    return ret;
}

// Here u can create new interpretation functions
// keep in mind that u already have accesso to liveData structure
void rpm(long raw_val)
{
    liveData.rpm = (float)raw_val / 4.0;
}

void boost(long raw_val)
{
    liveData.boost = (((float)raw_val / 100.0) - 1.0);
}