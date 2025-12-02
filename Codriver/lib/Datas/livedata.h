#pragma once

#include <Arduino.h>

#define delayTrip 500

struct Values
{
    // Raw OBD2 data
    float speed;
    float rpm;
    float engineTemp;
    float batteryVoltage;
    float oilPressure;
    float coolantTemp;
    float intakeAirTemp;
    float maf;
    float throttlePosition;
    float fuelLevel;
    int gear;   
    float steerAngle;
    float brakePressure;

    // Temperature/Humidity sensor data
    float InternalTemp;
    float ExternalTemp;
    float humidity;

    // IMU sensor data
    float accelX;
    float accelY;
    float accelZ;
    float gyroX;
    float gyroY;
    float gyroZ;

    // ======= Derived Data =======

    // Normalized and computed values
    float fuelConsumption; // in L/100km
    float engineLoad;      // in percentage
    int gearSuggestion;    // suggested gear for optimal fuel efficiency
    int extimatedRange;    // in km
    int EcoScore;          // overall eco score
    int clockTime;         // in HHMM format

    // Trip data
    float tripDistance;
    float tripFuelUsed;
    float tripDuration; // in seconds
    float averageSpeed;
    float averageFuelConsumption; // in L/100km
    float peakSpeed;
    int tripEcoScore;

    // Driving behavior events
    int hardBreakingEvents;
    int sharpTurnEvents;
    int hardAccelerationEvents;

    // Sensor related forces
    float brakingForce;
    float corneringForce;
    float accelerationForce;
};

void InitLiveData(Values *liveData);
void BackupLiveTripData(Values *liveData);
void RecomputeDerivedData(Values *liveData);
