#include <ArduinoJson.h>

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

bool robotEnabled = true;
ESP8266WebServer server(80);
SoftwareSerial* logger = nullptr;

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
  WiFi.begin(WIFI_SSID, WIFI_PASS);
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
      server.send(500, "text/plain", "robot sent invalid JSON response");
      return;
    }

    String jsonStr;
    serializeJson(doc, jsonStr);
    logger->print("Sending sensor data back to server: ");
    logger->println(jsonStr);
    server.send(200, "application/json", jsonStr);
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
}
