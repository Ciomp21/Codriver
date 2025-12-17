#include "global.hpp"
#include "requests.hpp"
#include "screen.hpp"
#include "bleconnection.hpp" 
#include "LittleFS.h"
#include <FS.h>
#include <Preferences.h>
#include "freertos/task.h"
#include "sensor.h"


void vOBDFetchTask(void *pvParameters) {    
    setupWifi(); 

    int cycleCounter = 0;
    DataCollector_t localSnap = {0};

    while (1) {
        // First of all check if a reconnection is needed
        bool needed;
        if (xSemaphoreTake(xReconnMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            needed = is_reconnect_needed;
            xSemaphoreGive(xReconnMutex);
        } else {
            needed = true; 
        }

        if (needed) {
            vTaskDelay(pdMS_TO_TICKS(500)); 
            continue;
        }
        int ret = 0;
        bool readSuccess = false;
        int errorCount = 0;
        
        // the technique here is to read one OBD parameter at a time in a round-robin fashion
        // this reduces the load on the OBD bus and increases responsiveness for critical data

        // adding more reads is possibile but not quite easy
        
        if (cycleCounter % 2 == 0) {
            ret =sendOBDCommand(PID_BOOST);
        } 
        else {
            // adjust the % parameter to add more reads here
            int secondaryIndex = (cycleCounter / 2) % 4; 

            switch (secondaryIndex) {
                case 0:
                    ret = sendOBDCommand(PID_COOLANT_TEMP);
                    break;

                case 1: 
                    ret = sendOBDCommand(PID_BATTERY_VOLTAGE);
                    break;

                case 2: 
                    ret =sendOBDCommand(PID_COOLANT_TEMP);
    
                    break;

                case 3: 
                    ret = sendOBDCommand(PID_ENGINE_LOAD);
                    break;
                // add more casese here if needed
                // these are the slow ones
            }
        }

        if (!readSuccess && ret == -1) { 
            errorCount++;
            if (errorCount > 5) {
                if (xSemaphoreTake(xReconnMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    is_reconnect_needed = true;
                    xSemaphoreGive(xReconnMutex);
                }
                errorCount = 0;
            }
        }

        cycleCounter++;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void vSENSFetchTask(void *pvParameters) {
    InitSensors();

    while (1) {
        // Read temperature and humidity, acceleration and incline

        // readTemperature();
        // readHumidity();
        readAcceleration();
        // readIncline();

        vTaskDelay(pdMS_TO_TICKS(150)); 
    }
}

void vUITask(void *pvParameters) {
    float received_val;
    setupScreen();

    while (1) {
        drawScreen(); 

        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

void vBLETask(void *pvParameters) {
    setupBLE(); 
    
    while (1) {
        checkWifiStatus(); 
        
        // ble viene usato poco, lo uso anche per la riconnessione
        bool needed = false;
        if (xSemaphoreTake(xReconnMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            needed = is_reconnect_needed;
            xSemaphoreGive(xReconnMutex);
        } else {
            needed = true;
        }

        if (needed) {
            Serial.println("🌐 Tentativo di RICONNESSIONE TCP...");
            int status = checkConnection(); 
            
            if (status == 0) {
                if (xSemaphoreTake(xReconnMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    is_reconnect_needed = false;
                    xSemaphoreGive(xReconnMutex);
                }
                vTaskDelay(pdMS_TO_TICKS(500)); 
            } else {
                vTaskDelay(pdMS_TO_TICKS(2000)); 
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(500)); 
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    xUIMutex = xSemaphoreCreateMutex();
    xSerialMutex = xSemaphoreCreateMutex(); 
    xReconnMutex = xSemaphoreCreateMutex();
    xBLEMutex = xSemaphoreCreateMutex();
    xDataMutex = xSemaphoreCreateMutex();
    
    if (xSemaphoreTake(xUIMutex, portMAX_DELAY) == pdTRUE) {
        ui_color = loadState("color");
        ui_index = loadState("screen");
        xSemaphoreGive(xUIMutex);
    }

    xTaskCreatePinnedToCore(
        vOBDFetchTask,
        "OBD_Fetch",
        10000,
        NULL,
        1, // Priorità Bassa
        NULL,
        0  // Core 0 (I/O e rete)
    );

    xTaskCreatePinnedToCore(
        vSENSFetchTask,
        "ACC_Fetch",
        10000,
        NULL,
        1, // Priorità Bassa
        NULL,
        0  // Core 0 (I/O e rete)
    );

    xTaskCreatePinnedToCore(
        vUITask,
        "UI_Rendering",
        15000,
        NULL,
        3, // Priorità ALTA
        NULL,
        1  // Core 1 (Dedicato all'UI)
    );

    xTaskCreatePinnedToCore(
        vBLETask,
        "BLE_Handler",
        4096,
        NULL,
        2, // Priorità Media
        NULL,
        0  // Core 0
    );

    changeBitmap();
    ui_index=1;
}

void loop() {
    vTaskDelete(NULL); 
}