# 🌾 Smart Farming IoT — Real-Time Soil Monitoring & Irrigation System

<div align="center">

![ESP32](https://img.shields.io/badge/ESP32-Microcontroller-blue?style=for-the-badge&logo=espressif)
![Arduino](https://img.shields.io/badge/Arduino-IDE-teal?style=for-the-badge&logo=arduino)
![HTML5](https://img.shields.io/badge/HTML5-Dashboard-orange?style=for-the-badge&logo=html5)
![JavaScript](https://img.shields.io/badge/JavaScript-ES6+-yellow?style=for-the-badge&logo=javascript)
![RS485](https://img.shields.io/badge/RS485-Modbus-green?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-purple?style=for-the-badge)

**An IoT-based precision agriculture system that monitors soil health in real-time and automates irrigation using ESP32 microcontroller with multiple sensor integrations.**

[🚀 Live Dashboard](https://kartikbaghelwork1-ship-it.github.io/smart-farming-iot/dashboard/) · [📖 Documentation](#-how-it-works) · [🔧 Setup Guide](#-setup--installation)

</div>

---

## 📋 Problem Statement

Indian farmers face significant challenges:
- **70% of irrigation** in India is flood-based, wasting up to 60% of water
- **Soil degradation** due to over-fertilization without proper NPK monitoring
- **Lack of real-time data** for informed decision-making
- **No affordable precision agriculture** tools for small-scale farmers

## 💡 Our Solution

A **low-cost, real-time soil monitoring and smart irrigation system** that:
- Reads **8 soil parameters** using RS485 Modbus + analog sensors
- Displays live data on a **beautiful web dashboard** (accessible from any device)
- **Automatically controls irrigation** based on soil moisture thresholds
- Works **offline** — no internet required (ESP32 creates its own Wi-Fi hotspot)
- Supports **11 Indian languages** for farmer accessibility

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    SENSOR LAYER                         │
│                                                         │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐              │
│  │ DHT22    │  │  Soil    │  │    pH    │              │
│  │ Temp +   │  │ Moisture │  │  Sensor  │              │
│  │ Humidity │  │ (Analog) │  │ (Analog) │              │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘              │
│       │              │              │                    │
│  ┌────┴──────────────┴──────────────┴────┐              │
│  │         ESP32 Microcontroller         │              │
│  │          (Processing Unit)            │              │
│  └────┬──────────────┬──────────────┬────┘              │
│       │              │              │                    │
│  ┌────┴─────┐  ┌─────┴────┐  ┌─────┴────┐              │
│  │   MST    │  │   NPK    │  │  Relay   │              │
│  │ Temp/EC/ │  │  N/P/K   │  │  Module  │──→ Water Pump│
│  │ Moisture │  │ (RS485)  │  │          │              │
│  │ (RS485)  │  │          │  │          │              │
│  └──────────┘  └──────────┘  └──────────┘              │
└─────────────────────────────────────────────────────────┘
                         │
                    Wi-Fi / USB
                         │
┌─────────────────────────────────────────────────────────┐
│                  DASHBOARD LAYER                        │
│                                                         │
│  ┌─────────────────────────────────────────────────┐    │
│  │          Real-Time Web Dashboard                │    │
│  │  • Live sensor readings with charts             │    │
│  │  • Crop suitability analysis                    │    │
│  │  • Nutrient guidance                            │    │
│  │  • Manual/Auto irrigation control               │    │
│  │  • Multi-language support (11 languages)        │    │
│  │  • CSV data export                              │    │
│  └─────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────┘
```

---

## 📊 Monitored Parameters

| # | Parameter | Sensor | Interface | Unit |
|---|-----------|--------|-----------|------|
| 1 | 🌡️ Air Temperature | DHT22 | GPIO 25 | °C |
| 2 | 💧 Air Humidity | DHT22 | GPIO 25 | % |
| 3 | 🌱 Soil Moisture | Capacitive Analog | GPIO 34 (ADC) | % |
| 4 | ⚗️ Soil pH | pH Electrode | GPIO 35 (ADC) | pH |
| 5 | ⚡ Electrical Conductivity | MST Sensor | RS485 Modbus | mS/cm |
| 6 | 🧪 Nitrogen (N) | NPK Sensor | RS485 Modbus | mg/kg |
| 7 | 🧪 Phosphorus (P) | NPK Sensor | RS485 Modbus | mg/kg |
| 8 | 🧪 Potassium (K) | NPK Sensor | RS485 Modbus | mg/kg |

---

## 🖥️ Dashboard Features

### Real-Time Monitoring
- Live updating sensor values with animated counters
- Interactive SVG charts for field conditions over time
- Sparkline graphs for quick trend visualization

### Smart Irrigation
- **Automatic pump control** — turns ON below 30% moisture, OFF above 35%
- Manual override with safety confirmation dialog
- Pump state acknowledgement from firmware
- Maximum run-time cutoff (30 seconds default) for safety

### Crop Suitability
- Rule-based crop matching (Tomato, Potato, Maize)
- Compares pH, moisture, and temperature against optimal ranges

### Nutrient Guidance
- NPK level assessment with visual bar charts
- Threshold-based recommendations

### Accessibility
- **11 languages**: English, Hindi, Marathi, Gujarati, Punjabi, Bengali, Tamil, Telugu, Kannada, Malayalam, Urdu, Spanish
- Fully responsive — works on mobile, tablet, desktop
- Keyboard accessible with proper ARIA labels

### Data Export
- One-click CSV export of all historical readings
- Timestamped data with source identification

---

## 🔧 Setup & Installation

### Prerequisites
- Arduino IDE with ESP32 board package
- Libraries: `ArduinoJson 7`, `DHT sensor library (Adafruit)`, `Adafruit Unified Sensor`

### Hardware Wiring (MITS-DU Trainer Kit)

| Component | ESP32 GPIO |
|-----------|-----------|
| DHT22 DATA | GPIO 25 |
| Soil Moisture AO | GPIO 34 |
| pH Sensor AO | GPIO 35 |
| Relay IN | GPIO 26 |
| RS485 RX (RO) | GPIO 16 |
| RS485 TX (DI) | GPIO 17 |
| RS485 DE/RE | GPIO 4 |

### Step 1: Upload Firmware
```bash
# Open in Arduino IDE
firmware/esp32_farming.ino

# Select: Tools → Board → ESP32 Dev Module
# Select: Tools → Port → Your ESP32 port
# Click Upload
```

### Step 2: Connect & View Dashboard

**Option A — Wi-Fi (No internet needed):**
1. Connect to Wi-Fi: `Fieldwise-xxxx` (password: `fieldwise2026`)
2. Open browser: `http://192.168.4.1`

**Option B — USB Serial:**
1. Open the [Live Dashboard](https://kartikbaghelwork1-ship-it.github.io/smart-farming-iot/dashboard/)
2. Go to Hardware Connection → USB Serial → Connect Device

**Option C — Local:**
```bash
cd dashboard
python3 -m http.server 8888
# Open http://localhost:8888
```

---

## 📁 Project Structure

```
smart-farming-iot/
├── README.md                    # Project documentation
├── LICENSE                      # MIT License
├── firmware/                    # ESP32 Arduino firmware
│   ├── esp32_farming.ino        # Main firmware (sensors + server)
│   ├── Config.h                 # GPIO pins, calibration, credentials
│   └── Dashboard.h              # Compressed dashboard (auto-served)
├── dashboard/                   # Web dashboard
│   └── index.html               # Single-file responsive dashboard
├── hardware/                    # Hardware documentation
│   └── wiring_diagram.md        # Connection guide
└── docs/                        # Additional documentation
    └── images/                  # Screenshots & photos
```

---

## ⚙️ Communication Protocol

### Telemetry (ESP32 → Dashboard)
```json
{
  "type": "telemetry",
  "protocol": "fieldwise-v1",
  "temperature": 28.4,
  "humidity": 64,
  "moisture": 42,
  "ph": 6.5,
  "nitrogen": 120,
  "phosphorus": 45,
  "potassium": 80,
  "ec": 1.2,
  "pumpOn": false
}
```

### Pump Command (Dashboard → ESP32)
```json
{
  "type": "pump",
  "id": "unique-command-id",
  "on": true,
  "token": "control-token"
}
```

### RS485 Modbus Protocol
- **MST Sensor** (Address: 0x01) — Registers 0x000B-0x000D (Temp, Moisture, EC)
- **NPK Sensor** (Address: 0x55) — Registers 0x0003-0x0005 (N, P, K)
- CRC16 validation on all Modbus frames

---

## 🛡️ Safety Features

- ⏱️ **Pump auto-cutoff** — Maximum run time enforced by hardware timer
- 🔐 **Token-based pump control** — Prevents unauthorized operation
- 🔒 **CORS protection** — Origin validation on all API endpoints
- ⚡ **Relay safety** — Driven OFF on boot before firmware initializes
- 📊 **Data validation** — All sensor readings validated before display

---

## 🚀 Tech Stack

| Layer | Technology |
|-------|-----------|
| Microcontroller | ESP32-WROOM (Xtensa dual-core) |
| Sensors | DHT22, Capacitive Soil, pH Electrode, RS485 Modbus |
| Communication | Wi-Fi (802.11 b/g/n), USB Serial, RS485 |
| Firmware | Arduino C++ (ESP-IDF) |
| Dashboard | HTML5, CSS3, Vanilla JavaScript |
| Charts | Custom SVG (no external libraries) |
| Data Format | JSON over HTTP REST / Serial |
| Deployment | GitHub Pages / ESP32 Self-hosted |

---

## 👥 Team

| Name | Role |
|------|------|
| Kartik Baghel | Developer & IoT Integration |

**Institution:** MITS-DU, Gwalior — Centre for IoT

---

## 📄 License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.

---

<div align="center">

**Made with ❤️ for Indian Farmers**

*Designed and Developed at Centre for IoT, MITS-DU, Gwalior*

</div>
