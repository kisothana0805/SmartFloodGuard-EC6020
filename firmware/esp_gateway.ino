#include <WiFi.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>

HardwareSerial mySerial(1);

#define RX_PIN 16
#define TX_PIN 17
#define RESET_OUT_PIN 25   // ESP32 -> AND gate input, active LOW reset pulse

// Wi-Fi
const char* ssid = "Share";
const char* password = "ajyaga2712";

// Backend
const char* serverUrl      = "http://10.191.31.209/flood_monitor/api/push_reading.php";
const char* pullCommandUrl = "http://10.191.31.209/flood_monitor/api/pull_command.php?device_id=DEV001";
const char* ackCommandUrl  = "http://10.191.31.209/flood_monitor/api/ack_command.php";
const char* deviceId       = "DEV001";

// Latest UART values
volatile int latestDepth = 0;
volatile int latestHumidity = 0;
volatile int latestWater = 0;
volatile int latestRelay = 0;

// Last sent values
int sentDepth = -9999;
int sentHumidity = -9999;
int sentWater = -9999;
int sentRelay = -9999;

// Timing
unsigned long lastPostMs = 0;
const unsigned long postIntervalMs = 250;

unsigned long lastCommandPollMs = 0;
const unsigned long commandPollIntervalMs = 300;

// UART line buffer
String serialBuffer = "";

// -------------------------------------------------
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi OK. IP: ");
  Serial.println(WiFi.localIP());
}

// -------------------------------------------------
void sendToServer(int depth, int humidity, int waterDetected, int relayState) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  int flood_detected = waterDetected ? 1 : 0;
  int height_cm = waterDetected ? depth : 0;
  int drip_triggered = relayState ? 1 : 0;

  String json = "{";
  json += "\"device_id\":\"" + String(deviceId) + "\",";
  json += "\"flood_detected\":" + String(flood_detected) + ",";
  json += "\"height_cm\":" + String(height_cm) + ",";
  json += "\"humidity\":" + String(humidity) + ",";
  json += "\"drip_triggered\":" + String(drip_triggered);
  json += "}";

  HTTPClient http;
  http.setTimeout(400);
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  int code = http.POST(json);
  http.end();

  Serial.print("POST code: ");
  Serial.println(code);
}

// -------------------------------------------------
void processLine(const String &line) {
  int depth = 0, humidity = 0, water = 0, relay = 0;

  if (sscanf(line.c_str(), "D:%d,H:%d,W:%d,R:%d", &depth, &humidity, &water, &relay) == 4) {
    latestDepth = depth;
    latestHumidity = humidity;
    latestWater = water;
    latestRelay = relay;
  }
}

// -------------------------------------------------
void readUARTFast() {
  while (mySerial.available()) {
    char c = (char)mySerial.read();

    if (c == '\n') {
      serialBuffer.trim();
      if (serialBuffer.length() > 0) {
        processLine(serialBuffer);
      }
      serialBuffer = "";
    } else if (c != '\r') {
      serialBuffer += c;

      if (serialBuffer.length() > 80) {
        serialBuffer = "";
      }
    }
  }
}

// -------------------------------------------------
bool stateChangedEnough() {
  return (latestDepth != sentDepth) ||
         (latestHumidity != sentHumidity) ||
         (latestWater != sentWater) ||
         (latestRelay != sentRelay);
}

// -------------------------------------------------
int extractJsonInt(const String &json, const String &key) {
  String token = "\"" + key + "\":";
  int start = json.indexOf(token);
  if (start == -1) return -1;

  start += token.length();
  while (start < json.length() && (json[start] == ' ' || json[start] == '\"')) start++;

  int end = start;
  if (start < json.length() && json[start] == '-') end++;

  while (end < json.length() && isDigit(json[end])) end++;

  if (end <= start) return -1;
  return json.substring(start, end).toInt();
}

String extractJsonString(const String &json, const String &key) {
  String token = "\"" + key + "\":\"";
  int start = json.indexOf(token);
  if (start == -1) return "";

  start += token.length();
  int end = json.indexOf("\"", start);
  if (end == -1) return "";

  return json.substring(start, end);
}

// -------------------------------------------------
void ackCommand(int commandId, const String &status, const String &message) {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  String json = "{";
  json += "\"command_id\":" + String(commandId) + ",";
  json += "\"status\":\"" + status + "\",";
  json += "\"message\":\"" + message + "\"";
  json += "}";

  HTTPClient http;
  http.setTimeout(500);
  http.begin(ackCommandUrl);
  http.addHeader("Content-Type", "application/json");

  int code = http.POST(json);
  http.end();

  Serial.print("ACK code: ");
  Serial.println(code);
}

// -------------------------------------------------
// Active-LOW reset pulse for AND gate logic
void pulseResetOutput() {
  Serial.println("Sending LOW reset pulse on GPIO25...");
  digitalWrite(RESET_OUT_PIN, LOW);   // trigger reset
  delay(300);                         // 300 ms pulse
  digitalWrite(RESET_OUT_PIN, HIGH);  // back to idle
  Serial.println("Reset pulse done.");
}

// -------------------------------------------------
void checkCommands() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  HTTPClient http;
  http.setTimeout(400);
  http.begin(pullCommandUrl);

  int code = http.GET();
  if (code <= 0) {
    http.end();
    return;
  }

  String response = http.getString();
  http.end();

  if (response.indexOf("\"data\":null") != -1) {
    return;
  }

  int commandId = extractJsonInt(response, "id");
  String command = extractJsonString(response, "command");

  if (commandId <= 0 || command == "") {
    return;
  }

  Serial.print("Command received: ");
  Serial.print(command);
  Serial.print(" (ID=");
  Serial.print(commandId);
  Serial.println(")");

  if (command == "BUZZER_RESET") {
    pulseResetOutput();
    ackCommand(commandId, "EXECUTED", "LOW reset pulse sent from ESP32");
  } else {
    ackCommand(commandId, "FAILED", "Unknown command");
  }
}

// -------------------------------------------------
void setup() {
  Serial.begin(9600);

  mySerial.setRxBufferSize(1024);
  mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  pinMode(RESET_OUT_PIN, OUTPUT);
  digitalWrite(RESET_OUT_PIN, HIGH);   // idle HIGH for active-LOW reset logic

  connectWiFi();

  sendToServer(0, 0, 0, 0);
  sentDepth = 0;
  sentHumidity = 0;
  sentWater = 0;
  sentRelay = 0;
  lastPostMs = millis();
  lastCommandPollMs = millis();
}

// -------------------------------------------------
void loop() {
  readUARTFast();

  unsigned long now = millis();

  // Existing sensor update logic unchanged
  if (stateChangedEnough() && (now - lastPostMs >= postIntervalMs)) {
    sendToServer(latestDepth, latestHumidity, latestWater, latestRelay);

    sentDepth = latestDepth;
    sentHumidity = latestHumidity;
    sentWater = latestWater;
    sentRelay = latestRelay;
    lastPostMs = now;
  }

  if ((now - lastPostMs) >= 2000) {
    sendToServer(latestDepth, latestHumidity, latestWater, latestRelay);
    lastPostMs = now;
  }

  // Command polling for reset button
  if ((now - lastCommandPollMs) >= commandPollIntervalMs) {
    checkCommands();
    lastCommandPollMs = now;
  }
}
