#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <Preferences.h>
#include <map>

// Here we define the pid we'll use
#define PID_RPM "0C"
#define PID_BOOST "0B"
#define PID_COOLANT_TEMP "05"
#define PID_BATTERY_VOLTAGE "42"
#define PID_ENGINE_LOAD "04"

// this one is a special one
#define PID_OIL_TEMP "22114D"



#define DEG2RAD 0.0174532925

// HERE WE CHOOSE IF WE ARE IN TESTING MODE OR NOT, IF WE ARE IN TESTING MODE THE UI WILL BE SIMULATED WITH SOME RANDOM VALUES, OTHERWISE IT WILL BE DRIVEN BY THE REAL DATA
// #define TESTING

typedef struct
{
    int resbytes;
    void (*interpretation)(long raw_val);
} OBDCommand_t;

typedef struct
{
    const char *bitmap_file;
    int decimals;
    float min;
    float max;
    void (*drawFunction)();
} DataTypes_t;

typedef struct
{
    float speed;
    float rpm;
    float boost;
    float engineTemp;
    float oilTemp;
    float coolantTemp;
    float batteryVoltage;
    float intakeAirTemp;
    float throttlePosition;
    float fuelLevel;
    int gearPosition;
    float steerAngle;
    float brakePressure;

    float fuelConsumption;
    float engineLoad;
    int gearSuggestion;

    int extimatedRange;
    int EcoScore;

    // Temperature/Humidity sensor data
    float InternalTemp;
    float ExternalTemp;
    float humidity;

    // IMU sensor data

    // For G-force calculations
    float accelX;
    float accelY;
    float accelZ;

    // Might not be useful
    // float gyroX;
    // float gyroY;
    // float gyroZ;

    float roll;
    float pitch;

} DataCollector_t;

extern void rpm(long raw_val);
extern void boost(long raw_val);
extern void coolant_temp(long raw_val);
extern void engine_load(long raw_val);
extern void battery_voltage(long raw_val);
extern void oil_temp(long raw_val);

extern DataTypes_t OBDScreens[];
extern std::map<std::string, OBDCommand_t> obdCommandMap;
extern std::map<int, char *> errorMap;
extern const int TOTAL_BITMAPS;

// semafori vari
extern SemaphoreHandle_t xSerialMutex; // non so se tenerlo

extern SemaphoreHandle_t xUIMutex;
extern volatile int ui_color;
extern volatile int ui_index;
extern volatile bool ui_update;

extern int err;

extern bool is_wifi_connected;
extern bool is_tcp_connected;

// queue dati
extern SemaphoreHandle_t xDataMutex;
extern DataCollector_t liveData;

// zero del giroscopio
extern SemaphoreHandle_t xBLEMutex;
extern volatile float zero_x, zero_y, zero_z;

// prototipi delle funzioni
extern void saveState(const char *state, int val);
extern int loadState(const char *state);
extern void setupWifi();
extern void setupBLE();

// funzioni per disegnare le schermate
extern void setupScreen();
extern void changeBitmap(int index);
extern void drawBoost();
extern void drawRPM();
extern void drawInit();
extern void drawAcceleration();
extern void drawScreen();
extern void drawPitch();
extern void drawRoll();
extern void drawBattery();
extern void drawTemperature();
extern void drawAirTemperature();

extern void checkWifiStatus();
extern int checkConnection();
extern int sendOBDCommand(const char *pid);
extern int readTemperature();
extern int readHumidity();
extern void readIMU();
extern void InitSensors();
extern void setError(int errCode);
extern void resolveError(int errCode);
extern int getError();

// task
extern void vUITask(void *pvParameters);
extern void vDataFetchTask(void *pvParameters);
extern void vBLETask(void *pvParameters);