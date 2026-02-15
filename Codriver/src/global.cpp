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
volatile int ui_index = 1;
volatile bool ui_update = true;

int err = 0;

Preferences preferences;

// here you need to add more commands
std::map<std::string, OBDCommand_t> obdCommandMap = {
    {PID_RPM,             {2, rpm}},             
    {PID_BOOST,           {1, boost}},           
    {PID_COOLANT_TEMP,    {1, coolant_temp}},   
    {PID_ENGINE_LOAD,     {1, engine_load}},     
    {PID_BATTERY_VOLTAGE, {2, battery_voltage}}

    // You can expand with new pids!
};

std::map<int, char *> errorMap = {
    {1, "Tentativo di riconnessione OBD..."},
    {2, "Errore inizializzazione LittleFs"},
    {3, "Errore FS, schermo non riconosciuto"},
    {3, "Errore Accelerometro"},
    {4, "Errore fetching termometro"},

    // Also you can add more errors
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
        .bitmap_file = "/rpm.bin",
        .decimals = 0,
        .min = 0,
        .max = 8000,
        .drawFunction = drawRPM,
    },
    {
        .bitmap_file = "/gforce.bin",
        .decimals = 2,
        .min = 0,
        .max = 2.0,
        .drawFunction = drawAcceleration,
    },
    {
        .bitmap_file = "/temp.bin",
        .decimals = 0,
        .min = 0,
        .max = 0,
        .drawFunction = drawTemperature,
    },
    {
        .bitmap_file = "/battery.bin",
        .decimals = 0,
        .min = 0,
        .max = 0,
        .drawFunction = drawBattery,
    },
    {
        .bitmap_file = "/PR.bin",
        .decimals = 0,
        .min = -90,
        .max = 90,
        .drawFunction = drawRoll,
    },
    {
        .bitmap_file = "/PR.bin",
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
    Serial.printf("Errore inviato: %s\n", msg);
    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        if (err == 0){
            err = errCode;
            ui_update = true;
        }
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

void resolveError(int errCode){
    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        if(err == errCode) {
            err = 0;
        }
        xSemaphoreGive(xUIMutex);
    }
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
    if (liveData.boost < 0.0) liveData.boost = 0.0;
}

void coolant_temp(long raw_val) {
    liveData.coolantTemp = (float)(raw_val - 40);
}

void engine_load(long raw_val) {
    liveData.engineLoad = ((float)raw_val * 100.0) / 255.0;
}

void battery_voltage(long raw_val) {
    liveData.batteryVoltage = (float)raw_val / 1000.0;
}