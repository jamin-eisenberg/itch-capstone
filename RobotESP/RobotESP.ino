#include <ArduinoJson.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>

// WIRING for mega:
// ESP -> Arduino
// D4 -> Rx1
// D7 -> Tx1

// Set WiFi credentials
#define WIFI_SSID "red-board"
#define WIFI_PASS "capstone"

#define BOARD_IP "192.168.4.1"
#define AP_CHANNEL 8

bool robotEnabled = true;
ESP8266WebServer server(80);
SoftwareSerial* logger = nullptr;

HTTPClient http;
WiFiClient client;


void handleIncomingCommand();

void setup() {
  // Setup serial port
  Serial.begin(9600);

  delay(500);
  Serial.swap();
  delay(500);
  logger = new SoftwareSerial(3, 1);
  logger->begin(9600);
  logger->println("\n\nUsing SoftwareSerial for logging");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS, AP_CHANNEL, NULL, true);
  logger->print("Connecting to ");
  logger->print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    logger->print(".");
  }
  logger->print("\nIP address: ");
  logger->println(WiFi.localIP());

  server.on("/", HTTP_POST, handleIncomingCommand);
  server.begin();
}

void loop() {
  if (Serial.available()) {
    StaticJsonDocument<128> doc;

    DeserializationError error = deserializeJson(doc, Serial);

    if (error) {
      logger->print(F("deserializeJson() failed: "));
      logger->println(error.f_str());
      return;
    }

    String jsonStr;
    serializeJson(doc, jsonStr);
    logger->print("Sending sensor data to server: ");
    logger->println(jsonStr);

    String url = "http://" BOARD_IP "/sensordata";

    int statusCode = 0;
    int retries = 0;
    do {
      if (statusCode < 0 && statusCode != -11) {
        logger->print("Failure with code ");
        logger->print(statusCode);
        logger->println(" Retrying....");
      }
      http.begin(client, url);
      http.addHeader("Content-Type", "application/json");

      statusCode = http.POST(jsonStr);
      retries++;
    } while (statusCode < 0 && retries < 10);
    int responseSize = http.getSize();

    if (responseSize > 0) {
      auto res = http.getString().c_str();
      //      char buffer[responseSize + 10];
      //      snprintf(buffer, responseSize + 10, "%d: %s", statusCode, res);
      logger->print("Board responded with ");
      logger->print(statusCode);
      logger->print(": ");
      logger->print(res);
      logger->print(", retries required: ");
      logger->println(retries);
    } else {
      logger->print("Board responded with ");
      logger->print(statusCode);
      logger->println(", with no response body");
      logger->println();
    }
  }

  server.handleClient();
}

void handleIncomingCommand() {
  server.keepAlive(true);
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

  if (strcmp(doc["name"], "enable") == 0) {
    robotEnabled = true;
    server.send(200, "text/plain", "robot enabled");
    return;
  }
  if (strcmp(doc["name"], "disable") == 0) {
    robotEnabled = false;
    server.send(200, "text/plain", "robot disabled");
    doc["name"] = "stop";
  } else if (!robotEnabled) {
    server.send(409, "text/plain", "robot currently disabled");
    return;
  }

  logger->print("Sending to robot: ");
  serializeJson(doc, *logger);
  logger->println();

  serializeJson(doc, Serial);

  server.send(200, "text/plain", "command executing...");
}
