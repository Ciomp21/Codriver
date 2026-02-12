#include "sensor.h"
#include "global.hpp"
#include "complementary_filter.hpp"

DHT dht(DHTPIN, DHTTYPE);
ComplementaryFilter filter;
static unsigned long lastDhtReadMs = 0;
static float lastTempC = NAN;
static float lastHumidity = NAN;
static const unsigned long kDhtMinIntervalMs = 2000;

void i2c_communicate(uint8_t reg, uint8_t data)
{
    // Starts I2C transmission to the MPU6050
    // Write the register address and the data byte
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
}

// Returns 0 on success, -1 on I2C error
int i2cReading(uint8_t reg, uint8_t len, uint8_t *buffer)
{
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);

    // requestFrom returns the number of bytes received, or 0 on error
    size_t bytesRead = Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)len, true);

    if (bytesRead != len)
    {
        Serial.printf("I2C Error: Expected %d bytes, got %d bytes\n", len, bytesRead);
        return -1; // I2C error occurred
    }

    for (int i = 0; i < len; i++)
        buffer[i] = Wire.read();

    return 0; // Success
}

void InitTempHumiditySensor()
{
    dht.begin();

    // Print temperature as proof.
    Serial.println(F("DHT11 sensor test"));

    Serial.print(F("Temperature: "));
    Serial.println(dht.readTemperature(false, true));

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

    // ✅ SAMPLE RATE: 100 Hz (ottimo compromesso auto)
    i2c_communicate(0x19, 0x09); // 1000Hz / (1+9) = 100 Hz

    // ✅ DLPF (Low Pass Filter): Bandwidth 20 Hz
    i2c_communicate(0x1A, 0x04); // Filtra vibrazioni motore/strada

    // ✅ GYRO: ±250°/s (sufficiente per auto normale)
    i2c_communicate(0x1B, 0x00); // Se fai track-day → 0x08 (±500°/s)

    // ✅ ACCEL: ±8g (copre frenate/accelerazioni brusche)
    i2c_communicate(0x1C, 0x10); // ±8g = 4096 LSB/g

    delay(1000);

    // 4. IMU CALIBRATION
    Serial.println("Calibrazione IMU in corso. Tenere fermo il dispositivo...");
    filter.calibrate(); // Calibrazione con 1000 campioni di default
    Serial.println("✅ Calibrazione IMU completata.");

    Serial.println("MPU6050 configurato.");
}

void InitSensors()
{
    // Initialize Temperature/Humidity sensor
    InitTempHumiditySensor();

    // Initialize IMU sensor here
    InitIMUSensor();

    Serial.println("All sensors initialized.");
}

// ==================================================================
// Read data from sensors and update LiveData structure
// ==================================================================

#define DEG_TO_RAD 0.01745329252f

unsigned long lastIMUPrintTime = 0;

// This function reads temperature from DHT11 sensor
// SHOULD be called with some delay to get accurate readings

int readTemperature()
{
    unsigned long now = millis();
    if (now - lastDhtReadMs < kDhtMinIntervalMs && !isnan(lastTempC))
    {
        liveData.InternalTemp = lastTempC;
        return 0;
    }

    float temp = dht.readTemperature();
    if (isnan(temp))
    {
        Serial.println("⚠️  Failed to read from DHT sensor!");
        return -1;
    }

    lastDhtReadMs = now;
    lastTempC = temp;
    liveData.InternalTemp = temp;
    Serial.println("Temperature: " + String(temp) + "°C");
    return 0;
}

int readHumidity()
{
    unsigned long now = millis();
    if (now - lastDhtReadMs < kDhtMinIntervalMs && !isnan(lastHumidity))
    {
        liveData.humidity = lastHumidity;
        return 0;
    }

    float humidity = dht.readHumidity();
    if (isnan(humidity))
    {
        Serial.println("⚠️  Failed to read humidity from DHT sensor!");
        return -1;
    }

    lastDhtReadMs = now;
    lastHumidity = humidity;
    liveData.humidity = humidity;
    return 0;
}

// This function reads data from the IMU sensor and updates liveData
// It also applies the complementary filter to compute roll and pitch
// and removes gravity component from accelerometer readings
void readIMU()
{
    uint8_t buffer[14];
    int16_t ax, ay, az; // Accelerometer raw values
    int16_t gx, gy, gz; // Gyroscope raw values

    // Check if I2C reading was successful
    if (i2cReading(0x3B, 14, buffer) != 0)
    {
        // Serial.println("⚠️  Skipping IMU reading due to I2C error");
        return; // Skip this reading if I2C failed
    }

    ax = (buffer[0] << 8) | buffer[1];
    ay = (buffer[2] << 8) | buffer[3];
    az = (buffer[4] << 8) | buffer[5];

    gx = (buffer[8] << 8) | buffer[9];
    gy = (buffer[10] << 8) | buffer[11];
    gz = (buffer[12] << 8) | buffer[13];

    // Convert raw values to 'g'
    float accelX = ((float)ax - filter.ax_offset) / 4096.0f;
    float accelY = ((float)ay - filter.ay_offset) / 4096.0f;
    float accelZ = ((float)az - filter.az_offset) / 4096.0f;

    // Convert raw values to 'rad/s'
    float gyroX = ((float)gx - filter.gx_offset) / 131.0f * DEG_TO_RAD;
    float gyroY = ((float)gy - filter.gy_offset) / 131.0f * DEG_TO_RAD;
    float gyroZ = ((float)gz - filter.gz_offset) / 131.0f * DEG_TO_RAD;

    // Need to update the Roll and Pitch values in filter
    // Convert accelerometer from 'g' to 'm/s²'
    filter.update(accelX * 9.81f, accelY * 9.81f, accelZ * 9.81f, gyroX, gyroY);

    // Remove gravity component
    // filter.removeGravity(accelX, accelY, accelZ);

    unsigned long currentTime = micros();

    if (currentTime - lastIMUPrintTime >= 3000)
    {
        lastIMUPrintTime = currentTime;
        // Serial.printf("Roll: %.0f°, Pitch: %.0f°\n", filter.roll_deg, filter.pitch_deg);
        // Serial.printf("Lin Accel -> X: %.2f m/s², Y: %.2f m/s², Z: %.2f m/s²\n", accelX, accelY, accelZ);
    }

    if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        liveData.accelX = accelX;
        liveData.accelY = accelY;
        liveData.accelZ = accelZ;

        liveData.roll = filter.roll_deg;
        liveData.pitch = filter.pitch_deg;

        xSemaphoreGive(xDataMutex);
    }

    return;
}
