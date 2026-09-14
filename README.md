# IoT Farming Security Prototype 🚜🔒

This repository contains the prototype source code, raw CloudWatch execution logs, and experimental datasets supporting the paper:
**"Threat-Centric Defence-in-Depth for IoT-Enabled Smart Farming Systems in Cloud Environments"** (ICECER 2026)

---

## 📁 Repository Structure & Supplementary Materials

### 1. Source Code & Firmware
- `iot_irrigation_esp/`: NodeMCU ESP32-S3 firmware (C++ / PlatformIO)
- `dashboard/`: Web application dashboard (Node.js / React)

### 2. Supplementary Datasets & Replication Package
| Category | File Name | Description |
| :--- | :--- | :--- |
| **Threat Modeling** | [`Comprehensive_Threat_Dataset.xlsx`](./Supplementary_Data/Comprehensive_Threat_Dataset.xlsx) | Full catalog of 124 identified Agri-IoT threats mapped to CVSS v3.1 |
| **Threat Modeling** | [`High_Risk_Threat_Summary.xlsx`](./Supplementary_Data/High_Risk_Threat_Summary.xlsx) | Selected high-risk threat matrix (CVSS >= 7.0) |
| **Threat Modeling** | [`Threat_Clarification_8_Threats.docx`](./Supplementary_Data/Threat_Clarification_8_Threats.docx) | Step-by-step breakdown of the 8 simulated attack scenarios (T1–T8) |
| **Experimental Data** | [`Dataset_Before_Mitigation.xlsx`](./Supplementary_Data/Dataset_Before_Mitigation.xlsx) | Unsecured baseline telemetry stream |
| **Experimental Data** | [`Processed_Security_Logs.xlsx`](./Supplementary_Data/Processed_Security_Logs.xlsx) | Master processed evaluation dataset (5,698 secure publishes) |
