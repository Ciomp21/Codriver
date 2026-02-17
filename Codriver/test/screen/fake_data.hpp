#pragma once

#include "global.hpp"
#include <math.h>

/**
 * Fake data generator for testing screen displays
 * Provides functions to inject realistic test data into liveData
 */

class FakeDataGenerator
{
private:
    static float sine_wave(float phase, float min, float max)
    {
        return min + (max - min) * (0.5f + 0.5f * sin(phase));
    }

    static float triangle_wave(float phase, float min, float max)
    {
        phase = fmod(phase, 2.0f * M_PI);
        float normalized = phase / M_PI;
        if (normalized < 1.0f)
            return min + (max - min) * normalized;
        else
            return max - (max - min) * (normalized - 1.0f);
    }

public:
    // Generate RPM data (0-8000)
    static void set_rpm(float value)
    {
        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            liveData.rpm = value;
            xSemaphoreGive(xDataMutex);
        }
    }

    // Generate Boost data (-0.3 - 2.0 bar)
    static void set_boost(float value)
    {
        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            liveData.boost = value;
            xSemaphoreGive(xDataMutex);
        }
    }

    // Generate Coolant Temperature (0-120°C)
    static void set_coolant_temp(float value)
    {
        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            liveData.coolantTemp = value;
            xSemaphoreGive(xDataMutex);
        }
    }

    // Generate Engine Load (0-100%)
    static void set_engine_load(float value)
    {
        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            liveData.engineLoad = value;
            xSemaphoreGive(xDataMutex);
        }
    }

    // Generate Battery Voltage (9 - 16V)
    static void set_battery_voltage(float value)
    {
        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            liveData.batteryVoltage = value;
            xSemaphoreGive(xDataMutex);
        }
    }

    // Generate Acceleration data (G-forces)
    static void set_acceleration(float ax, float ay, float az)
    {
        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            liveData.accelX = ax;
            liveData.accelY = ay;
            liveData.accelZ = az;
            xSemaphoreGive(xDataMutex);
        }
    }

    // Generate Roll angle (-90 to 90 degrees)
    static void set_roll(float value)
    {
        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            liveData.roll = value;
            xSemaphoreGive(xDataMutex);
        }
    }

    // Generate Pitch angle (-90 to 90 degrees)
    static void set_pitch(float value)
    {
        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            liveData.pitch = value;
            xSemaphoreGive(xDataMutex);
        }
    }

    // Generate Temperature & Humidity
    static void set_internal_temp(float value)
    {
        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            liveData.InternalTemp = value;
            xSemaphoreGive(xDataMutex);
        }
    }

    static void set_humidity(float value)
    {
        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            liveData.humidity = value;
            xSemaphoreGive(xDataMutex);
        }
    }

    // Generate sinusoidal RPM data (useful for animation tests)
    static void set_rpm_sine(float phase, float min = 0.0f, float max = 8000.0f)
    {
        set_rpm(sine_wave(phase, min, max));
    }

    // Generate sinusoidal Boost data
    static void set_boost_sine(float phase, float min = -0.3f, float max = 2.0f)
    {
        set_boost(sine_wave(phase, min, max));
    }

    // Generate triangle wave RPM (acceleration/deceleration simulation)
    static void set_rpm_triangle(float phase, float min = 0.0f, float max = 8000.0f)
    {
        set_rpm(triangle_wave(phase, min, max));
    }

    // Generate realistic acceleration pattern (lateral G-forces during cornering)
    static void set_cornering_force(float phase, float max_g = 1.5f)
    {
        float ax = max_g * sin(phase);
        float ay = max_g * cos(phase);
        float az = 9.81f; // Gravity
        set_acceleration(ax, ay, az);
    }

    // Generate braking pattern (longitudinal deceleration)
    static void set_braking_pattern(float intensity = 0.8f)
    {
        float ax = -9.81f * intensity;
        set_acceleration(ax, 0.0f, 9.81f);
    }

    // Generate acceleration pattern
    static void set_acceleration_pattern(float intensity = 0.6f)
    {
        float ax = 9.81f * intensity;
        set_acceleration(ax, 0.0f, 9.81f);
    }

    // Initialize all data to safe defaults
    static void reset_all()
    {
        if (xSemaphoreTake(xDataMutex, pdMS_TO_TICKS(10)) == pdTRUE)
        {
            liveData.rpm = 0.0f;
            liveData.boost = 0.0f;
            liveData.coolantTemp = 20.0f;
            liveData.engineLoad = 0.0f;
            liveData.batteryVoltage = 12.0f;
            liveData.accelX = 0.0f;
            liveData.accelY = 0.0f;
            liveData.accelZ = 9.81f;
            liveData.roll = 0.0f;
            liveData.pitch = 0.0f;
            liveData.InternalTemp = 25.0f;
            liveData.humidity = 50.0f;
            xSemaphoreGive(xDataMutex);
        }
    }
};
