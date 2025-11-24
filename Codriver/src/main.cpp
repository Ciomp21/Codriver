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

void setup()
{
  // put your setup code here, to run once:
}

void loop()
{
  // put your main code here, to run repeatedly:
}
