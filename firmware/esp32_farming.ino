/* Fieldwise ESP32 — MITS-DU IoT Irrigation & Monitoring Node
 * Full integration: RS485 MST + NPK, DHT22, Soil Moisture, pH, Relay/Pump
 * Outputs JSON telemetry for Fieldwise dashboard (index8.html)
 * Supports USB Serial + Wi-Fi connections
 * 
 * Dependencies: ArduinoJson 7, DHT sensor library (Adafruit), Adafruit Unified Sensor
 */
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <HardwareSerial.h>
#include <esp_timer.h>
#include "Config.h"
#include "Dashboard.h"

// =====================================================
// RS485 Serial
// =====================================================
HardwareSerial RS485Serial(2);

// =====================================================
// DHT22
// =====================================================
DHT dht(DHT_PIN, DHT22);

// =====================================================
// Web Server
// =====================================================
WebServer server(80);
String apName;

// =====================================================
// Serial buffer for USB commands
// =====================================================
String serialBuffer;
bool discardSerial = false;

// =====================================================
// Sensor Data (global state)
// =====================================================
float airTemperature = NAN;
float airHumidity = NAN;
float soilMoisturePercent = NAN;
float soilPH = NAN;
float soilEC = NAN;            // from MST sensor via RS485
float mstTemperature = NAN;    // soil temp from MST
float mstMoisture = NAN;       // soil moisture from MST
int nitrogen = -1;
int phosphorus = -1;
int potassium = -1;
int soilRawADC = -1;
int phRawADC = -1;
float phVoltage = NAN;
uint32_t lastSample = 0;

// =====================================================
// Pump State
// =====================================================
portMUX_TYPE pumpMutex = portMUX_INITIALIZER_UNLOCKED;
bool pumpOn = false;
uint32_t pumpStarted = 0;
esp_timer_handle_t cutoffTimer = nullptr;

bool reportedPump() {
  portENTER_CRITICAL(&pumpMutex);
  bool on = pumpOn;
  portEXIT_CRITICAL(&pumpMutex);
  return on;
}

void setPump(bool on) {
  if (RELAY_PIN < 0) return;
  portENTER_CRITICAL(&pumpMutex);
  if (on && !pumpOn) pumpStarted = millis();
  pumpOn = on;
  digitalWrite(RELAY_PIN, on ? RELAY_ACTIVE_LEVEL : !RELAY_ACTIVE_LEVEL);
  portEXIT_CRITICAL(&pumpMutex);
}

void cutoff(void*) {
  portENTER_CRITICAL(&pumpMutex);
  if (pumpOn && uint32_t(millis() - pumpStarted) >= uint32_t(PUMP_MAX_SECONDS) * 1000) {
    pumpOn = false;
    digitalWrite(RELAY_PIN, !RELAY_ACTIVE_LEVEL);
  }
  portEXIT_CRITICAL(&pumpMutex);
}

// =====================================================
// MODBUS CRC16
// =====================================================
uint16_t modbusCRC(byte *buffer, byte length) {
  uint16_t crc = 0xFFFF;
  for (byte pos = 0; pos < length; pos++) {
    crc ^= buffer[pos];
    for (byte i = 0; i < 8; i++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

// =====================================================
// RS485 SEND REQUEST
// =====================================================
void sendModbusRequest(byte *request, byte length) {
  while (RS485Serial.available()) {
    RS485Serial.read();
  }
  digitalWrite(RS485_DE_RE, HIGH);
  delayMicroseconds(100);
  RS485Serial.write(request, length);
  RS485Serial.flush();
  delayMicroseconds(500);
  digitalWrite(RS485_DE_RE, LOW);
}

// =====================================================
// RS485 READ RESPONSE
// =====================================================
int readModbusResponse(byte *buffer, int bufferSize) {
  int count = 0;
  unsigned long startTime = millis();
  while (millis() - startTime < 1000) {
    if (RS485Serial.available()) {
      byte data = RS485Serial.read();
      if (count < bufferSize) {
        buffer[count++] = data;
      }
    }
  }
  return count;
}

// =====================================================
// READ MST SENSOR (Temperature, Moisture, EC)
// Registers: 0x000B=Temp, 0x000C=Moisture, 0x000D=EC
// =====================================================
void readMSTSensor() {
  mstTemperature = NAN;
  mstMoisture = NAN;
  soilEC = NAN;

  byte request[8];
  request[0] = MST_ADDRESS;
  request[1] = 0x03;
  request[2] = 0x00;
  request[3] = 0x0B;
  request[4] = 0x00;
  request[5] = 0x03;
  uint16_t crc = modbusCRC(request, 6);
  request[6] = crc & 0xFF;
  request[7] = (crc >> 8) & 0xFF;

  sendModbusRequest(request, 8);

  byte buffer[30];
  int count = readModbusResponse(buffer, sizeof(buffer));

  int responseStart = -1;
  for (int i = 0; i <= count - 11; i++) {
    if (buffer[i] == MST_ADDRESS && buffer[i + 1] == 0x03 && buffer[i + 2] == 0x06) {
      responseStart = i;
      break;
    }
  }

  if (responseStart < 0) return;

  byte *r = &buffer[responseStart];
  uint16_t receivedCRC = ((uint16_t)r[10] << 8) | r[9];
  uint16_t calculatedCRC = modbusCRC(r, 9);

  if (receivedCRC != calculatedCRC) return;

  uint16_t rawTemp = ((uint16_t)r[3] << 8) | r[4];
  uint16_t rawMoist = ((uint16_t)r[5] << 8) | r[6];
  uint16_t rawEC = ((uint16_t)r[7] << 8) | r[8];

  mstTemperature = rawTemp / 10.0;
  mstMoisture = rawMoist / 10.0;
  soilEC = rawEC / 100.0;
}

// =====================================================
// READ NPK SENSOR (Nitrogen, Phosphorus, Potassium)
// Registers: 0x0003=N, 0x0004=P, 0x0005=K
// =====================================================
void readNPKSensor() {
  nitrogen = -1;
  phosphorus = -1;
  potassium = -1;

  byte request[8];
  request[0] = NPK_ADDRESS;
  request[1] = 0x03;
  request[2] = 0x00;
  request[3] = 0x03;
  request[4] = 0x00;
  request[5] = 0x03;
  uint16_t crc = modbusCRC(request, 6);
  request[6] = crc & 0xFF;
  request[7] = (crc >> 8) & 0xFF;

  sendModbusRequest(request, 8);

  byte buffer[30];
  int count = readModbusResponse(buffer, sizeof(buffer));

  int responseStart = -1;
  for (int i = 0; i <= count - 11; i++) {
    if (buffer[i] == NPK_ADDRESS && buffer[i + 1] == 0x03 && buffer[i + 2] == 0x06) {
      responseStart = i;
      break;
    }
  }

  if (responseStart < 0) return;

  byte *r = &buffer[responseStart];
  uint16_t receivedCRC = ((uint16_t)r[10] << 8) | r[9];
  uint16_t calculatedCRC = modbusCRC(r, 9);

  if (receivedCRC != calculatedCRC) return;

  nitrogen = ((uint16_t)r[3] << 8) | r[4];
  phosphorus = ((uint16_t)r[5] << 8) | r[6];
  potassium = ((uint16_t)r[7] << 8) | r[8];
}

// =====================================================
// READ DHT22
// =====================================================
void readDHT22Sensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (isfinite(t) && t >= -40 && t <= 80) airTemperature = t;
  else airTemperature = NAN;
  if (isfinite(h) && h >= 0 && h <= 100) airHumidity = h;
  else airHumidity = NAN;
}

// =====================================================
// READ ANALOG SOIL MOISTURE
// =====================================================
void readSoilMoisture() {
  soilRawADC = analogRead(SOIL_PIN);
  // 4095 = dry, 0 = wet (as per your kit)
  int percent = map(soilRawADC, SOIL_DRY, SOIL_WET, 0, 100);
  soilMoisturePercent = constrain(percent, 0, 100);
}

// =====================================================
// READ pH SENSOR
// =====================================================
void readPHSensor() {
  phRawADC = analogRead(PH_PIN);
  phVoltage = (phRawADC / 4095.0) * 3.3;
  // Linear calibration: pH7 → PH7_VOLTAGE, pH4 → PH4_VOLTAGE
  float value = 7.0 + (phVoltage - PH7_VOLTAGE) * (4.0 - 7.0) / (PH4_VOLTAGE - PH7_VOLTAGE);
  soilPH = constrain(value, 0.0f, 14.0f);
}

// =====================================================
// SAMPLE ALL SENSORS
// =====================================================
void sampleAllSensors() {
  readMSTSensor();
  delay(200);
  readNPKSensor();
  delay(200);
  readDHT22Sensor();
  delay(200);
  readSoilMoisture();
  delay(200);
  readPHSensor();
}

// =====================================================
// AUTO PUMP CONTROL (based on soil moisture)
// =====================================================
void autoPumpControl() {
  if (RELAY_PIN < 0) return;
  if (!isfinite(soilMoisturePercent)) return;
  
  int moisture = (int)soilMoisturePercent;
  if (moisture < MOISTURE_ON_THRESHOLD) {
    setPump(true);
  } else if (moisture >= MOISTURE_OFF_THRESHOLD) {
    setPump(false);
  }
}

// =====================================================
// JSON TELEMETRY (for dashboard)
// =====================================================
void numberOrNull(JsonDocument& doc, const char* key, float value) {
  if (isfinite(value)) doc[key] = value;
  else doc[key] = nullptr;
}

String telemetry() {
  JsonDocument doc;
  doc["type"] = "telemetry";
  doc["protocol"] = "fieldwise-v1";
  doc["uptimeMs"] = millis();

  // Use DHT22 air temperature for dashboard
  numberOrNull(doc, "temperature", airTemperature);
  numberOrNull(doc, "humidity", airHumidity);

  // Soil moisture from analog sensor
  numberOrNull(doc, "moisture", soilMoisturePercent);

  // pH from analog sensor
  numberOrNull(doc, "ph", soilPH);

  // NPK from RS485
  if (nitrogen >= 0) doc["nitrogen"] = nitrogen; else doc["nitrogen"] = nullptr;
  if (phosphorus >= 0) doc["phosphorus"] = phosphorus; else doc["phosphorus"] = nullptr;
  if (potassium >= 0) doc["potassium"] = potassium; else doc["potassium"] = nullptr;

  // EC from MST RS485 sensor
  numberOrNull(doc, "ec", soilEC);

  // Pump state
  if (RELAY_PIN >= 0) doc["pumpOn"] = reportedPump();
  else doc["pumpOn"] = nullptr;

  // Raw values for diagnostics
  doc["raw"]["soilADC"] = soilRawADC;
  doc["raw"]["phMilliVolts"] = phRawADC >= 0 ? (int)(phVoltage * 1000) : -1;
  doc["raw"]["ecMilliVolts"] = -1;  // EC comes via RS485, not analog

  // Extra MST data (direct assignment — can't pass proxy as JsonDocument&)
  JsonObject mst = doc["mst"].to<JsonObject>();
  if (isfinite(mstTemperature)) mst["temperature"] = mstTemperature; else mst["temperature"] = nullptr;
  if (isfinite(mstMoisture)) mst["moisture"] = mstMoisture; else mst["moisture"] = nullptr;
  if (isfinite(soilEC)) mst["ec"] = soilEC; else mst["ec"] = nullptr;

  String out;
  serializeJson(doc, out);
  return out;
}

String deviceInfo() {
  JsonDocument doc;
  doc["protocol"] = "fieldwise-v1";
  doc["name"] = apName;
  doc["firmware"] = "Fieldwise MITS-DU Kit 2.0";
  doc["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  doc["hotspotIP"] = WiFi.softAPIP().toString();
  doc["stationConnected"] = WiFi.status() == WL_CONNECTED;
  doc["pins"]["dht"] = DHT_PIN;
  doc["pins"]["soil"] = SOIL_PIN;
  doc["pins"]["ph"] = PH_PIN;
  doc["pins"]["relay"] = RELAY_PIN;
  doc["pins"]["rs485_rx"] = RS485_RX;
  doc["pins"]["rs485_tx"] = RS485_TX;
  doc["pins"]["rs485_de_re"] = RS485_DE_RE;
  doc["modbus"]["mst_address"] = MST_ADDRESS;
  doc["modbus"]["npk_address"] = NPK_ADDRESS;
  doc["maxPumpSeconds"] = PUMP_MAX_SECONDS;
  doc["rawCalibrationMode"] = RAW_CALIBRATION_MODE;
  String out;
  serializeJson(doc, out);
  return out;
}

// =====================================================
// CORS + HTTP helpers
// =====================================================
bool originAllowed() {
  String origin = server.header("Origin");
  if (origin.length() == 0) return true;
  if (String(ALLOWED_ORIGIN) == "*") return true;
  String own = "http://" + server.hostHeader();
  return origin == own || origin == String(ALLOWED_ORIGIN);
}

void cors() {
  String origin = server.header("Origin");
  if (origin.length() && originAllowed()) {
    server.sendHeader("Access-Control-Allow-Origin", origin);
    server.sendHeader("Vary", "Origin");
  }
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");
  server.sendHeader("Cache-Control", "no-store");
}

void jsonResponse(int status, const String& data) {
  cors();
  server.send(status, "application/json", data);
}

String errorBody(const char* message) {
  JsonDocument doc;
  doc["type"] = "error";
  doc["message"] = message;
  String out;
  serializeJson(doc, out);
  return out;
}

// =====================================================
// PUMP COMMAND HANDLER
// =====================================================
struct RecentCommand { String id; bool on = false; };
RecentCommand recent[16];
size_t recentIndex = 0;

String processPump(const String& body, bool http, int& status) {
  status = 400;
  if (body.length() > 1024) return errorBody("Command too large");
  JsonDocument doc;
  if (deserializeJson(doc, body)) return errorBody("Invalid JSON");
  if (!doc["type"].is<const char*>() || String(doc["type"].as<const char*>()) != "pump" ||
      !doc["id"].is<const char*>() || !doc["on"].is<bool>())
    return errorBody("Expected pump type, string id and boolean on");

  String id = doc["id"].as<String>();
  if (id.length() == 0 || id.length() > 80) return errorBody("Invalid command ID");

  String token = http ? server.header("Authorization") : String(doc["token"] | "");
  if (http && token.startsWith("Bearer ")) token = token.substring(7);
  else if (http) token = "";

  if (strlen(CONTROL_TOKEN) < 12 || token != String(CONTROL_TOKEN)) {
    status = 401;
    return errorBody("Pump control token required");
  }
  if (RELAY_PIN < 0 || cutoffTimer == nullptr) {
    status = 409;
    return errorBody("Relay disabled or cutoff unavailable");
  }

  bool requested = doc["on"].as<bool>();
  bool seen = false;
  for (const auto& entry : recent)
    if (entry.id == id) {
      if (entry.on != requested) { status = 409; return errorBody("Command ID reused with different state"); }
      seen = true; break;
    }
  if (!seen) {
    setPump(requested);
    recent[recentIndex].id = id;
    recent[recentIndex].on = requested;
    recentIndex = (recentIndex + 1) % 16;
  }

  JsonDocument ack;
  ack["type"] = "ack";
  ack["id"] = id;
  ack["pumpOn"] = reportedPump();
  String out;
  serializeJson(ack, out);
  status = 200;
  return out;
}

// =====================================================
// SETUP
// =====================================================
void setup() {
  // Relay OFF first
  if (RELAY_PIN >= 0) {
    digitalWrite(RELAY_PIN, !RELAY_ACTIVE_LEVEL);
    pinMode(RELAY_PIN, OUTPUT);
    setPump(false);
  }

  Serial.begin(115200);
  serialBuffer.reserve(1024);

  // RS485 setup
  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);
  RS485Serial.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);

  // Pump cutoff timer
  if (RELAY_PIN >= 0) {
    esp_timer_create_args_t args = {};
    args.callback = cutoff;
    args.name = "pump-cutoff";
    if (esp_timer_create(&args, &cutoffTimer) != ESP_OK)
      cutoffTimer = nullptr;
    else if (esp_timer_start_periodic(cutoffTimer, 100000) != ESP_OK) {
      esp_timer_delete(cutoffTimer);
      cutoffTimer = nullptr;
    }
  }

  // ADC setup
  analogReadResolution(12);
  if (SOIL_PIN >= 0) { pinMode(SOIL_PIN, INPUT); analogSetPinAttenuation(SOIL_PIN, ADC_11db); }
  if (PH_PIN >= 0) { pinMode(PH_PIN, INPUT); analogSetPinAttenuation(PH_PIN, ADC_11db); }

  // DHT22
  if (DHT_PIN >= 0) dht.begin();

  // Wi-Fi: AP + optional STA
  WiFi.mode(WIFI_AP_STA);
  apName = "Fieldwise-" + String(uint32_t(ESP.getEfuseMac()) & 0xffff, HEX);
  WiFi.softAP(apName.c_str(), AP_PASSWORD);
  if (strlen(WIFI_SSID) > 0) WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // HTTP server
  static const char* headers[] = {"Authorization", "Origin"};
  server.collectHeaders(headers, 2);

  server.on("/", HTTP_GET, [] {
    server.sendHeader("Content-Encoding", "gzip");
    server.send_P(200, "text/html; charset=utf-8",
      reinterpret_cast<const char*>(DASHBOARD_GZ), sizeof(DASHBOARD_GZ));
  });

  server.on("/api/info", HTTP_GET, [] {
    if (!originAllowed()) { server.send(403, "application/json", errorBody("Origin denied")); return; }
    jsonResponse(200, deviceInfo());
  });

  server.on("/api/sensors", HTTP_GET, [] {
    if (!originAllowed()) { server.send(403, "application/json", errorBody("Origin denied")); return; }
    jsonResponse(200, telemetry());
  });

  server.on("/api/pump", HTTP_POST, [] {
    if (!originAllowed()) { server.send(403, "application/json", errorBody("Origin denied")); return; }
    int status;
    String reply = processPump(server.arg("plain"), true, status);
    jsonResponse(status, reply);
  });

  server.onNotFound([] {
    if (server.method() == HTTP_OPTIONS && originAllowed()) { cors(); server.send(204); return; }
    jsonResponse(404, errorBody("Endpoint not found"));
  });

  server.begin();

  delay(2000);

  // Initial reading
  sampleAllSensors();
  lastSample = millis();
  Serial.println(telemetry());
}

// =====================================================
// LOOP
// =====================================================
void loop() {
  server.handleClient();

  // Process USB serial commands (pump commands from browser)
  size_t budget = 256;
  while (Serial.available() && budget--) {
    char ch = Serial.read();
    if (ch == '\n') {
      if (!discardSerial && serialBuffer.length()) {
        int status;
        Serial.println(processPump(serialBuffer, false, status));
      }
      serialBuffer = "";
      discardSerial = false;
    } else if (ch != '\r' && !discardSerial) {
      if (serialBuffer.length() >= 1024) {
        serialBuffer = "";
        discardSerial = true;
      } else {
        serialBuffer += ch;
      }
    }
  }

  // Sample sensors every 2.5 seconds
  if (uint32_t(millis() - lastSample) >= 2500) {
    lastSample = millis();
    sampleAllSensors();
    // Auto pump control based on moisture
    autoPumpControl();
    // Send JSON telemetry over USB Serial
    Serial.println(telemetry());
  }

  delay(1);
}
