# IoT Farming Security Prototype 🚜🔒

ESP32 smart irrigation + secure cloud dashboard with AWS IoT Core.

[![ESP32](https://img.shields.io/badge/ESP32-S3-blue)](https://docs.platformio.org/en/latest/boards/espressif32/esp32-s3.html)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-IDE-orange)](https://platformio.org/)

## 📁 Structure

## 🚀 Quick Start

### ESP32 Firmware
```bash
cd iot_irrigation_esp
pio run --target upload  # Flash to ESP32

cd dashboard/dashboard
npm install
node index.js

