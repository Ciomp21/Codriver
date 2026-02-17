#include <Arduino.h>
#include <unity.h>
#include "common/test_config.hpp"
#include "screen.hpp"

// Compile all test suites into this single test runner.
#include "non_hw/test_non_hardware.cpp"
#include "hw/test_hw_sensors.cpp"
#include "screen/test_screen.cpp"

void setUp(void) {}
void tearDown(void) {}

void setup()
{
    Serial.begin(115200);
    delay(200);

#if CODRIVER_TEST_ENABLE_SCREEN
    // Initialize screen once at startup for display tests
    setupScreen();
    delay(500);
#endif

    UNITY_BEGIN();
    register_non_hw_tests();
    register_hw_tests();
    register_screen_tests();
    UNITY_END();
}

void loop() {}
