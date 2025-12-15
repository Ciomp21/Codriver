#include "sensor.h"

DHT dht(DHTPIN, DHTTYPE);

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

// void InitIMUSensor()
// {

//     Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);    // Initialize I2C with the pins used on ESP32-S3
//     Wire.setClock(100000); // Set I2C frequency to 400kHz

//     // Wake up the IMU sensor (MPU6050) by writing to the PWR_MGMT_1 register
//     i2c_communicate(0x6B, 0x00);

//     i2c_communicate(0x19, 0x07); // 1 kHz / (1+7) = 125 Hz

//     i2c_communicate(0x1A, 0x03); // DLPF = 3 → BW=44 Hz (stabile per auto)

//     i2c_communicate(0x1B, 0x08); // Gyro full scale ±500°/s

//     i2c_communicate(0x1C, 0x08); // Accel full scale ±4g

//     Serial.println("MPU6050 configured");
// }

void InitIMUSensor()
{
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000); 

    // 1. PRIMO TENTATIVO DI COMUNICAZIONE: Leggere WHO_AM_I (Registro 0x75)
    uint8_t who_am_i = 0;
    
    // a) Invia richiesta di lettura al registro 0x75
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x75); // WHO_AM_I Register Address
    Wire.endTransmission(false); // Mantieni il bus per la lettura (Repeated Start)
    
    // b) Leggi 1 byte
    Wire.requestFrom(MPU_ADDR, 1, true); 
    if (Wire.available()) {
        who_am_i = Wire.read();
    }

    // 2. VERIFICA
    if (who_am_i != 0x68 && who_am_i != 0x70) {
        Serial.printf("❌ Errore MPU6050 (ID): 0x%X. ID non riconosciuto!\n", who_am_i);
        return; 
    }
    
    Serial.println("✅ MPU6050 ID Corretto (0x68). Procedo con la configurazione.");

    // 3. WAKE UP e configurazione (SOLO SE il sensore è stato riconosciuto)
    i2c_communicate(0x6B, 0x00); // Wake up
    i2c_communicate(0x19, 0x07); // Sample Rate
    // ... tutte le altre configurazioni ...
    
    Serial.println("MPU6050 configurato.");
}



void InitSensors()
{
    // Initialize Temperature/Humidity sensor
    // InitTempHumiditySensor();

    // Initialize IMU sensor here
    InitIMUSensor();
}

// ==================================================================
// Read data from sensors and update LiveData structure
// ==================================================================

int16_t ax, ay, az; // Accelerometer raw values
int16_t gx, gy, gz; // Gyroscope raw values

// This function reads temperature from DHT11 sensor
// SHOULD be called with some delay to get accurate readings

float ReadTemperature()
{
    return dht.readTemperature();
}

float readHumidity()
{
    return dht.readHumidity();
}

float ReadAcceleration()
{
    // Read IMU data and update data->accelX, data->accelY, data->accelZ, etc.

    uint8_t buffer[14];
    i2cReading(0x3B, 14, buffer);

    ax = (buffer[0] << 8) | buffer[1];
    ay = (buffer[2] << 8) | buffer[3];
    az = (buffer[4] << 8) | buffer[5];

    // Convert raw values to 'g' and '°/s' based on full scale settings
    float accelX = truncf((float)ax / 16384.0f * 10) / 10.0f;
    float accelY = truncf((float)ay / 16384.0f * 10) / 10.0f;
    float accelZ = truncf((float)az / 16384.0f * 10) / 10.0f;

    Serial.print("Accel X: ");
    Serial.print(accelX);
    Serial.print(" g, Y: ");
    Serial.print(accelY);
    Serial.print(" g, Z: ");
    Serial.print(accelZ);
    Serial.println(" g");

    float value = accelX * 100000.0 + accelY * 1000.0 + accelZ * 10.0;
    // Serial.print(value);
    // Serial.println(" totale");

    return value;
}

float ReadIncline()
{
  
}