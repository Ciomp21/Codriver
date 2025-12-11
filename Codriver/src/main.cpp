// includiamo il file con i mutex etc
#include "global.hpp"

// tutti gli alti import
#include "requests.hpp"
#include "screen.hpp"
#include "bleconnection.hpp" 
#include "LittleFS.h"
#include <FS.h>
#include <Preferences.h>

void vDataFetchTask(void *pvParameters) {
    
    while (1) {        
        float value = sendOBDCommand();
        if (xObdDataQueue != NULL) {
            // qui sto passando i dati alla queue in modo che la task del display
            // possa prenderli
            xQueueSend(xObdDataQueue, &value, pdMS_TO_TICKS(10));
        }
        vTaskDelay(pdMS_TO_TICKS(150)); 
    }
}

void vUITask(void *pvParameters) {
    float val;

    setupScreen();

    while (1) {
        if (xQueueReceive(xObdDataQueue, &val, 0) == pdPASS) {            
            drawScreen(val);
        }
        vTaskDelay(pdMS_TO_TICKS(50)); 
    }
}

void vBLETask(void *pvParameters) {
    //ble fa tutto tramite setup
    setupBLE();
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // inizilizzo il semaforo
    xUIMutex = xSemaphoreCreateMutex();
    xObdDataQueue = xQueueCreate(5, sizeof(float));

    if (xSemaphoreTake(xUIMutex, portMAX_DELAY) == pdTRUE) {
        ui_color = loadState("color");
        ui_index = loadState("screen");
        ui_color = 0xFFFFFF;
        
        xSemaphoreGive(xUIMutex);
    }


    xTaskCreatePinnedToCore(
        vDataFetchTask,
        "OBD_Fetch",
        10000,
        NULL,
        1, // Priorità Bassa
        NULL,
        0  // Core 
    );

    xTaskCreatePinnedToCore(
        vUITask,
        "UI_Rendering",
        15000,
        NULL,
        3, // Priorità Alta
        NULL,
        1 
    );

    xTaskCreatePinnedToCore(
        vBLETask,
        "BLE_Handler",
        4096,
        NULL,
        2,
        NULL,
        0
    );

    changeBitmap();
    
}

void loop() {
    vTaskDelete(NULL); // Elimina il loop di Arduino
}