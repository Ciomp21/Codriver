#pragma once

struct LiveData
{
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
    int gearPosition;
    float steerAngle;
    float brakePressure;

    // Calculated data
    float fuelConsumption; // in L/100km
    float engineLoad; // in percentage
    int gearSuggestion; // suggested gear for optimal fuel efficiency

    int extimatedRange; // in km
    int EcoScore; // overall eco score

    int clockTime; // in HHMM format
    int date; // in DDMMYY format


    // Temperature/Humidity sensor data
    float InternalTemp;
    float ExternalTemp;
    float humidity;

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

    // IMU sensor data
    float accelX;
    float accelY;
    float accelZ;
    float gyroX;
    float gyroY;
    float gyroZ;

    // Sensor related forces
    float brakingForce;
    float corneringForce;
    float accelerationForce;


};
