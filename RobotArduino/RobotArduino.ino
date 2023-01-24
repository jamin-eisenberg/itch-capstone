#include <ArduinoJson.h>

void onReceiveCommand(JsonDocument& doc);
void onRobotDone();
void loadSensorData(JsonDocument& doc);

// TODO change Serial1 to Serial and remove debug output

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
}

void loop() {
  if (Serial1.available()) {
    Serial.println("Receiving message");
    StaticJsonDocument<96> doc;

    DeserializationError error = deserializeJson(doc, Serial1);

    if (!error) {
      onReceiveCommand(doc);
    } else {
      Serial.println(error.c_str());
    }
  }
}

void onReceiveCommand(JsonDocument& doc) {
  serializeJson(doc, Serial);
  const char* blockName = doc["name"];
  int argVal = doc["argument"];

  // TODO actual robot stuff
  delay(1000); // simulated robot stuff

  onRobotDone();
}

void onRobotDone() {
  StaticJsonDocument<64> doc;
  loadSensorData(doc);
  serializeJson(doc, Serial1);
}

void loadSensorData(JsonDocument& doc) {
  // TODO read actual sensor values
  doc["close to wall"] = true;
  doc["red"] = false;
  doc["green"] = true;
  doc["blue"] = true;
}
