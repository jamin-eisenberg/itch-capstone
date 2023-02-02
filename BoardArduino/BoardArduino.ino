#include <ArduinoJson.h>

void getState();

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  StaticJsonDocument<64> doc;
  doc["name"] = "move forward";
  doc["argument"] = 123;

  Serial.print("Sending JSON: ");
  serializeJson(doc, Serial);
  serializeJson(doc, Serial1);
  Serial.println();
}

void loop() {
  if (Serial1.available() > 0) {
    while (Serial1.available() > 0) {
      Serial1.read();
    }
    getState();
  }
}


void getState() {
  StaticJsonDocument<2000> doc; // CHANGE SIZE

  JsonObject root = doc.to<JsonObject>();
  JsonArray state = root.createNestedArray("state");

  JsonObject block1 = state.createNestedObject();
  block1["type"] = "conditional";
  block1["name"] = "if";
  block1["argument"] = "isBlue";

  JsonObject block2 = state.createNestedObject();
  block2["type"] = "statement";
  block2["name"] = "move backward";
  block2["argument"] = 213;

  root["inactivity duration"] = 2311;
  serializeJson(root, Serial);
  serializeJson(root, Serial1);
}
