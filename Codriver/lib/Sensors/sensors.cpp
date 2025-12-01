#include <Arduino.h>
#include "sensors.h"
#include "Wire.h"

#define MPU_ADDR 0x68 // I2C address of the MPU6050

DHT dht(DHTPIN, DHTTYPE);

void i2c_communicate(uint8_t reg, uint8_t data)
{
    // Starts I2C transmission to the MPU6050
    // Write the register address and the data byte
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
}

void i2cReadBytes(uint8_t reg, uint8_t len, uint8_t *buffer)
{
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, len, true);
    for (int i = 0; i < len; i++)
        buffer[i] = Wire.read();
}

void InitTempHumiditySensor()
{
    dht.begin();

    // Print temperature as proof.
    Serial.println(F("DHT11 sensor test"));

    Serial.print(F("Temperature: "));
    Serial.println(dht.readTemperature());

    // Print humidity as proof.
    Serial.print(F("Humidity: "));
    Serial.println(dht.readHumidity());

    Serial.println(F("DHT11 sensor initialized."));
}

void InitIMUSensor()
{
    // | Registro       | Indirizzo | Funzione                       |
    // | -------------- | --------- | ------------------------------ |
    // | `PWR_MGMT_1`   | `0x6B`    | Wake-up / clock source         |
    // | `SMPLRT_DIV`   | `0x19`    | Sample rate                    |
    // | `CONFIG`       | `0x1A`    | Digital low-pass filter        |
    // | `GYRO_CONFIG`  | `0x1B`    | Gyro full scale                |
    // | `ACCEL_CONFIG` | `0x1C`    | Accel full scale               |
    // | `ACCEL_XOUT_H` | `0x3B`    | Inizio burst read dati sensori |
    //  Indirizzo I²C del sensore: 0x68
    // Physical connection: SDA -> GPIO21, SCL -> GPIO22

    Wire.begin(21, 22);    // Initialize I2C with the pins used on ESP32-S3
    Wire.setClock(400000); // Set I2C frequency to 400kHz

    // Wake up the IMU sensor (MPU6050) by writing to the PWR_MGMT_1 register
    i2c_communicate(0x6B, 0x00);

    i2c_communicate(0x19, 0x07); // 1 kHz / (1+7) = 125 Hz

    i2c_communicate(0x1A, 0x03); // DLPF = 3 → BW=44 Hz (stabile per auto)

    i2c_communicate(0x1B, 0x08); // Gyro full scale ±500°/s

    i2c_communicate(0x1C, 0x08); // Accel full scale ±4g

    Serial.println("MPU6050 configured");
}

void InitSensors()
{
    // Initialize Temperature/Humidity sensor
    InitTempHumiditySensor();

    // Initialize IMU sensor here
    InitIMUSensor();
}

// ==================================================================
// Read data from sensors and update LiveData structure
// ==================================================================

int16_t ax, ay, az; // Accelerometer raw values
int16_t gx, gy, gz; // Gyroscope raw values

uint16_t DelayIMUSensor = 20; // Read IMU every 20 ms
unsigned long lastIMUReadTime = 0;

uint16_t DelayTempSensor = 2000; // Read Temp/Humidity every 2 seconds
unsigned long lastTempReadTime = 0;

void ReadSensorData(LiveData *data)
{
    // Read data from additional sensors and update the LiveData structure

    unsigned long currentMillis = millis();

    // Get temperature event and print its value.
    if (currentMillis - lastTempReadTime >= DelayTempSensor)
    {
        // Read temperature as Celsius (the default)
        data->InternalTemp = dht.readTemperature();

        // Get humidity event and print its value.
        data->humidity = dht.readHumidity();

        lastTempReadTime = currentMillis;
    }

    // Read of the IMU sensor would go here
    currentMillis = millis();

    if (currentMillis - lastIMUReadTime >= DelayIMUSensor)
    {
        // Read IMU data and update data->accelX, data->accelY, data->accelZ, etc.

        uint8_t buffer[14];
        i2cReadBytes(0x3B, 14, buffer);

        ax = (buffer[0] << 8) | buffer[1];
        ay = (buffer[2] << 8) | buffer[3];
        az = (buffer[4] << 8) | buffer[5];

        gx = (buffer[8] << 8) | buffer[9];
        gy = (buffer[10] << 8) | buffer[11];
        gz = (buffer[12] << 8) | buffer[13];

        // Convert raw values to 'g' and '°/s' based on full scale settings
        data->accelX = (float)ax / 8192.0f;
        data->accelY = (float)ay / 8192.0f;
        data->accelZ = (float)az / 8192.0f;

        data->gyroX = (float)gx / 65.5f;
        data->gyroY = (float)gy / 65.5f;
        data->gyroZ = (float)gz / 65.5f;

        lastIMUReadTime = currentMillis;
    }
}