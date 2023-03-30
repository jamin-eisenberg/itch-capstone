#include <ArduinoJson.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>

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

SoftwareSerial* logger = nullptr;
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
  // Setup hardware serial port
  Serial.begin(9600);
  delay(500);
  Serial.swap();
  delay(500);
  logger = new SoftwareSerial(3, 1);
  logger->begin(9600);
  logger->println("\n\nUsing SoftwareSerial for logging");

  // Begin Access Point
  WiFi.mode(WIFI_AP_STA);
  logger->print("Beginning Accesss Point at ");
  logger->println(AP_SSID);
  WiFi.softAP(AP_SSID, AP_PASS, AP_CHANNEL, AP_HIDDEN, AP_MAX_CONNECTED_STATIONS);

  // Begin WiFi
  WiFi.hostname(AP_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  serverSetup();
  registerWithConsoleHost();
}

void loop() {
  if (Serial.available()) {
    StaticJsonDocument<2000> doc;

    DeserializationError error = deserializeJson(doc, Serial);

    if (error) {
      logger->print(F("deserializeJson() failed: "));
      logger->println(error.f_str());
      return;
    }

    if (!doc.containsKey("inactivity duration")) {
      auto response = postToConnectedClient(doc);
      if (response.statusCode == 200) {
        logger->print("Robot initial response: ");
        logger->println(response.data);
      } else {
        logger->print("Response from client: ");
        logger->print(response.statusCode);
        logger->print(": ");
        logger->println(response.data);
      }
    } else {
      String json;
      serializeJson(doc, json);
      logger->println("Recieved full board state");
      logger->println(json);
      Serial.println("Sending out on server, do not send to robot.");
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
    //    http.setTimeout(10000);

    //    String json;
    //    serializeJson(doc, json);
    //    logger->print("Posting JSON to client: ");
    //    logger->println(json);

    int statusCode = http.POST(json);
    int responseSize = http.getSize();

    if (responseSize > 0) {
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
  logger->println("received full state request");
  Serial.write(1); // just trigger the board arduino (void message)
}

void onReceiveSensorData() {
  logger->println("received robot sensor data");

  if (!server.hasArg("plain")) { //Check if body received
    server.send(400, "text/plain", "Body not received");
    return;
  }

  String incoming = server.arg("plain");
  logger->print("Incoming message from server: ");
  logger->println(incoming);

  StaticJsonDocument<128> doc;

  DeserializationError error = deserializeJson(doc, incoming);

  if (error) {
    server.send(400, "text/plain", error.c_str());
    return;
  }

  logger->print("Sending to board: ");
  serializeJson(doc, *logger);
  logger->println();

  serializeJson(doc, Serial);
}

void onReceiveEnabledMsg(bool enable) {
  StaticJsonDocument<64> doc;

  doc["name"] = enable ? "enable" : "disable";
  doc["argument"] = nullptr;

  auto msg = postToConnectedClient(doc);

  server.send(200, "text/plain", msg.data);
}
