#include <ArduinoJson.h>

enum BoardStateRequestType {
  FULL_STATE,
  ACTIONABLE_BLOCK
};

void onReceiveStateRequest(BoardStateRequestType type);
void getState(BoardStateRequestType type, JsonDocument& doc);

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
  if (Serial.available() > 0) {
    onReceiveStateRequest((BoardStateRequestType) Serial.read());
  }
}

void onReceiveStateRequest(BoardStateRequestType type) {
  DynamicJsonDocument doc(1536);
  getState(type, doc);
  serializeJson(doc, Serial);
}

void getState(BoardStateRequestType type, JsonDocument& doc) {
  // example:
  JsonObject doc_0 = doc.createNestedObject();
  doc_0["type"] = "statement";
  doc_0["name"] = "move backward";
  doc_0["argument"] = 255;

  // TODO fill with actual board state data
  if (type == FULL_STATE) {

  } else {

  }
}
