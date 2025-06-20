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

* 🩺 **Vital signs**: SpO₂ & heart rate via MAX30100
* 🌡️ **Temperatures**: Body (MLX90614) & ambient/humidity (DHT22)
* 🕺 **Behavior**: Resting, standing, grazing, motion via ADXL345 + FFT
* 🌐 **Data upload**: HTTP POST to CHMS server with NTP‑synced timestamps
* 🧩 **Non‑blocking**: Dual‑core ESP32 (core 0 for sensors, core 1 for network)

---

## ✨ Features

* ⏱️ **2 s sensor warm‑up**, then continuous measurements
* 📊 **10 s rolling health window**: RMS heart rate & max SpO₂
* 🔄 **Rectal temp estimation**:

  ```math
  T_{\text{rectal}} = 0.82\,T_{\text{body}} + 0.13\,T_{\text{ambient}} + 0.03\,\%RH + 2.1
  ```
* 🎚️ **Behavior classification** at 50 Hz, 2.56 s windows
* ⌚ **NTP timekeeping** (IST, UTC+5:30) for `measured_at`
* 🔒 **URL‑encoded POST** compatible with CHMS API

---

## 🧰 Hardware Requirements

| ✅ Component             | 🔧 Purpose                       | 🔌 Interface |
| ----------------------- | -------------------------------- | ------------ |
| ESP32 Dev Module        | MCU & Wi‑Fi                      | —            |
| MAX30100 Pulse Oximeter | SpO₂ & HR sensing                | I²C          |
| MLX90614 IR Sensor      | Non‑contact body temp            | I²C          |
| DHT22 (AM2302)          | Ambient temp & humidity          | GPIO         |
| ADXL345 Accelerometer   | Motion & behavior classification | I²C          |
| Breadboard & Wires      | Prototyping                      | —            |
| 5 V Power Supply        | Stable power                     | —            |

---

## 💻 Software Requirements

* **Arduino IDE** (≥ 1.8.13) or **PlatformIO**
* **ESP32 board support**
* **Libraries** (via Library Manager):

  * MAX30100\_PulseOximeter
  * Adafruit MLX90614
  * DHT sensor library
  * Wire (built‑in)

---

## 🔌 Wiring & Pinout

| ESP32 Pin | MAX30100 | MLX90614 | ADXL345 | DHT22                       |
| --------- | -------- | -------- | ------- | --------------------------- |
| 3V3       | VCC      | VCC      | VCC     | —                           |
| GND       | GND      | GND      | GND     | —                           |
| GPIO21    | SDA      | SDA      | SDA     | —                           |
| GPIO22    | SCL      | SCL      | SCL     | —                           |
| —         | —        | —        | —       | DATA (GPIO4, 10 KΩ pull‑up) |

---

## 🚀 Installation

1. **Clone the repo**

   ```bash
   git clone https://github.com/<you>/cattle-health-monitoring-iot.git
   cd cattle-health-monitoring-iot
   ```
2. **Install libraries**

   * In Arduino IDE: **Sketch** ▶️ **Include Library** ▶️ **Manage Libraries** → install dependencies
   * In PlatformIO: ensure `platformio.ini` lists the required libraries

---

## ⚙️ Configuration

Edit the top of `src/main.ino`:

```cpp
// Wi‑Fi
const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASS";

// NTP (IST)
const long gmtOffset_sec = 19800;

// CHMS API
const char* healthDataURL = "https://…/sendIotData";
const char* motionDataURL = "https://…/sendMotionData";

// IDs
const char* cattle_id = "134";
const char* device_id = "1";
```

---

## ▶️ Usage

1. **Power on** the ESP32
2. **Open Serial Monitor** @115200 baud to see:

   * 🔗 Wi‑Fi status
   * ⚙️ Sensor stabilization
   * 🩺 Health data every 10 s
   * 🐾 Behavior logs on change
3. **Data posts** happen automatically to configured endpoints.

---

## 📤 Data Format & Endpoints

### Health Data (`healthDataURL`)

* **Method**: POST
* **Content-Type**: application/x-www-form-urlencoded
* **Payload**:

  ```text
  cattle_id=<ID>&device_id=<ID>&measured_at=YYYY-MM-DD HH:MM:SS&
  data=<body_temp>,<SpO2>,<HR>,<amb_temp>,<RH>,<rectal_temp>
  ```

### Motion Data (`motionDataURL`)

* **Method**: POST
* **Content-Type**: application/x-www-form-urlencoded
* **Payload**:

  ```text
  cattle_id=<ID>&device_id=<ID>&stdDev=<σ>&maxMagnitude=<g>&domFrequency=<Hz>
  ```

---

## 📂 Directory Structure

```
cattle-health-monitoring-iot/
├─ .gitignore
├─ README.md
├─ LICENSE
├─ docs/          📐 schematics & flowcharts
├─ code/           💾 Arduino sketch
│   └─ chms.ino
├─ images/          🗒 sample images

```

---

## 🛠 Troubleshooting

* ❌ **Wi‑Fi**: check SSID/password & ensure 2.4 GHz network
* 📉 **Sensor data**: verify wiring & library versions
* 🕰️ **NTP**: try alternate servers or correct GMT offset
* 🌐 **POST errors**: inspect endpoint URL & SSL certificates

---

## 🔮 Future Enhancements

* 🔒 HTTPS + certificate pinning
* 🤖 Edge AI anomaly detection on-device
* ☀️ Solar/battery power management
* 📈 Real‑time dashboard & alerting system



## 📄 License

Distributed under the MIT License. See [LICENSE](LICENSE) for details.

---

## ✉️ Contact

**ADDY**
📧 [ad68dy@gmail.com](mailto:ad68dy@gmail.com)
🔗 [https://github.com/Addy-678/cattle-health-monitoring-iot](https://github.com/Addy-678/cattle-health-monitoring-iot)
