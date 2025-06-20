# 🐄 Cattle Health Monitoring System Using IoT

> An ESP32‑based edge device that continuously measures vital signs, temperatures, and behavior of cattle—and uploads data for real‑time monitoring.

---

## 📑 Table of Contents

1. [🔎 Overview](#🔎-overview)  
2. [✨ Features](#✨-features)  
3. [🧰 Hardware Requirements](#🧰-hardware-requirements)  
4. [💻 Software Requirements](#💻-software-requirements)  
5. [🔌 Wiring & Pinout](#🔌-wiring--pinout)  
6. [🚀 Installation](#🚀-installation)  
7. [⚙️ Configuration](#⚙️-configuration)  
8. [▶️ Usage](#▶️-usage)  
9. [📤 Data Format & Endpoints](#📤-data-format--endpoints)  
10. [📂 Directory Structure](#📂-directory-structure)  
11. [🛠 Troubleshooting](#🛠-troubleshooting)  
12. [🔮 Future Enhancements](#🔮-future-enhancements)  
13. [🤝 Contributing](#🤝-contributing)  
14. [📄 License](#📄-license)  
15. [✉️ Contact](#✉️-contact)  

---

## 🔎 Overview

This project implements a wearable IoT module for cattle:

- 🩺 **Vital signs**: SpO₂ & heart rate via MAX30100  
- 🌡️ **Temperatures**: Body (MLX90614) & ambient/humidity (DHT22)  
- 🕺 **Behavior**: Resting, standing, grazing, motion via ADXL345 + FFT  
- 🌐 **Data upload**: HTTP POST to CHMS server with NTP‑synced timestamps  
- 🧩 **Non‑blocking**: Dual‑core ESP32 (core 0 for sensors, core 1 for network)

---

## ✨ Features

- ⏱️ **2 s sensor warm‑up**, then continuous measurements  
- 📊 **10 s rolling health window**: RMS heart rate & max SpO₂  
- 🔄 **Rectal temp estimation**:  
  \[
    T_{\text{rectal}} = 0.82\,T_{\text{body}} + 0.13\,T_{\text{ambient}} + 0.03\,\%RH + 2.1
  \]
- 🎚️ **Behavior classification** at 50 Hz, 2.56 s windows  
- ⌚ **NTP timekeeping** (IST, UTC+5:30) for `measured_at`  
- 🔒 **URL‑encoded POST** compatible with CHMS API  

---

## 🧰 Hardware Requirements

| ✅ Component            | 🔧 Purpose                       | 🔌 Interface  |
| ---------------------- | ------------------------------- | ------------- |
| ESP32 Dev Module       | MCU & Wi‑Fi                      | —             |
| MAX30100 Pulse Oximeter| SpO₂ & HR sensing                | I²C           |
| MLX90614 IR Sensor     | Non‑contact body temp            | I²C           |
| DHT22 (AM2302)         | Ambient temp & humidity          | GPIO          |
| ADXL345 Accelerometer  | Motion & behavior classification | I²C           |
| Breadboard & Wires     | Prototyping                      | —             |
| 5 V Power Supply       | Stable power                     | —             |

---

## 💻 Software Requirements

- **Arduino IDE** (≥ 1.8.13) or **PlatformIO**  
- **ESP32 board support**  
- **Libraries** (via Library Manager):  
  - MAX30100_PulseOximeter  
  - Adafruit MLX90614  
  - DHT sensor library  
  - Wire (built‑in)

---

