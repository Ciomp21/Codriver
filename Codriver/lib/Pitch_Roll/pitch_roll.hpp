#pragma once
#include <math.h>
#include <sensor.hpp>

class ComplementaryFilter
{

private:
    float roll = 0.0f;  // in radians
    float pitch = 0.0f; // in radians
    unsigned long last_update_time = 0;

    float alpha = 0.96f; // Dai più peso all'accelerometro

public:
    float roll_deg = 0;
    float pitch_deg = 0;

    float ax_offset = 0.0f;
    float ay_offset = 0.0f;
    float az_offset = 0.0f;

    float gx_offset = 0.0f;
    float gy_offset = 0.0f;
    float gz_offset = 0.0f;

    void calibrate(int samples = 1000)
    {
        float ax_sum = 0.0f, ay_sum = 0.0f, az_sum = 0.0f;
        float gx_sum = 0.0f, gy_sum = 0.0f, gz_sum = 0.0f;

        for (int i = 0; i < samples; i++)
        {
            uint8_t buffer[14];
            if (i2cReading(0x3B, 14, buffer) != 0)
            {
                Serial.println("⚠️  Skipping IMU reading due to I2C error");
                return; // Skip this reading if I2C failed
            }

            int16_t ax = (buffer[0] << 8) | buffer[1];
            int16_t ay = (buffer[2] << 8) | buffer[3];
            int16_t az = (buffer[4] << 8) | buffer[5];

            int16_t gx = (buffer[8] << 8) | buffer[9];
            int16_t gy = (buffer[10] << 8) | buffer[11];
            int16_t gz = (buffer[12] << 8) | buffer[13];

            ax_sum += ax;
            ay_sum += ay;
            az_sum += az;

            gx_sum += gx;
            gy_sum += gy;
            gz_sum += gz;

            delay(2);
        }

        ax_offset = -(ax_sum / samples);
        ay_offset = -(az_sum / samples);
        az_offset = -(ay_sum / samples);

        gx_offset = -(gx_sum / samples);
        gy_offset = -(gz_sum / samples);
        gz_offset = -(gy_sum / samples);
    }

    // Update the filter with new accelerometer and gyroscope data
    // ax, ay, az are in m/s²
    // gx, gy are in rad/s
    void update(float ax, float ay, float az, float gx, float gy)
    {
        // Accelerometer angles (reference assoluta rispetto alla gravità)
        float roll_acc = atan2(ay, az);
        float pitch_acc = atan2(-ax, sqrt(ay * ay + az * az));

        // Time delta
        unsigned long now = micros();
        float dt = (now - last_update_time) * 1e-6f;
        last_update_time = now;

        // Prima volta? Inizializza con accelerometro
        if (dt > 1.0f || dt <= 0.0f)
        {
            roll = roll_acc;
            pitch = pitch_acc;
            last_update_time = now;
            roll_deg = roll * 57.2958f;
            pitch_deg = pitch * 57.2958f;
            return;
        }

        // Gyro integration
        roll += gx * dt;
        pitch += gy * dt;

        // Rileva accelerazioni dinamiche del veicolo (accelerazione/frenata/curva)
        float accel_magnitude = sqrt(ax * ax + ay * ay + az * az);
        float dynamic_accel = fabs(accel_magnitude - 9.81f);

        // Rileva rotazioni veloci
        float gyro_activity = fabs(gx) + fabs(gy);

        float adaptive_alpha;

        if (dynamic_accel > 3.0f || gyro_activity > 0.2f)
        {
            // Movimento dinamico forte → più fiducia al giroscopio
            adaptive_alpha = 0.98f;
        }
        else if (dynamic_accel > 1.0f)
        {
            // Movimento moderato → bilanciato
            adaptive_alpha = 0.96f;
        }
        else
        {
            // Quasi fermo → più fiducia all'accelerometro per correggere drift
            adaptive_alpha = 0.92f;
        }

        // Complementary fusion
        roll = adaptive_alpha * roll + (1.0f - adaptive_alpha) * roll_acc;
        pitch = adaptive_alpha * pitch + (1.0f - adaptive_alpha) * pitch_acc;

        // Update degrees
        roll_deg = roll * 57.2958f;
        pitch_deg = pitch * 57.2958f;
    }

    // Remove gravity component from accelerometer readings
    // ax, ay, az are in m/s²
    void removeGravity(float &ax, float &ay, float &az)
    {
        const float g = 9.81f; // Assuming angles in radians

        // Gravity components in body frame
        float gx = g * sin(pitch);
        float gy = -g * sin(roll) * cos(pitch);
        float gz = g * cos(roll) * cos(pitch);

        // Remove gravity to get true linear acceleration
        ax = ax - gx;
        ay = ay - gy;
        az = az - gz;

        // Serial.printf("Gravity -> X: %.2f m/s², Y: %.2f m/s², Z: %.2f m/s²\n", gx, gy, gz);
    }
};