#include <Arduino.h>
#include <unity.h>
#include <math.h>

#include "../common/test_config.hpp"
#include "global.hpp"
#include "screen.hpp"
#include "fake_data.hpp"

// ========================== SETUP ==========================

static void ensure_mutexes_screen()
{
    if (!xUIMutex)
        xUIMutex = xSemaphoreCreateMutex();
    if (!xDataMutex)
        xDataMutex = xSemaphoreCreateMutex();
}

// ========================== RPM SCREEN TEST ==========================

static void test_rpm_screen()
{
    ensure_mutexes_screen();
    FakeDataGenerator::reset_all();

    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        ui_index = 2; // RPM screen
        ui_update = true;
        xSemaphoreGive(xUIMutex);
    }

    // Animate RPM from 0 to 7000
    for (int i = 0; i <= 70; i += 5)
    {
        FakeDataGenerator::set_rpm(i * 100.0f);
        drawRPM();
        delay(100);
    }

    TEST_ASSERT_EQUAL(7000.0f, liveData.rpm);
}

// ========================== BOOST SCREEN TEST ==========================

static void test_boost_screen()
{
    ensure_mutexes_screen();
    FakeDataGenerator::reset_all();

    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        ui_index = 1; // Boost screen
        ui_update = true;
        xSemaphoreGive(xUIMutex);
    }

    drawScreen();

    // Animate boost from -0.3 to 2.0
    for (float boost = -0.3f; boost <= 2.0f; boost += 0.15f)
    {
        FakeDataGenerator::set_boost(boost);
        drawBoost();
        delay(100);
    }

    TEST_ASSERT_FLOAT_WITHIN(0.2f, 2.0f, liveData.boost);
}

// ========================== G-FORCE SCREEN TEST ==========================

static void test_gforce_screen()
{
    ensure_mutexes_screen();
    FakeDataGenerator::reset_all();

    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        ui_index = 3; // G-Force screen
        ui_update = true;
        xSemaphoreGive(xUIMutex);
    }

    drawScreen();

    // Test different G-force scenarios
    Serial.println("Test: Static (0G lateral)");
    FakeDataGenerator::set_acceleration(0.0f, 0.0f, 9.81f);
    drawAcceleration();
    delay(1000);

    Serial.println("Test: Cornering (1.5G lateral)");
    for (int i = 0; i < 360; i += 45)
    {
        FakeDataGenerator::set_cornering_force(i * M_PI / 180.0f, 1.5f);
        drawAcceleration();
        delay(200);
    }

    Serial.println("Test: Braking (0.9G decel)");
    FakeDataGenerator::set_braking_pattern(0.9f);
    drawAcceleration();
    delay(1000);

    Serial.println("Test: Acceleration (0.7G accel)");
    FakeDataGenerator::set_acceleration_pattern(0.7f);
    drawAcceleration();
    delay(1000);

    TEST_ASSERT_TRUE(isfinite(liveData.accelX));
}

// ========================== TEMPERATURE SCREEN TEST ==========================

static void test_temperature_screen()
{
    ensure_mutexes_screen();
    FakeDataGenerator::reset_all();

    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        ui_index = 4; // Coolant temp screen
        ui_update = true;
        xSemaphoreGive(xUIMutex);
    }

    drawScreen();

    // Simulate engine warmup
    for (float temp = 20.0f; temp <= 120.0f; temp += 10.0f)
    {
        FakeDataGenerator::set_coolant_temp(temp);
        drawTemperature();
        delay(150);
    }

    TEST_ASSERT_EQUAL(120.0f, liveData.coolantTemp);
}

// ========================== AIR TEMPERATURE SCREEN TEST ==========================

static void test_air_temperature_screen()
{
    ensure_mutexes_screen();
    FakeDataGenerator::reset_all();

    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        ui_index = 5; // Air temp screen
        ui_update = true;
        xSemaphoreGive(xUIMutex);
    }

    drawScreen();

    for (float temp = 10.0f; temp <= 50.0f; temp += 5.0f)
    {
        FakeDataGenerator::set_internal_temp(temp);
        drawAirTemperature();
        delay(150);
    }

    TEST_ASSERT_EQUAL(50.0f, liveData.InternalTemp);
}

// ========================== BATTERY SCREEN TEST ==========================

static void test_battery_screen()
{
    ensure_mutexes_screen();
    FakeDataGenerator::reset_all();

    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        ui_index = 6; // Battery screen
        ui_update = true;
        xSemaphoreGive(xUIMutex);
    }

    drawScreen();

    for (float volt = 10.0f; volt <= 15.0f; volt += 0.5f)
    {
        FakeDataGenerator::set_battery_voltage(volt);
        drawBattery();
        delay(150);
    }

    TEST_ASSERT_EQUAL(15.0f, liveData.batteryVoltage);
}

// ========================== ROLL SCREEN TEST ==========================

static void test_roll_screen()
{
    ensure_mutexes_screen();
    FakeDataGenerator::reset_all();

    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        ui_index = 7; // Roll screen
        ui_update = true;
        xSemaphoreGive(xUIMutex);
    }

    drawScreen();

    // Simulate left turn
    for (float roll = 0.0f; roll >= -30.0f; roll -= 3.0f)
    {
        FakeDataGenerator::set_roll(roll);
        drawRoll();
        delay(100);
    }

    // Return to center
    for (float roll = -30.0f; roll <= 30.0f; roll += 3.0f)
    {
        FakeDataGenerator::set_roll(roll);
        drawRoll();
        delay(100);
    }

    // Left turn
    for (float roll = 30.0f; roll >= 0.0f; roll -= 3.0f)
    {
        FakeDataGenerator::set_roll(roll);
        drawRoll();
        delay(100);
    }

    TEST_ASSERT_TRUE(isfinite(liveData.roll));
}

// ========================== PITCH SCREEN TEST ==========================

static void test_pitch_screen()
{
    ensure_mutexes_screen();
    FakeDataGenerator::reset_all();

    if (xSemaphoreTake(xUIMutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        ui_index = 8; // Pitch screen
        ui_update = true;
        xSemaphoreGive(xUIMutex);
    }

    drawScreen();

    // Simulate acceleration (nose up)
    for (float pitch = 0.0f; pitch <= 15.0f; pitch += 1.5f)
    {
        FakeDataGenerator::set_pitch(pitch);
        drawPitch();
        delay(100);
    }

    // Return to level
    for (float pitch = 15.0f; pitch >= -20.0f; pitch -= 1.5f)
    {
        FakeDataGenerator::set_pitch(pitch);
        drawPitch();
        delay(100);
    }

    // Return to level
    for (float pitch = -20.0f; pitch <= 0.0f; pitch += 1.5f)
    {
        FakeDataGenerator::set_pitch(pitch);
        drawPitch();
        delay(100);
    }

    TEST_ASSERT_TRUE(isfinite(liveData.pitch));
}

// ========================== TEST REGISTRATION ==========================

void register_screen_tests()
{
#if CODRIVER_TEST_ENABLE_SCREEN
    Serial.println("=== Screen Display Tests ===");
    Serial.println("Each test displays animations on the screen.");
    Serial.println();

    setupScreen();
    delay(1000);

    RUN_TEST(test_rpm_screen);
    RUN_TEST(test_boost_screen);
    RUN_TEST(test_gforce_screen);
    RUN_TEST(test_temperature_screen);
    RUN_TEST(test_air_temperature_screen);
    RUN_TEST(test_battery_screen);
    RUN_TEST(test_roll_screen);
    RUN_TEST(test_pitch_screen);
#else
    Serial.println("Screen tests disabled.");
#endif
}
