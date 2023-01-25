#include <ArduinoJson.h>

char buffer[50];

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial2.begin(9600);
  Serial.println("sup");

//  delay(1000);
//
//  //  { "type": "conditional", "name": "move backward", "argument": 123 }
//
//  StaticJsonDocument<64> doc;
//
//  doc["typ"] = "conditional";
//  doc["name"] = "move backward";
//  doc["argument"] = 123;
//
//  serializeJson(doc, Serial1);
}

void loop() {
  // put your main code here, to run repeatedly:
//    if (Serial1.available()) {
      sprintf(buffer, "ESP said %c", Serial2.read());
      Serial.println(buffer);
//    }
}
