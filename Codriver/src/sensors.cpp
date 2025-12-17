#include "sensor.h"
#include "global.hpp"
#include "complementary_filter.hpp"

DHT dht(DHTPIN, DHTTYPE);

ComplementaryFilter filter;

#define I2C_SDA_PIN 4 // Pin per i dati I2C
#define I2C_SCL_PIN 5 // Pin per il clock I2C

void i2c_communicate(uint8_t reg, uint8_t data)
{
    // Starts I2C transmission to the MPU6050
    // Write the register address and the data byte
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
}

void i2cReading(uint8_t reg, uint8_t len, uint8_t *buffer)
{
    Wire.beginTransmission(MPU_ADDR);

    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)len, true);
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
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000);

    // 1. PRIMO TENTATIVO DI COMUNICAZIONE: Leggere WHO_AM_I (Registro 0x75)
    uint8_t who_am_i = 0;

    // a) Invia richiesta di lettura al registro 0x75
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x75);            // WHO_AM_I Register Address
    Wire.endTransmission(false); // Mantieni il bus per la lettura (Repeated Start)

    // b) Leggi 1 byte
    Wire.requestFrom(MPU_ADDR, 1, true);
    if (Wire.available())
    {
        who_am_i = Wire.read();
    }

    // 2. VERIFICA
    if (who_am_i != 0x68 && who_am_i != 0x70)
    {
        Serial.printf("❌ Errore MPU6050 (ID): 0x%X. ID non riconosciuto!\n", who_am_i);
        return;
    }

    Serial.println("✅ MPU6050 ID Corretto (0x68). Procedo con la configurazione.");

    // 3. WAKE UP e configurazione (SOLO SE il sensore è stato riconosciuto)
    i2c_communicate(0x6B, 0x00);

    i2c_communicate(0x19, 0x07); // 1 kHz / (1+7) = 125 Hz

    i2c_communicate(0x1A, 0x03); // DLPF = 3 → BW=44 Hz (stabile per auto)

    i2c_communicate(0x1B, 0x08); // Gyro full scale ±500°/s

    i2c_communicate(0x1C, 0x08); // Accel full scale ±4g

    delay(100);

    // 4. IMU CALIBRATION
    Serial.println("Calibrazione IMU in corso. Tenere fermo il dispositivo...");
    filter.calibrate(); // Calibrazione con 1000 campioni di default
    Serial.println("✅ Calibrazione IMU completata.");

    Serial.println("MPU6050 configurato.");
}

void InitSensors()
{
    // Initialize Temperature/Humidity sensor
    // InitTempHumiditySensor();

    // Initialize IMU sensor here
    InitIMUSensor();

    Serial.println("All sensors initialized.");
}

// ==================================================================
// Read data from sensors and update LiveData structure
// ==================================================================

int16_t ax, ay, az; // Accelerometer raw values
int16_t gx, gy, gz; // Gyroscope raw values

#define DEG_TO_RAD 0.01745329252f

// This function reads temperature from DHT11 sensor
// SHOULD be called with some delay to get accurate readings

int readTemperature()
{
    liveData.InternalTemp = dht.readTemperature();
    return 0;
}

int readHumidity()
{
    liveData.humidity = dht.readHumidity();
    return 0;
}

// This function reads data from the IMU sensor and updates liveData
// It also applies the complementary filter to compute roll and pitch
// and removes gravity component from accelerometer readings
void readIMU()
{
    uint8_t buffer[14];
    i2cReading(0x3B, 14, buffer);

    ax = (buffer[0] << 8) | buffer[1];
    ay = (buffer[2] << 8) | buffer[3];
    az = (buffer[4] << 8) | buffer[5];

    gx = (buffer[8] << 8) | buffer[9];
    gy = (buffer[10] << 8) | buffer[11];
    gz = (buffer[12] << 8) | buffer[13];

    // Convert raw values to 'g'
    float accelX = ((float)ax - filter.ax_offset) / 8192.0f;
    float accelY = ((float)ay - filter.ay_offset) / 8192.0f;
    float accelZ = ((float)az - filter.az_offset) / 8192.0f;

    // Convert raw values to 'rad/s'
    float gyroX = ((float)gx - filter.gx_offset) / 65.5f * DEG_TO_RAD;
    float gyroY = ((float)gy - filter.gy_offset) / 65.5f * DEG_TO_RAD;
    float gyroZ = ((float)gz - filter.gz_offset) / 65.5f * DEG_TO_RAD;

    // Need to update the Roll and Pitch values in filter
    filter.update(accelX * 9.81f, accelY * 9.81f, accelZ * 9.81f, gyroX, gyroY);

    // Remove gravity component
    float ax_lin, ay_lin, az_lin;
    filter.removeGravity(accelX, accelY, accelZ,
                         ax_lin, ay_lin, az_lin);

    accelX = truncf(ax_lin * 10) / 10.0f;
    accelY = truncf(ay_lin * 10) / 10.0f;
    accelZ = truncf(az_lin * 10) / 10.0f;

    if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        liveData.accelX = accelX;
        liveData.accelY = accelY;
        liveData.accelZ = accelZ;

        liveData.gyroX = gyroX;
        liveData.gyroY = gyroY;
        liveData.gyroZ = gyroZ;

        liveData.roll = filter.roll_deg;
        liveData.pitch = filter.pitch_deg;

        xSemaphoreGive(xDataMutex);
    }

    return;
}
