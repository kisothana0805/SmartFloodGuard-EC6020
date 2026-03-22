#include <WiFi.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>

HardwareSerial mySerial(1);

#define RX_PIN 16
#define TX_PIN 17

// Wi-Fi
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// Backend
const char* serverUrl = "http://192.168.8.103/flood_monitor/api/push_reading.php";
const char* deviceId  = "DEV001";

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
void setup() {
  Serial.begin(9600);

  mySerial.setRxBufferSize(1024);
  mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  connectWiFi();

  sendToServer(0, 0, 0, 0);
  sentDepth = 0;
  sentHumidity = 0;
  sentWater = 0;
  sentRelay = 0;
  lastPostMs = millis();
}

// -------------------------------------------------
void loop() {
  readUARTFast();

  unsigned long now = millis();

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
}
