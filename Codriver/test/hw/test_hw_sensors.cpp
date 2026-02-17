#include <Arduino.h>
#include <unity.h>
#include <math.h>

#include "../common/test_config.hpp"
#include "global.hpp"
#include "sensor.hpp"
#include "bleconnection.hpp"

static void ensure_data_mutex()
{
    if (!xDataMutex)
    {
        xDataMutex = xSemaphoreCreateMutex();
    }
}

static void OBDII_Test()
{
    setupWifi();

    int cycleCounter = 0;
    int errorCounter = 0;

    int ret = 0;

    while (cycleCounter < 100)
    {

        // Logica Round-Robin (3 step)
        if (cycleCounter % 3 == 0)
        {
            ret = sendOBDCommand(PID_BOOST);
            TEST_ASSERT_TRUE(isfinite(liveData.boost));
        }
        else if (cycleCounter % 3 == 1)
        {
            ret = sendOBDCommand(PID_RPM);
            TEST_ASSERT_TRUE(isfinite(liveData.rpm));
        }
        else
        {
            // Gestione dei PID rimanenti a rotazione
            switch (cycleCounter)
            {
            case 2:
                ret = sendOBDCommand(PID_COOLANT_TEMP);
                TEST_ASSERT_TRUE(isfinite(liveData.coolantTemp));
                break;
            case 5:
                ret = sendOBDCommand(PID_ENGINE_LOAD);
                TEST_ASSERT_TRUE(isfinite(liveData.engineLoad));
                break;
            case 8:
                ret = sendOBDCommand(PID_BATTERY_VOLTAGE);
                TEST_ASSERT_TRUE(isfinite(liveData.batteryVoltage));
                break;
            }
        }

        if (ret == -1)
        {
            errorCounter++;
        }

        cycleCounter++;
    }

    TEST_ASSERT_LESS_OR_EQUAL_INT(10, errorCounter);

    delay(20);
}

static void test_sensors()
{
    ensure_data_mutex();

    InitSensors();

    TEST_ASSERT_INT_WITHIN(-10, 45, readTemperature());
    TEST_ASSERT_INT_WITHIN(10, 65, readHumidity());

    for (int i = 0; i < 1000; i++)
    {
        readIMU();
        TEST_ASSERT_TRUE(isfinite(liveData.accelX));
        TEST_ASSERT_TRUE(isfinite(liveData.accelY));
        TEST_ASSERT_TRUE(isfinite(liveData.accelZ));

        TEST_ASSERT_TRUE(isfinite(liveData.roll));
        TEST_ASSERT_TRUE(isfinite(liveData.pitch));
    }
}

static void BLE_Test()
{
    setupBLE();
    TEST_ASSERT_TRUE(true);

    // Simulare la ricezione di un messaggio BLE chiamando direttamente la funzione di parsing con un JSON di test.
    parseJson("{\"r\":255,\"g\":0,\"b\":0,\"id\":1,\"min\":0,\"max\":100}");
    TEST_ASSERT(ui_color == 0xF800); // Verifica che il colore sia stato aggiornato correttamente
    TEST_ASSERT(ui_index == 1);      // Verifica che l'indice della UI sia stato aggiornato correttamente
    TEST_ASSERT_TRUE(ui_update);

    delay(10000);
}

void register_hw_tests()
{
#if CODRIVER_TEST_ENABLE_HW
#if CODRIVER_TEST_ENABLE_HW_SENSORS
    RUN_TEST(test_sensors);
#else
    Serial.println("Hardware sensor tests disabled.");
#endif
#if CODRIVER_TEST_ENABLE_OBDII
    RUN_TEST(OBDII_Test);
#else
    Serial.println("OBD-II tests disabled.");
#endif
    RUN_TEST(BLE_Test);
#else
    Serial.println("Hardware sensor tests disabled.");
#endif
}
