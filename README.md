# ESP32-S3 Automotive Dashboard

This project reads OBD-II and IMU data, processes it on an ESP32-S3, and displays it across 3 screens.  
All libraries are auto-installed through PlatformIO.

## 🔧 How to Build & Flash

1. Install **VS Code**
2. Install **PlatformIO extension**
3. Clone this repo:

4. Open the folder in VS Code  
5. Click **PlatformIO → Upload**

Done. No manual library installation needed.

## 📁 Structure
- `/src/main.cpp` — firmware source  
- `/data/` — files for SPIFFS/LittleFS  
- `/lib/` — custom local libraries  
- `/include/` — headers/config  
- `platformio.ini` — manages dependencies  
