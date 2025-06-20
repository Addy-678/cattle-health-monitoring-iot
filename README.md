```markdown
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

## 🔌 Wiring & Pinout

```

ESP32      ──── MAX30100 ──── MLX90614 ──── ADXL345 ─── DHT22
3V3   ─── VCC       VCC            VCC           ──
GND   ─── GND       GND            GND           ──
GPIO21 ── SDA       SDA            SDA           ──
GPIO22 ── SCL       SCL            SCL           ──
—         —              —          DATA (GPIO4, 10 KΩ pull‑up)

````

---

## 🚀 Installation

1. **Clone**  
   ```bash
   git clone https://github.com/<you>/cattle-health-monitoring-iot.git
   cd cattle-health-monitoring-iot
````

2. **Libraries**
   Arduino IDE → **Sketch ▶️ Include Library ▶️ Manage Libraries** → install dependencies.
3. **Open**
   Load `src/main.ino` (or `platformio.ini` in PlatformIO).

---

## ⚙️ Configuration

Edit top of `src/main.ino`:

```cpp
// Wi‑Fi
const char* ssid     = "YOUR_SSID";
const char* password = "YOUR_PASS";

// NTP (IST)
const long gmtOffset_sec     = 19800;

// CHMS API
const char* healthDataURL = "https://…/sendIotData";
const char* motionDataURL = "https://…/sendMotionData";

// IDs
const char* cattle_id = "134";
const char* device_id = "1";
```

---

## ▶️ Usage

1. **Power on** ESP32
2. **Serial Monitor** @115200 baud shows:

   * 🔗 Wi‑Fi status
   * ⚙️ Sensor stabilization
   * 🩺 Health data every 10 s
   * 🐾 Behavior logs on change
3. **Data posts** happen automatically.

---

## 📤 Data Format & Endpoints

* **Health** (`healthDataURL`):
  - `POST` `application/x-www-form-urlencoded`
  - Payload:

  ```
  cattle_id=<ID>&device_id=<ID>&measured_at=YYYY-MM-DD HH:MM:SS&
  data=<body_temp>,<SpO2>,<HR>,<amb_temp>,<RH>,<rectal_temp>
  ```
* **Motion** (`motionDataURL`):
  - `POST` same content‐type
  - Payload:

  ```
  cattle_id=<ID>&device_id=<ID>&stdDev=<σ>&maxMagnitude=<g>&domFrequency=<Hz>
  ```

---

## 📂 Directory Structure

```
📁 cattle-health-monitoring-iot/
├─ .gitignore
├─ README.md
├─ LICENSE
├─ docs/          📐 schematics & flowcharts
├─ src/           💾 Arduino sketch
│   └─ main.ino
├─ data/          🗒 sample payloads
│   └─ sample_payload.json
└─ scripts/       📊 Python plotting tools
    └─ plot_data.py
```

---

## 🛠 Troubleshooting

* ❌ **Wi‑Fi**: check SSID/password & 2.4 GHz
* 📉 **Sensor data**: verify wiring & libs
* 🕰️ **NTP**: try alternate servers
* 🌐 **POST errors**: inspect endpoint & SSL

---

## 🔮 Future Enhancements

* 🔒 HTTPS + certificate pinning
* 🤖 Edge AI anomaly detection
* ☀️ Solar/battery management
* 📈 Real‑time dashboard & alerts

---

## 🤝 Contributing

1. Fork & branch (`feature/xyz`)
2. Commit & push
3. Open a PR

> Please follow the [Contributor Covenant](https://www.contributor-covenant.org/).

---

## 📄 License

Distributed under the **MIT License**. See [LICENSE](LICENSE).

---

## ✉️ Contact

**ADDY**
📧 [ad68dy@gmail.com](mailto:ad68dy@gmail.com)
🔗 [https://github.com/your‑username/cattle-health-monitoring-iot](https://github.com/Addy-678/cattle-health-monitoring-iot)

```
```
