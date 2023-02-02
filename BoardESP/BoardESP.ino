#include <ArduinoJson.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Set WiFi credentials
#define WIFI_SSID "itch"
#define WIFI_PASS "capstone"

// Set AP credentials
#define AP_SSID "red-board"
#define AP_PASS "capstone"

#define AP_CHANNEL 1
#define AP_HIDDEN 0
#define AP_MAX_CONNECTED_STATIONS 1

#define CONSOLE_HOST_PORT_STR ":5000"
IPAddress consolehostIP;


ESP8266WebServer server(80);

WiFiClient client;
HTTPClient http;

enum BoardStateRequestType {
  FULL_STATE,
  ACTIONABLE_BLOCK
};


void onReceiveStateRequest(BoardStateRequestType type);
void onReceiveEnabledMsg(bool enable);
void serverSetup();
void registerWithConsoleHost();

void setup()
{
  // Setup serial port
  Serial.begin(9600);
  Serial1.begin(9600);

  // Begin Access Point
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, AP_HIDDEN, AP_MAX_CONNECTED_STATIONS);

  // Begin WiFi
  WiFi.hostname(AP_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  serverSetup();
  registerWithConsoleHost();

  //  Serial.swap();
}

void loop() {
  if (Serial.available()) {
    StaticJsonDocument<128> doc;

    DeserializationError error = deserializeJson(doc, Serial);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    postToConnectedClient(doc);
  }

  server.handleClient();
}


String postToConnectedClient(JsonDocument& doc) {

  struct station_info *stat_info;

  struct ip4_addr *IPaddress;
  IPAddress address;

  stat_info = wifi_softap_get_station_info();

  Serial.print("Received JSON from board: ");
  serializeJson(doc, Serial);
  Serial.println();

  while (stat_info != NULL) {

    IPaddress = &stat_info->ip;
    address = IPaddress->addr;

    Serial.print("Posting JSON payload ");
    serializeJson(doc, Serial);
    Serial.print(" to server ");
    String url = "http://" + address.toString();
    Serial.println(url);

    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    String json;
    serializeJson(doc, json);
    Serial.print("Received server response: ");
    int statusCode = http.POST(json);
    Serial.println(statusCode);
    Serial.println(http.getString());

    char buffer[50];
    sprintf(buffer, "%d: %s", statusCode, http.getString().c_str());
    return buffer;

    stat_info = STAILQ_NEXT(stat_info, next);
  }
}


void onReceiveStateRequest(BoardStateRequestType type) {
  //  Serial.write(type);
  server.send(200, "text/plain", "");
}

void onReceiveEnabledMsg(bool enable) {
  StaticJsonDocument<64> doc;

  doc["name"] = enable ? "enable" : "disable";
  doc["argument"] = nullptr;

  String msg = postToConnectedClient(doc);

  server.send(200, "text/plain", msg);
}
