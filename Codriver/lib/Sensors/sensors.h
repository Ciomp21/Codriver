#pragma once
#include "livedata.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTTYPE    DHT11
#define DHTPIN     10  // GPIO pin where the DHT11 is connected

void ReadSensorData(LiveData *data);
void InitSensors();