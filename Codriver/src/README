# Codriver - IoT Vehicle Monitoring System

## Overview

Codriver is an advanced IoT-based vehicle monitoring system that connects to a car's OBD-II interface via WiFi, displays real-time vehicle data on a TFT display, and streams telemetry data via Bluetooth Low Energy (BLE). The system monitors engine parameters, environmental conditions, and vehicle dynamics using integrated sensors.

---

## Hardware Requirements

### Main Components

- **ESP32-S3 DevKit C-1** (with 4MB Flash, PSRAM support)
- **TFT Display** (240x240 SPI Display)
  - MOSI: GPIO 11
  - SCLK: GPIO 12
  - CS: GPIO 10
  - DC: GPIO 6
  - RST: GPIO 7
- **MPU6050** (6-axis IMU - Accelerometer & Gyroscope)
  - SDA: GPIO 5
  - SCL: GPIO 4
  - I2C Address: 0x68
- **DHT11** (Temperature & Humidity Sensor)
  - Data Pin: GPIO 2
- **WiFi OBD-II Adapter** (ELM327 compatible)

### Wiring Diagram Summary

| Component   | Pin  | ESP32 GPIO |
| ----------- | ---- | ---------- |
| TFT MOSI    | MOSI | 11         |
| TFT SCLK    | SCLK | 12         |
| TFT CS      | CS   | 10         |
| TFT DC      | DC   | 6          |
| TFT RST     | RST  | 7          |
| MPU6050 SDA | SDA  | 5          |
| MPU6050 SCL | SCL  | 4          |
| DHT11 Data  | DATA | 2          |

---

## Software Requirements

### Development Environment

- **PlatformIO** (VS Code Extension or CLI)
- **Arduino Framework** for ESP32
- **Python 3.x** (for PlatformIO)
- **Git** (for version control)

### Required Libraries

The following libraries are automatically installed via PlatformIO:

- `moononournation/GFX Library for Arduino @ 1.4.7` - Display graphics
- `bblanchon/ArduinoJson @ ^6.20.0` - JSON parsing
- `adafruit/DHT sensor library @ ^1.4.6` - DHT11 sensor support
- `Arduino_GFX_Library` - TFT display driver
- `BLEDevice` - Bluetooth Low Energy communication
- `WiFi` - WiFi connectivity (ESP32 core)
- `LittleFS` - File system support
- `Wire` - I2C communication

---

## Project Layout

### Directory Structure

```
Codriver/
├── src/                          # Main source code
│   ├── main.cpp                  # Entry point, FreeRTOS task management
│   ├── global.hpp/cpp            # Global variables, data structures, mutexes
│   ├── requests.hpp/cpp          # WiFi and OBD-II communication
│   ├── screen.hpp/cpp            # TFT display rendering logic
│   ├── bleconnection.hpp/cpp    # BLE server setup and data streaming
│   ├── sensors.cpp               # DHT11 and MPU6050 sensor reading
│   ├── sensor.h                  # Sensor definitions and pin mappings
│   ├── complementary_filter.hpp  # IMU data filtering algorithm
│   └── codes.txt                 # ELM327 command reference
├── lib/
│   └── Datas/
│       └── livedata.h            # LiveData structure definition
├── data/                         # LittleFS filesystem data
├── include/                      # Additional headers
├── platformio.ini                # PlatformIO configuration
├── partitions.csv                # ESP32 partition table
├── sdkconfig.esp32-s3-devkitc-1 # ESP32-S3 SDK configuration
└── README.md                     # This file
```

### Source Code Organization

#### Core Components

**main.cpp**

- FreeRTOS task creation and management
- 5 concurrent tasks running on dual cores:
  - `vOBDFetchTask` - Fetches OBD-II data via WiFi (Core 0)
  - `vSENSFetchTask` - Reads DHT11 and MPU6050 sensors (Core 0)
  - `vUITask` - Renders TFT display (Core 1)
  - `vBLETask` - Handles BLE communication (Core 0)
  - `vSaveTask` - Persists settings to flash (Core 0)

**global.hpp/cpp**

- Shared data structures (`LiveData`, `OBDCommand_t`, `DataTypes_t`)
- FreeRTOS mutexes for thread-safe operations
- OBD-II PID definitions (RPM, Boost, Coolant Temp, etc.)
- State management and preferences storage

**requests.hpp/cpp**

- WiFi connection to OBD-II adapter
- ELM327 command protocol implementation
- Round-robin PID polling strategy
- Error handling and reconnection logic

**screen.hpp/cpp**

- TFT display initialization (GFX Library)
- Dynamic gauge rendering for different data types
- Multiple display modes (RPM, Boost, G-Force, etc.)
- Bitmap management and screen transitions

**bleconnection.hpp/cpp**

- BLE server setup with custom UUIDs
- Real-time telemetry streaming to mobile devices
- Characteristics for each vehicle parameter

**sensors.cpp/sensor.h**

- DHT11 temperature and humidity reading
- MPU6050 I2C initialization and configuration
- Accelerometer and gyroscope data acquisition
- Complementary filter for tilt/acceleration calculation

**complementary_filter.hpp**

- Sensor fusion algorithm for IMU data
- Combines accelerometer and gyroscope readings
- Removes drift and noise from motion tracking

---

## How to Build, Flash, and Run

### Prerequisites

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install [PlatformIO IDE extension](https://platformio.org/install/ide?install=vscode)
3. Connect your ESP32-S3 DevKit to your computer via USB

### Building the Project

```bash
# Open terminal in VS Code (Ctrl+`)
# Navigate to project directory
cd YourProjectDirectory\Codriver

# Build the project
pio run
```

Alternatively, use PlatformIO GUI:

- Open the PlatformIO sidebar (alien icon)
- Click "Build" under "Project Tasks" → "esp32-s3-devkitc-1"

### Flashing to ESP32-S3

```bash
# Upload firmware to the board
pio run --target upload
```

Or via GUI:

- Click "Upload" in PlatformIO Project Tasks

### Monitoring Serial Output

```bash
# Open serial monitor
pio device monitor -b 115200
```

Or via GUI:

- Click "Monitor" in PlatformIO Project Tasks

### Complete Workflow

1. **Configure WiFi Credentials** (in [requests.cpp](src/requests.cpp))
   - Set your OBD-II adapter's WiFi SSID and password
   - Default OBD-II adapter IP: `192.168.0.10:35000`

2. **Build and Upload**

   ```bash
   pio run --target upload
   ```

3. **Connect Hardware**
   - Plug OBD-II adapter into your vehicle's OBD-II port
   - Power on the ESP32-S3 (via USB or external power)
   - The system will automatically connect to the OBD adapter

4. **Test Functionality**
   - Check Serial Monitor for debug output
   - Verify TFT display shows vehicle data
   - Connect via BLE to change screen via a mobile app

---

## User Guide

### Initial Setup

1. **First Boot**: The system initializes all sensors and connects to WiFi
2. **Display Modes**: Press Enter in the Serial Monitor to cycle through different display modes:
   - RPM Gauge
   - Boost Pressure
   - G-Force Visualization
   - Engine Temperature
   - Internal Temps
   - Battery Voltage
   - Pitch and Roll Gauges

### Display Modes

- **RPM**: Shows engine revolutions per minute
- **Boost**: Displays turbo boost pressure
- **G-Force**: Real-time acceleration visualization (braking, cornering)
- **Temperature**: Engine coolant temperature
- **Voltage**: Battery voltage monitoring
- **Car Temp**: Inside the car temperature and humidity
- **Pitch and Roll**: Side and front inclination of the car currently

### Bluetooth Connectivity

1. Enable BLE on your mobile device
2. Scan for "Codriver" or the custom device name
3. Connect to access the customization of the UI
4. Use a BLE scanning app (e.g., nRF Connect) to view characteristics

### Testing Mode

For development without a vehicle:

- Uncomment `#define TESTING` in [global.hpp](src/global.hpp)
- The system will generate simulated vehicle data

### Troubleshooting

| Issue                 | Solution                                                       |
| --------------------- | -------------------------------------------------------------- |
| Display not working   | Check SPI wiring and power supply                              |
| WiFi connection fails | Verify OBD adapter is powered and WiFi credentials are correct |
| No sensor data        | Check I2C connections (MPU6050) and DHT11 wiring               |
| BLE not discoverable  | Ensure BLE is enabled in code and ESP32 is powered             |

---

## Features

### Real-Time Monitoring

- ✅ Engine RPM
- ✅ Turbo Boost Pressure
- ✅ Coolant & Oil Temperature
- ✅ Battery Voltage
- ✅ Engine Load
- ✅ G-Force (Acceleration, Braking, Cornering)
- ✅ Cabin Temperature & Humidity
- ✅ Vehicle Tilt/Incline

### Advanced Capabilities

- **Dual-Core Processing**: Separate UI rendering from data acquisition
- **Round-Robin PID Polling**: Optimized OBD-II query scheduling
- **Persistent Settings**: Saves display preferences to flash memory
- **Error Handling**: Automatic reconnection on WiFi/OBD failures
- **Sensor Fusion**: Complementary filter for accurate IMU data

---

## Project Links

### Documentation & Presentation

- 📊 **PowerPoint Presentation**: [Link to Presentation](#) _(Add your link here)_
- 🎥 **YouTube Demo Video**: [Link to Video](#) _(Add your link here)_

---

## Team Members & Contributions

### Team Structure

This project was developed by a collaborative team with the following contributions:

#### **[Member Name 1]** - _Team Lead & Hardware Integration_

- Designed and implemented the hardware wiring schematic
- Integrated MPU6050 IMU sensor and DHT11 sensor
- Configured ESP32-S3 pinout and peripheral connections
- Tested and validated all sensor readings

#### **[Member Name 2]** - _Firmware Development & OBD-II Communication_

- Developed OBD-II WiFi communication protocol ([requests.cpp](src/requests.cpp))
- Implemented ELM327 command parsing and error handling
- Created FreeRTOS task architecture in [main.cpp](src/main.cpp)
- Optimized round-robin PID polling strategy

#### **[Member Name 3]** - _Display & User Interface_

- Designed TFT display rendering system ([screen.cpp](src/screen.cpp))
- Created dynamic gauge visualizations and bitmap management
- Implemented multiple display modes (RPM, Boost, G-Force)
- Developed UI update logic and screen transitions

#### **[Member Name 4]** - _Bluetooth & Data Streaming_

- Implemented BLE server and characteristic definitions ([bleconnection.cpp](src/bleconnection.cpp))
- Designed real-time telemetry streaming architecture
- Created data structure formats for mobile app integration
- Tested BLE connectivity with various mobile devices

#### **[Member Name 5]** - _Sensor Fusion & Algorithm Development_

- Developed complementary filter for IMU data ([complementary_filter.hpp](src/complementary_filter.hpp))
- Implemented sensor reading logic ([sensors.cpp](src/sensors.cpp))
- Tuned sensor calibration and filtering parameters
- Validated acceleration and tilt calculations

#### **Collective Efforts**

- Code review and debugging sessions
- System integration and testing
- Documentation and presentation preparation
- Demo video production and editing

---

## Technical Specifications

### Performance Metrics

- **Refresh Rate**: 50 Hz (20ms UI update cycle)
- **OBD-II Polling**: Variable (10-20ms per PID)
- **Sensor Update**: 150ms cycle
- **BLE Streaming**: 500ms update interval

### Memory Usage

- **Flash**: ~2MB (out of 4MB)
- **SRAM**: ~32KB heap usage
- **PSRAM**: Enabled for graphics buffering

### Power Consumption

- **Active Mode**: ~200-300mA @ 5V
- **Display**: ~80-100mA
- **ESP32-S3**: ~120-150mA (WiFi + BLE active)

---

## License

This project is developed as part of an IoT university course. Please refer to your course guidelines for usage and distribution policies.

---

## Acknowledgments

- **Espressif Systems** - ESP32-S3 platform
- **PlatformIO** - Development environment
- **Arduino Community** - Libraries and examples
- **Course Instructors** - Guidance and support

---

## Contact & Support

For questions or issues, please contact the team members or refer to the project repository's issue tracker.

**Last Updated**: February 2026
