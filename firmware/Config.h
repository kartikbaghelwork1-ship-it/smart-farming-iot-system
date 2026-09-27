#pragma once
// Generated for MITS-DU IoT Irrigation & Monitoring Node trainer kit.
// Pin mapping verified from actual kit firmware code.
// Contains private credentials; do not share.

// ─── Wi-Fi ───
static const char* WIFI_SSID = "";            // Leave blank = ESP hotspot only
static const char* WIFI_PASSWORD = "";
static const char* AP_PASSWORD = "fieldwise2026";   // ESP hotspot password
static const char* CONTROL_TOKEN = "mits-farming-token-2026";
static const char* ALLOWED_ORIGIN = "*";      // Allow all origins for easy testing

// ─── Mode ───
static constexpr bool RAW_CALIBRATION_MODE = false;  // Full mode with all sensors

// ─── GPIO Pin Map (VERIFIED from actual MITS-DU kit code) ───

// DHT22 DATA → GPIO 25
static constexpr int DHT_PIN = 25;

// Soil Moisture Sensor AO → ADC1 pin GPIO 34
static constexpr int SOIL_PIN = 34;

// pH Sensor Analog Output → ADC1 pin GPIO 35
static constexpr int PH_PIN = 35;

// EC Sensor — read via RS485 MST sensor, not analog
// Set to -1 because EC comes from Modbus, not direct ADC
static constexpr int EC_PIN = -1;

// Relay Module IN → GPIO 26
static constexpr int RELAY_PIN = 26;

// Relay active level: 0 = Active LOW (your kit uses LOW to turn ON)
static constexpr int RELAY_ACTIVE_LEVEL = 0;

// ─── RS485 Pins ───
static constexpr int RS485_RX = 16;
static constexpr int RS485_TX = 17;
static constexpr int RS485_DE_RE = 4;

// ─── Modbus Addresses ───
static constexpr uint8_t MST_ADDRESS = 0x01;   // Temperature, Moisture, EC
static constexpr uint8_t NPK_ADDRESS = 0x55;   // Nitrogen, Phosphorus, Potassium

// ─── Soil Moisture Calibration ───
// ADC: 4095 = dry, 0 = wet (from your code)
static constexpr int SOIL_DRY = 4095;
static constexpr int SOIL_WET = 0;

// ─── pH Calibration (voltage-based from your code) ───
static constexpr float PH7_VOLTAGE = 2.50;   // Voltage at pH 7 buffer
static constexpr float PH4_VOLTAGE = 3.00;   // Voltage at pH 4 buffer
// Legacy mV fields (unused, kept for compatibility)
static constexpr float PH4_MV = 0;
static constexpr float PH7_MV = 0;

// ─── Pump Safety ───
static constexpr int PUMP_MAX_SECONDS = 30;

// ─── Moisture Thresholds for Auto Pump ───
static constexpr int MOISTURE_ON_THRESHOLD = 30;   // Below this → pump ON
static constexpr int MOISTURE_OFF_THRESHOLD = 35;   // Above this → pump OFF
