#include <ArduinoJson.h>

void onReceiveCommand(JsonDocument& doc);
void onRobotDone();
void loadSensorData(JsonDocument& doc);

void setup() {
  Serial.begin(9600);
}

void loop() {
  StaticJsonDocument<96> doc;

  DeserializationError error = deserializeJson(doc, Serial);

  if (!error) {
    onReceiveCommand(doc);
  }
}

void onReceiveCommand(JsonDocument& doc) {
  const char* blockName = doc["name"];
  int argVal = doc["argument"];

  // TODO actual robot stuff
  delay(1000); // simulated robot stuff

  onRobotDone();
}

void onRobotDone() {
  StaticJsonDocument<64> doc;
  loadSensorData(doc);
  serializeJson(doc, Serial);
}

void loadSensorData(JsonDocument& doc) {
  // TODO read actual sensor values
  doc["close to wall"] = true;
  doc["red"] = false;
  doc["green"] = true;
  doc["blue"] = true;
}
