#include <ArduinoJson.h>

char buffer[50];

// Serial Tx = to computer
// Serial Rx = from Mega
// Serial1 Tx = to Mega

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.swap();
  Serial1.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
//  if (Serial.available()) {
//    StaticJsonDocument<128> doc;
//
//    DeserializationError error = deserializeJson(doc, Serial);
//
//    if (error) {
//      Serial.print(F("deserializeJson() failed: "));
//      Serial.println(error.f_str());
//      return;
//    }
//
//    const char* type = doc["type"]; // "conditional"
//    const char* name = doc["name"]; // "move backward"
//    int argument = doc["argument"]; // 123
//
//    Serial.print(type);
//    Serial.print("\t");
//    Serial.print(name);
//    Serial.print("\t");
//    Serial.println(argument);
//  }
  Serial.println("0Hello");
  Serial1.println("1Hello");
}
