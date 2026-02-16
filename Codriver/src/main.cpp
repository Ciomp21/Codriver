#include "global.hpp"
#include "requests.hpp"
#include "screen.hpp"
#include "bleconnection.hpp"
#include "LittleFS.h"
#include <FS.h>
#include <Preferences.h>
#include "freertos/task.h"
#include "sensor.h"

// -------------------------------------------------
//                      TASKS
// -------------------------------------------------


// This task handles the Wi-Fi fetching and connection

void vOBDFetchTask(void *pvParameters)
{
    setupWifi();

    int cycleCounter = 0;
    bool reconnect = true;
    int errorCounter = 0;
    int secondCycle = 0;

    while (1)
    {
        checkWifiStatus();
        
        if (reconnect)
        {
            int status = checkConnection();
            if (status == 0)
            {
                if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE)
                {
                    err = 0;
                    ui_update = true;
                    xSemaphoreGive(xUIMutex);
                }
                reconnect = false;
                errorCounter = 0;
            }
            else
            {
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        else
        {
            int ret = 0;

            // Logica Round-Robin (3 step)
            if (cycleCounter % 3 == 0)
            {
                ret = sendOBDCommand(PID_BOOST);
            } 
            else if (cycleCounter % 3 == 1) 
            {
                ret = sendOBDCommand(PID_RPM);
            } 
            else 
            {
                // Gestione dei PID rimanenti a rotazione
                switch (secondCycle) 
                {
                    case 2:
                        ret = sendOBDCommand(PID_COOLANT_TEMP);
                        break;
                    case 5:
                        ret = sendOBDCommand(PID_ENGINE_LOAD);
                        break;
                    case 8:
                        ret = sendOBDCommand(PID_BATTERY_VOLTAGE);
                        break;
                    case 11:
                        ret = sendOBDCommand(PID_OIL_TEMP);
                        break;
                }
                
                secondCycle++;
                if (secondCycle > 2) secondCycle = 0;
            }

            if (ret == -1)
            {
                Serial.println("Errore lettura OBD...");
                errorCounter++;
                if (errorCounter > 10)
                {
                    reconnect = true;
                    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE)
                    {
                        if (err == 0) setError(1);
                        xSemaphoreGive(xUIMutex);
                    }
                }
            }
            else 
            {
                errorCounter = 0; // Reset errori se la lettura è OK
            }

            cycleCounter++;
            if (cycleCounter >= 30) {
                cycleCounter = 0;
            }
            
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void vSENSFetchTask(void *pvParameters)
{
    InitSensors();
    Serial.println("Sensor Task Started.");

    while (1)
    {
        // Read temperature and humidity, acceleration and incline

        readTemperature();
        readHumidity();
        readIMU();

        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

void vSaveTask(void *pvParameters)
{
    while (1)
    {

        if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            if (ui_index == 0)
            {
                xSemaphoreGive(xUIMutex);
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
            saveState("screen", ui_index);
            saveState("color", ui_color);
            saveState("min", OBDScreens[ui_index].min);
            saveState("max", OBDScreens[ui_index].max);
            Serial.println("Salvataggio");
            xSemaphoreGive(xUIMutex);
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
}

void vUITask(void *pvParameters)
{
    setupScreen();

    while (1)
    {
        // Controllo input da Serial Monitor
        if (Serial.available() > 0) {
            char incomingChar = Serial.read();
            
            // 10 è '\n' (New Line), 13 è '\r' (Carriage Return)
            if (incomingChar == '\n' || incomingChar == '\r') {
                if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                    ui_index++;
                    if (ui_index >= TOTAL_BITMAPS) {
                        ui_index = 0; // Torna al primo schermo
                    }
                    ui_update = true;
                    Serial.printf("Input ricevuto: Cambio schermo all'indice %d\n", ui_index);
                    xSemaphoreGive(xUIMutex);
                }
                
                // Pulisce il buffer seriale per evitare cambi multipli con un solo invio
                while(Serial.available() > 0) Serial.read(); 
            }
        }

        drawScreen();

        vTaskDelay(pdMS_TO_TICKS(20)); 
    }
}

void vBLETask(void *pvParameters)
{
    setupBLE();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

#ifdef TESTING
    pinMode(SCREEN_BUTTON_PIN, INPUT_PULLUP);
#endif

    xUIMutex = xSemaphoreCreateMutex();
    xSerialMutex = xSemaphoreCreateMutex();
    xBLEMutex = xSemaphoreCreateMutex();
    xDataMutex = xSemaphoreCreateMutex();

    xTaskCreatePinnedToCore(
        vOBDFetchTask,
        "OBD_Fetch",
        16000,
        NULL,
        1, // Priorità Bassa
        NULL,
        0 // Core 0 (I/O e rete)
    );

    xTaskCreatePinnedToCore(
        vSaveTask,
        "Save_data",
        10000,
        NULL,
        1, // Priorità Bassa
        NULL,
        0 // Core 0 (I/O e rete)
    );

    xTaskCreatePinnedToCore(
        vSENSFetchTask,
        "ACC_Fetch",
        10000,
        NULL,
        1, // Priorità Bassa
        NULL,
        0 // Core 0 (I/O e rete)
    );

    xTaskCreatePinnedToCore(
        vUITask,
        "UI_Rendering",
        15000,
        NULL,
        1, // Priorità ALTA
        NULL,
        1 // Core 1 (Dedicato all'UI)
    );

    xTaskCreatePinnedToCore(
        vBLETask,
        "BLE_Handler",
        16000,
        NULL,
        1, // Priorità Media
        NULL,
        0 // Core 0
    );
}

void loop()
{
    vTaskDelete(NULL);
}