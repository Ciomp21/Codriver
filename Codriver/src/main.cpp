#include "global.hpp"
#include "requests.hpp"
#include "screen.hpp"
#include "bleconnection.hpp" 
#include "LittleFS.h"
#include <FS.h>
#include <Preferences.h>
#include "freertos/task.h"
#include "sensor.h"

void vDataFetchTask(void *pvParameters) {
    float value;
    setupWifi(); 
    InitSensors();

    while (1) {
        bool needed;
        if (xSemaphoreTake(xReconnMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            needed = is_reconnect_needed;
            xSemaphoreGive(xReconnMutex);
        } else {
            needed = true; 
        }

        // delay se perdo la connessione
        if (needed) {
            vTaskDelay(pdMS_TO_TICKS(500)); 
            continue;
        }

        int index = 0;

        if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            index = ui_index;
            xSemaphoreGive(xUIMutex);
        } else {
            index = ui_index;
        }


        switch (OBDScreens[index].sens){
        case ACCEL:
            value = ReadAcceleration();
            break;
        case TEMP:
            /* code */
            break;
        default:
            value = sendOBDCommand(); 
            break;
        }

        // per capire meglio
        // Serial.print("valore");
        // Serial.println(value);

        if (value != -1.0) {
            //mando sulla queue solo valori validi
            xQueueSend(xObdDataQueue, &value, pdMS_TO_TICKS(10));
        } else {
            // Dati non validi/connessione persa
            // forse da cambiare
            if (xSemaphoreTake(xReconnMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                is_reconnect_needed = true;
                xSemaphoreGive(xReconnMutex);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(150)); 
    }
}


void vUITask(void *pvParameters) {
    float received_val;
    setupScreen();

    while (1) {
        if (xQueueReceive(xObdDataQueue, &received_val, 0) == pdPASS) {
            Serial.print("UI: Valore ricevuto dalla queue: ");
            Serial.println(received_val);
            drawScreen(received_val); 
        }
        // passo -1 se voglio semplicemente fare lo smoothing
        drawScreen(-1); 

        vTaskDelay(pdMS_TO_TICKS(50)); 
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
    xObdDataQueue = xQueueCreate(5, sizeof(float));
    xSerialMutex = xSemaphoreCreateMutex(); 
    xReconnMutex = xSemaphoreCreateMutex();
    xBLEMutex = xSemaphoreCreateMutex();
    
    if (xSemaphoreTake(xUIMutex, portMAX_DELAY) == pdTRUE) {
        ui_color = loadState("color");
        ui_index = loadState("screen");
        xSemaphoreGive(xUIMutex);
    }

    xTaskCreatePinnedToCore(
        vDataFetchTask,
        "OBD_Fetch",
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
    ui_index=2;
}

void loop() {
    vTaskDelete(NULL); 
}