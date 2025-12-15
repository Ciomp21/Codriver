#include <Arduino.h>
#include "sensors.h"

DHT dht(DHTPIN, DHTTYPE);

uint16_t DelayTempSensor = 2000; // DHT11 needs 2 seconds between reads
unsigned long lastTempReadTime = 0;

uint16_t DelayIMUSensor = 50; // IMU can be read more frequently
unsigned long lastIMUReadTime = 0;

void InitSensors()
{
    // Initialize Temperature/Humidity sensor
    dht.begin();

    // Print temperature as proof.
    Serial.println(F("DHT11 sensor test"));
    Serial.print(F("Temperature: "));
    Serial.println(dht.readTemperature());

    // Print humidity as proof.
    Serial.print(F("Humidity: "));
    Serial.println(dht.readHumidity());

    Serial.println(F("DHT11 sensor initialized."));


    // Initialize IMU sensor here


}

void ReadSensorData(LiveData *data)
{
    // Read data from additional sensors and update the LiveData structure

    unsigned long currentMillis = millis();

    // Get temperature event and print its value.
    if(currentMillis - lastTempReadTime >= DelayTempSensor)
    {
        // Read temperature as Celsius (the default)
        data->InternalTemp = dht.readTemperature();

        // Get humidity event and print its value.
        data->humidity = dht.readHumidity();

        lastTempReadTime = currentMillis;
    }
    else{
        lastTempReadTime = currentMillis;
    }


    // Read of the IMU sensor would go here

    currentMillis = millis();

    if (currentMillis - lastIMUReadTime >= DelayIMUSensor)
    {
        // Read IMU data and update data->accelX, data->accelY, data->accelZ, etc.

        
        lastIMUReadTime = currentMillis;
        
    }
    else{
        lastIMUReadTime = currentMillis;
    }

}