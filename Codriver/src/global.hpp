// global.hpp

#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#include <Preferences.h>

//non so se serve davvero
enum types{
  GAUGE,
  TEMP,
  ACCEL,
  NULLTYPE,
};

// IMPORTANTE: ho pensato fosse meglio tirare tutto insieme
typedef struct {
    const char* bitmap_file;   
    enum types type;           
    int decimals;                        
    const char* obd_code;                 
    float min;                       
    float max; 
    int resbytes;             
    float (*interpretation)(long raw_val); 
} DataTypes_t;

/*
    queste soono le funzioni di interpretazione che usiamo per convertire il formato
    dati che ci viene restituito dalle chiamate all'OBD
*/
extern float rpm(long raw_val);
extern float boost(long raw_val);


extern const DataTypes_t OBDScreens[];
extern const int TOTAL_BITMAPS;

// importante usare i semafori per garantire
extern SemaphoreHandle_t xUIMutex;
extern volatile int ui_color;
extern volatile int ui_index;

extern QueueHandle_t xObdDataQueue; 


// --- Prototipi delle Funzioni NVS ---
extern void saveState(const char* state, int val);
extern int loadState(const char* state);

// --- Prototipi delle Task (main.cpp dovrà implementarle) ---
extern void vUITask(void *pvParameters);
extern void vDataFetchTask(void *pvParameters);
extern void vBLETask(void *pvParameters);