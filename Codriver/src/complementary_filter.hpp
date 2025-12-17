#pragma once
#include <math.h>
#include <Arduino.h>
#include "sensor.h"

class ComplementaryFilter
{

private:
    float roll = 0.0f;  // in radians
    float pitch = 0.0f; // in radians
    unsigned long last_update_time = 0;

public:
    float roll_deg = 0;
    float pitch_deg = 0;

    float ax_offset = 0.0f;
    float ay_offset = 0.0f;
    float az_offset = 0.0f;

    float gx_offset = 0.0f;
    float gy_offset = 0.0f;
    float gz_offset = 0.0f;

    float alpha = 0.98f; // gyro weight

    void calibrate(int samples = 1000)
    {
        float ax_sum = 0.0f, ay_sum = 0.0f, az_sum = 0.0f;
        float gx_sum = 0.0f, gy_sum = 0.0f, gz_sum = 0.0f;

        for (int i = 0; i < samples; i++)
        {
            uint8_t buffer[14];
            i2cReading(0x3B, 14, buffer);

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

        ax_offset = ax_sum / samples;
        ay_offset = ay_sum / samples;
        az_offset = az_sum / samples - 8192.0f; // assuming static position, so Z should read 1g

        gx_offset = gx_sum / samples;
        gy_offset = gy_sum / samples;
        gz_offset = gz_sum / samples;
    }

    // Update the filter with new accelerometer and gyroscope data
    // ax, ay, az are in m/s²
    // gx, gy are in rad/s
    void update(float ax, float ay, float az,
                float gx, float gy)
    {

        // Accelerometer angles
        float roll_acc = atan2(ay, az);
        float pitch_acc = atan2(-ax, sqrt(ay * ay + az * az));

        // Time delta
        unsigned long now = micros();
        float dt = (now - last_update_time) * 1e-6f;
        last_update_time = now;

        // Gyro integration
        roll += gx * dt;
        pitch += gy * dt;

        // Complementary fusion
        roll = alpha * roll + (1.0f - alpha) * roll_acc;
        pitch = alpha * pitch + (1.0f - alpha) * pitch_acc;

        // Update degrees
        roll_deg = roll * (180.0f / 3.14159f);
        pitch_deg = pitch * (180.0f / 3.14159f);
    }

    // Remove gravity component from accelerometer readings
    // ax, ay, az are in m/s²
    // Outputs passed as memory locations in ax_lin, ay_lin, az_lin are linear accelerations
    void removeGravity(float ax, float ay, float az,
                       float &ax_lin, float &ay_lin, float &az_lin)
    {
        const float g = 9.81f;

        ax_lin = ax - g * sin(pitch);
        ay_lin = ay + g * sin(roll) * cos(pitch);
        az_lin = az - g * cos(roll) * cos(pitch);
    }
};