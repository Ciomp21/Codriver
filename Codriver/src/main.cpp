#include <Arduino.h>

// -------------------------------------------------- Features to do -----------------------------------------------

// FREERTOS use vTaskDelayUntil for periodic tasks to avoid cpu overload and ensure precise timing

// 2 main tasks: Sensor reading + Control (main loop), Webserver server

// Sensor reading task: read sensors every 10ms, update global variables

// Control task: run every 10ms, read global variables, compute control, update actuators

// Webserver task: serve web requests, modify global variables as needed, upload data to cloud

// Use MUTEX/SEM to protect shared global variables between tasks

// Mobile app to set presets and view trip data remotely

// Web server to configure settings and upload trip data to cloud

// Presets for different driving modes (e.g., Eco, Sport, Comfort)

// Displays show the preset info based on selected mode

// Read the OBD2 data using ELM327 over UART or CAN bus

// Sensors: MAF, RPM, Speed, Throttle position, Engine load, Intake temp, Fuel level, Engine temp, Oil pressure

// IMU for acceleration, braking, cornering forces

// -------------------------------------------------- Display Info --------------------------------------------------

// ECO MODE DISPLAYS

// Display 1 — Fuel Efficiency Gauge

// Instant fuel consumption (calculated from MAF + speed) / Average L/100km or MPG / Eco score (calculated from throttle smoothness)

// Fuel rate (L/h) = MAF / 14.7 / 720 * density / Eco score = weighted smoothness of throttle, RPM and braking

// Display 2 — Driving Smoothness / Load

// Engine load % / Throttle position % / RPM (with color zones favoring low RPMs)

// Smooth acceleration indicator (based on IMU forward G stability)

// Display 3 — Predictive Efficiency Tools

// Suggested shift indicator (based on RPM + load) / Overbrake detection (IMU + speed drop)

// Shift recommendation using simple efficiency map / Brake aggressiveness = Δspeed vs ΔIMU_G

// SPORT MODE DISPLAYS

// Display 1 — Performance Metrics
// Instant horsepower / Torque (calculated from RPM + load) / Intake temperature

// Display 2 — Engine Stress Indicators
// RPM (with redline warning) / Engine temperature / Oil pressure / 0–100 km/h timer (automatically triggered)

// Display 3 — Driving Dynamics
// Lateral G-force (from IMU) / Throttle position % / Brake pressure (if available)

// NORMAL MODE DISPLAYS

// Display 1 — Standard Driving Info
// Speed / RPM / Fuel level

// Display 2 — Trip Information
// Trip distance / Average speed / Average fuel consumption

// Display 3 — Vehicle Health
// Engine temperature / Oil pressure / Road incline (pitch) / Battery voltage

// -------------------------------------------------- Live trip datas ----------------------------------------------

// Live trip data to cloud: speed, RPM, fuel consumption, trip distance, driving behavior (acceleration, braking patterns)
// Use MQTT or HTTP POST to send data to cloud server at regular intervals (e.g., every 1 minute)
// Data to MONITOR:
// Speed, RPM, Fuel consumption rate, Trip distance, Acceleration patterns, Braking patterns
// Use JSON format for data payload

// Esp32 on power up starts webserver and connects to WiFi
// Starts to send live trip data to cloud server at regular intervals
// Saves the last 1 minute of data locally, then does an average and sends to cloud every minute

// Web/Mobile app to view live trip data and historical trends
// App connects to cloud server to fetch and display data
// Detects trips by a gap of 5 minutes of no movement and does an average of the trip data

// Data is stored locally if no internet and sent when connection is restored

//------------------------------------------------- OBD2 and Sensors ------------------------------------------------

// Use ELM327 or similar OBD2 interface to read vehicle data
// Connect via UART or CAN bus depending on vehicle compatibility
// Use appropriate libraries for OBD2 communication (e.g., OBD2ESP32)

// Read key PIDs: MAF, RPM, Speed, Throttle position, Engine load, Intake temp, Fuel level, Engine temp, Oil pressure
// Use IMU (e.g., MPU6050) to get acceleration, braking, cornering forces

// Combine OBD2 and IMU data for comprehensive vehicle monitoring

// -------------------------------------------------- Setup & Loop -------------------------------------------------

#include <obd.h>
#include <sensors.h>
#include <server.h>
#include <render.h>
#include <liveData.h>


LiveData liveData; // Use global struct to hold live data (must be protected with mutex if accessed from multiple tasks)


void RecomputeDerivedData()
{
    // Compute any derived data from the raw sensor/OBD2 data
    // For example, calculate fuel consumption from MAF and speed

    if (liveData.speed > 0)
    {
        // Fuel consumption (L/100km) = (MAF (g/s) * 3600) / (14.7 * 720 * speed (km/h))
        liveData.fuelConsumption = (liveData.maf * 3600.0f) / (14.7f * 720.0f * liveData.speed);
    }
    else
    {
        liveData.fuelConsumption = 1.0f; // Base consumption when idle
    }

    // Trip distance could be computed by integrating speed over time

    static unsigned long lastUpdateTime = 0;
    unsigned long currentTime = millis();

    if (lastUpdateTime != 0)
    {
      // Time passed from the last update in hours
        float deltaTimeHours = (currentTime - lastUpdateTime) / 3600000.0f; // Convert ms to hours

      // Approximate trip distance increment = speed * time (Fast enough for small intervals)
        liveData.tripDistance += liveData.speed * deltaTimeHours; // distance = speed * time
    }
    lastUpdateTime = currentTime;

    // Trip fuel used = integrate fuel consumption over distance (Just an approximation here but good enough)
    liveData.tripFuelUsed += liveData.fuelConsumption * liveData.speed * ((currentTime - lastUpdateTime) / 3600000.0f); // L/100km * km = L

    // Trip duration in seconds
    liveData.tripDuration += (currentTime - lastUpdateTime) / 1000.0f; // Convert ms to seconds

    if (liveData.tripDuration > 0)
    {
        liveData.averageSpeed = liveData.tripDistance / (liveData.tripDuration / 3600.0f); // km/h
        liveData.averageFuelConsumption = (liveData.tripFuelUsed / liveData.tripDistance) * 100.0f; // L/100km
    }

    
}

void BackupLiveTripData(LiveData *data)
{
    // Backup live trip data to non-volatile storage if needed
    // This is a placeholder function; actual implementation would depend on the storage method used

    // Probably every 30 seconds or so to avoid excessive writes to flash memory
    // Just write it on a Json file with a certain format


    // Web server can give back the last trip data on request


    // IMPORTANT!!!! : How to manage old trip data? Overwrite last trip? Save multiple trips with timestamps?
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(96000);

  InitSensors();

  InitObd();

  InitDisplays();

  startWebServer();

  liveData = {}; // Initialize live data struct to zero values
}

void loop()
{
  // put your main code here, to run repeatedly:
  for (;;)
  {
    // Read sensors with delays to allow sensor read time
    // Temperature/humidity sensor needs time to read
    // Other sensors can be read quickly
    ReadSensorData(&liveData);

    // Read OBD2 data with some sort of delay to avoid overwhelming the bus
    ReadObdData(&liveData);

    RecomputeDerivedData();

    BackupLiveTripData(&liveData);

    // Might use a modified parameter to control update rate of display
    DisplayRefresh(&liveData);

    delay(2000);
  }
}
