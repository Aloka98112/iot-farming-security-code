\# IoT Farming Security Code



ESP32 smart irrigation + secure dashboard prototype.



\## Folders

\- `iot\_irrigation\_esp/` - ESP32 firmware (PlatformIO, MQTT/TLS, AWS IoT)

\- `dashboard/` - Node.js dashboard (AWS IoT Core)



\## Setup

1\. Add AWS certs to `iot\_irrigation\_esp/data/`

2\. `pio run --target upload` (ESP32)

3\. `npm install \&\& node dashboard/index.js`



\*\*Security\*\*: Certs removed. Add your own.



