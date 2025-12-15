#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <Preferences.h>

enum sensors{
  OBD,
  ACCEL,
  TEMP,
};

enum types{
  GAUGE,
  GRAPH,
  THERMOMETER,
  NULLTYPE,
};

typedef struct {
    const char* bitmap_file;   
    enum sensors sens;
    enum types type;           
    int decimals;                        
    const char* obd_code;                 
    float min;                       
    float max; 
    int resbytes;             
    float (*interpretation)(long raw_val); 
} DataTypes_t;

extern float rpm(long raw_val);
extern float boost(long raw_val);
extern const DataTypes_t OBDScreens[];
extern const int TOTAL_BITMAPS;

// semafori vari
extern SemaphoreHandle_t xSerialMutex; // non so se tenerlo

extern SemaphoreHandle_t xUIMutex;
extern volatile int ui_color;
extern volatile int ui_index;

extern SemaphoreHandle_t xReconnMutex;
extern volatile bool is_reconnect_needed; 

// queue dati
extern QueueHandle_t xObdDataQueue; 

// zero del giroscopio
extern SemaphoreHandle_t xBLEMutex;
extern volatile float zero_x, zero_y, zero_z;

// prototipi delle funzioni
extern void saveState(const char* state, int val);
extern int loadState(const char* state);
extern void setupWifi();
extern void setupBLE();
extern void setupScreen();
extern void changeBitmap();
extern void drawScreen(float val);
extern void checkWifiStatus();
extern int checkConnection();
extern float sendOBDCommand();
extern float ReadTemperature();
extern float readHumidity();
extern float ReadAcceleration();
extern float ReadIncline();
extern void InitSensors();

// task
extern void vUITask(void *pvParameters);
extern void vDataFetchTask(void *pvParameters);
extern void vBLETask(void *pvParameters);