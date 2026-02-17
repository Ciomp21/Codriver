#include <Arduino.h>
#include <unity.h>

#include "../common/test_config.hpp"
#include "global.hpp"
#include "pitch_roll.hpp"

static void ensure_mutexes()
{
    if (!xUIMutex)
    {
        xUIMutex = xSemaphoreCreateMutex();
    }
    if (!xDataMutex)
    {
        xDataMutex = xSemaphoreCreateMutex();
    }
}

static void test_rpm_conversion()
{
    rpm(4000);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1000.0f, liveData.rpm);
}

static void test_boost_clamp()
{
    boost(0);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, liveData.boost);

    boost(150);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, liveData.boost);
}

static void test_coolant_temp()
{
    coolant_temp(60);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 20.0f, liveData.coolantTemp);
}

static void test_engine_load()
{
    engine_load(128);
    TEST_ASSERT_FLOAT_WITHIN(0.2f, 50.2f, liveData.engineLoad);
}

static void test_battery_voltage()
{
    battery_voltage(12345);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.345f, liveData.batteryVoltage);
}

static void test_error_state()
{
    ensure_mutexes();

    err = 0;
    ui_update = false;

    setError(1);
    TEST_ASSERT_EQUAL(1, getError());
    TEST_ASSERT_TRUE(ui_update);

    resolveError(1);
    TEST_ASSERT_EQUAL(0, getError());
}

static void test_complementary_filter_stable()
{
    ComplementaryFilter filter;
    // Simula 1000 letture a riposo (acceleroemtro vicino a 0,0,9.81 e giroscopio vicino a 0)
    for (int i = 0; i < 1000; i++)
    {
        float NegativeORPositiveRandom = (rand() % 2) == 0 ? -1.0f : 1.0f;

        float accel_x = ((rand() % 10) / 100.0f) * NegativeORPositiveRandom;
        float accel_y = ((rand() % 10) / 100.0f) * NegativeORPositiveRandom;
        float accel_z = 9.81f + ((rand() % 10) / 100.0f) * NegativeORPositiveRandom;
        float gyro_x = ((rand() % 10) / 100.0f) * NegativeORPositiveRandom;
        float gyro_y = ((rand() % 10) / 100.0f) * NegativeORPositiveRandom;

        filter.update(accel_x, accel_y, accel_z, gyro_x, gyro_y);
        delay(2);
    }

    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, filter.roll_deg);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 0.0f, filter.pitch_deg);
}

static void test_complementary_filter_running()
{
    ComplementaryFilter filter;

    const float g = 9.81f;
    const float dt = 0.01f;
    const int samples = 500;

    const float roll_amp = 5.0f * (PI / 180.0f);
    const float pitch_amp = 3.0f * (PI / 180.0f);
    const float roll_w = 2.0f * PI * 0.2f;
    const float pitch_w = 2.0f * PI * 0.15f;
    const float pitch_phase = 0.5f;

    for (int i = 0; i < samples; i++)
    {
        float t = i * dt;

        float roll = roll_amp * sinf(roll_w * t);
        float pitch = pitch_amp * sinf(pitch_w * t + pitch_phase);

        float ax = g * sinf(pitch);
        float ay = -g * sinf(roll) * cosf(pitch);
        float az = g * cosf(roll) * cosf(pitch);

        ax += 0.2f * sinf(2.0f * PI * 0.5f * t);
        ay += 0.1f * sinf(2.0f * PI * 0.3f * t);

        float gx = roll_amp * roll_w * cosf(roll_w * t);
        float gy = pitch_amp * pitch_w * cosf(pitch_w * t + pitch_phase);

        ax += 0.02f * sinf(2.0f * PI * 1.3f * t);
        ay += 0.02f * cosf(2.0f * PI * 1.1f * t);
        gx += 0.002f * sinf(2.0f * PI * 0.9f * t);
        gy += 0.002f * cosf(2.0f * PI * 0.7f * t);

        filter.update(ax, ay, az, gx, gy);
        delay(10);
    }

    float t_end = (samples - 1) * dt;
    float expected_pitch_deg = (pitch_amp * sinf(pitch_w * t_end + pitch_phase)) * 57.2958f;

    TEST_ASSERT_FLOAT_WITHIN(2.0f, 0.0f, filter.roll_deg);
    TEST_ASSERT_FLOAT_WITHIN(5.0f, expected_pitch_deg, filter.pitch_deg);
}

void register_non_hw_tests()
{
#if CODRIVER_TEST_ENABLE_NON_HW
    RUN_TEST(test_rpm_conversion);
    RUN_TEST(test_boost_clamp);
    RUN_TEST(test_coolant_temp);
    RUN_TEST(test_engine_load);
    RUN_TEST(test_battery_voltage);
    RUN_TEST(test_error_state);
    RUN_TEST(test_complementary_filter_stable);
    RUN_TEST(test_complementary_filter_running);
#else
    Serial.println("Non-hardware tests disabled.");
#endif
}
