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

#define CONSOLE_HOST_NAME "consolehost:5000"

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
  Serial.begin(115200);
  Serial.println();

  // Begin Access Point
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, AP_HIDDEN, AP_MAX_CONNECTED_STATIONS);

  // Begin WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  serverSetup();
  registerWithConsoleHost();
}

void loop() {
  delay(2000);

  StaticJsonDocument<64> doc;

  doc["name"] = "move forward";
  doc["argument"] = 123;
  
  postToConnectedClient(doc);

  server.handleClient();
}


void postToConnectedClient(JsonDocument& doc) {

  struct station_info *stat_info;

  struct ip4_addr *IPaddress;
  IPAddress address;

  stat_info = wifi_softap_get_station_info();

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
    http.POST(json);

    stat_info = STAILQ_NEXT(stat_info, next);
  }
}

//void getConnectedStation() {
//  struct station_info *stat_info;
//
//  //  while (stat_info != NULL) {
//  // just get the first IP address, there should only be one
//  struct ip4_addr *IPaddress;
//  IPAddress address;
//
//  stat_info = wifi_softap_get_station_info();
//
//  IPaddress = &stat_info->ip;
//  address = IPaddress->addr;
//
//
//  Serial.print(" IP adress is = ");
//  Serial.print((address));
//
//
//  //    stat_info = STAILQ_NEXT(stat_info, next);
//  //  }
//}


void onReceiveStateRequest(BoardStateRequestType type) {
  //  Serial.write(type);
  server.send(200, "text/plain", "");
}

void onReceiveEnabledMsg(bool enable) {
  StaticJsonDocument<64> doc;

  doc["name"] = enable ? "enable" : "disable";
  doc["argument"] = nullptr;

  serializeJson(doc, Serial);
}
