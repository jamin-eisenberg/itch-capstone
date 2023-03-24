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

void onReceiveStateRequest();
void onReceiveEnabledMsg(bool enable);
void serverSetup();
void registerWithConsoleHost();

/**
   Generic HTTP response struct.
*/
struct Response {
  int statusCode;
  const char* data;
};

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

  Serial.swap();
}

void loop() {
  if (Serial.available()) {
    StaticJsonDocument<2000> doc;

    DeserializationError error = deserializeJson(doc, Serial);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    if (!doc.containsKey("inactivity duration")) {
      auto response = postToConnectedClient(doc);
      if (response.statusCode == 200) {
        Serial.print(response.data);
        Serial1.print(response.data);
      } else {
        Serial.print("Response from client: ");
        Serial.print(response.statusCode);
        Serial.print(": ");
        Serial.println(response.data);
      }
    } else {
      String json;
      serializeJson(doc, json);
      //      Serial.println("Recieved full board state");
      //      Serial.println(json);
      server.send(200, "application/json", json);
    }
  }

  server.handleClient();
}

Response postToConnectedClient(JsonDocument& doc) {

  struct station_info *stat_info;

  struct ip4_addr *IPaddress;
  IPAddress address;

  stat_info = wifi_softap_get_station_info();

  while (stat_info != NULL) {

    IPaddress = &stat_info->ip;
    address = IPaddress->addr;

    String url = "http://" + address.toString();

    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000);

    String json;
    serializeJson(doc, json);
    //    Serial.print("Posting JSON to client: ");
    //    Serial.println(json);

    int statusCode = http.POST(json);
    int responseSize = http.getSize();

    if (http.getSize() > 0) {
      auto res = http.getString().c_str();
      //      char buffer[responseSize + 10];
      //      snprintf(buffer, responseSize + 10, "%d: %s", statusCode, res);
      return Response {statusCode, res};
    } else {
      return Response {statusCode, ""};
    }

    stat_info = STAILQ_NEXT(stat_info, next);
  }
  return Response { -1, "No connected stations found!"};
}


void onReceiveStateRequest() {
  //  Serial.println("recevied full state request");
  Serial.write(1); // just trigger the board arduino (void message)
  Serial1.write(1); // just trigger the board arduino (void message)
}

void onReceiveEnabledMsg(bool enable) {
  StaticJsonDocument<64> doc;

  doc["name"] = enable ? "enable" : "disable";
  doc["argument"] = nullptr;

  auto msg = postToConnectedClient(doc);

  server.send(200, "text/plain", msg.data);
}
