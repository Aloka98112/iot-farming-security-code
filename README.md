[![DOI](https://zenodo.org/badge/1142867559.svg)](https://doi.org/10.5281/zenodo.22742488)
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
| **Threat Modeling** | [`Comprehensive Threat Dataset.xlsx`](./Supplementary_Data/Comprehensive%20Threat%20Dataset.xlsx) | Full catalog of 124 identified Agri-IoT threats mapped to CVSS v3.1 |
| **Threat Modeling** | [`Final High-Risk Threat Summary.xlsx`](./Supplementary_Data/Final%20High-Risk%20Threat%20Summary.xlsx) | Selected high-risk threat matrix (CVSS >= 7.0) |
| **Threat Modeling** | [`Threat Clarification - 8 Threats.docx`](./Supplementary_Data/Threat%20Clarification%20-%208%20Threats.docx) | Step-by-step breakdown of the 8 simulated attack scenarios (T1–T8) |
| **Experimental Data** | [`Dataset - Before Mitigation.xlsx`](./Supplementary_Data/Dataset%20-%20Before%20Mitigation.xlsx) | Unsecured baseline telemetry stream |
| **Experimental Data** | [`Processed Security Logs.xlsx`](./Supplementary_Data/Processed%20Security%20Logs.xlsx) | Master processed evaluation dataset (5,698 secure publishes) |
