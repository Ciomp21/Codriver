#include "global.hpp"
#include "requests.hpp"
#include "screen.hpp"
#include "bleconnection.hpp"
#include "LittleFS.h"
#include <FS.h>
#include <Preferences.h>
#include "freertos/task.h"
#include "sensor.h"

void vOBDFetchTask(void *pvParameters)
{
    setupWifi();

    int cycleCounter = 0;
    DataCollector_t localSnap = {0};
    bool reconnect = true;
    int errorCounter = 0;

    while (1)
    {
        checkWifiStatus();

        if (reconnect)
        {
            int status = checkConnection();
            if (status == 0)
            {
                if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE){
                    err = 0;
                    ui_update = true;
                    xSemaphoreGive(xUIMutex);
                }
                reconnect = false;
                errorCounter = 0;
                
            }
            else
            {
                setError(1);
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            
            int ret = 0;
            bool readSuccess = false;
            int errorCount = 0;

            // the technique here is to read one OBD parameter at a time in a round-robin fashion
            // this reduces the load on the OBD bus and increases responsiveness for critical data

            // adding more reads is possibile but not quite easy


            // ATTENZIONE FATTO SOLO PER DEBUG!!!!, serve mettere i dati effettivi
            if (cycleCounter % 2 == 0)
            {
                ret = sendOBDCommand(PID_BOOST);
            }

            if (ret == -1) {
                Serial.println("Troppi errori, riconnessione... ");
                errorCounter++;
                if (errorCounter > 10){
                    reconnect = true;
                    if(xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE){
                        if(err == 0) setError(1);
                        xSemaphoreGive(xUIMutex);
                    }
                } 
            }
            
            cycleCounter++;

            vTaskDelay(pdMS_TO_TICKS(10));
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void vSENSFetchTask(void *pvParameters)
{
    // InitSensors();

    while (1)
    {
        // Read temperature and humidity, acceleration and incline

        // readTemperature();
        // readHumidity();
        // readIMU();

        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

void vSaveTask(void *pvParameters)
{
    while(1){
        
        if(xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE){
            if(ui_index == 0) {
                xSemaphoreGive(xUIMutex);
                vTaskDelay(pdMS_TO_TICKS(5000)); 
                continue;
            }
            saveState("screen", ui_index);
            saveState("color", ui_color);
            saveState("min", OBDScreens[ui_index].min);
            saveState("max", OBDScreens[ui_index].max);
            // Serial.println("Salvataggio");
            xSemaphoreGive(xUIMutex);
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
}

void vUITask(void *pvParameters)
{
    float received_val;
    setupScreen();

    while (1)
    {
        drawScreen();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


void vBLETask(void *pvParameters)
{
    setupBLE();

    while (1)
    {
        

        vTaskDelay(pdMS_TO_TICKS(500));

        // just for testing, read the temperature and print it
        float temp = temperatureRead();
        Serial.printf("🔥 Temp CPU: %.1f °C\n", temp);
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000);
    
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
    // xTaskCreatePinnedToCore(
    //     vSENSFetchTask,
    //     "ACC_Fetch",
    //     10000,
    //     NULL,
    //     1, // Priorità Bassa
    //     NULL,
    //     0 // Core 0 (I/O e rete)
    // );

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