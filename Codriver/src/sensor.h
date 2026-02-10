#pragma once
#include <Arduino.h>
#include <DHT.h>
#include "Wire.h"

#define DHTTYPE DHT11 /**< DHT TYPE 11 */
#define DHTPIN 10     // GPIO pin where the DHT11 is connected
#define MPU_ADDR 0x68 // I2C address of the MPU6050
#define I2C_SDA_PIN 4 // Pin per i dati I2C
#define I2C_SCL_PIN 5 // Pin per il clock I2C

// MPU6050 Register Map
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

int readTemperature();
int readHumidity();
int readAcceleration();
int readIncline();
void InitSensors();
int i2cReading(uint8_t reg, uint8_t len, uint8_t *buffer);
void i2ccommunicate(uint8_t reg, uint8_t data);
