#include <ArduinoJson.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// IMPORTANT:
// TODO question everything done so far and consider making all APIs REST APIs with web servers for unification (still need softAP so board and robot can be on the same network)

// Set WiFi credentials
// TODO change to Pi creds
#define WIFI_SSID "Wendys Customers"
#define WIFI_PASS "123Dogfood!"

// Set AP credentials
#define AP_SSID "red-board"
#define AP_PASS "capstone"

#define CONSOLE_HOST_NAME "consolehost.local:5000"

ESP8266WebServer server(80);

WiFiClient client;
HTTPClient http;

enum BoardStateRequestType {
  FULL_STATE,
  ACTIONABLE_BLOCK
};

void onReceiveStateRequest(BoardStateRequestType type);
void onReceiveEnabledMsg(bool enable);

void setup()
{
  // Setup serial port
  Serial.begin(115200);
  Serial.println();

  // Begin Access Point
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);

  // Begin WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  //  serverSetup();
  //  registerWithConsoleHost();
}

void loop() {
  server.handleClient();
}

void onReceiveStateRequest(BoardStateRequestType type) {
  Serial.write(type);
}

void onReceiveEnabledMsg(bool enable) {
  StaticJsonDocument<64> doc;

  doc["name"] = enable ? "enable" : "disable";
  doc["argument"] = nullptr;

  serializeJson(doc, Serial);
}
