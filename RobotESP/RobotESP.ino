#include <ESP8266WiFi.h>

// Set WiFi credentials
#define WIFI_SSID "red-board"
#define WIFI_PASS "capstone"

const char* host = "192.168.4.1";
const uint16_t port = 12345;

WiFiClient client;

void onReceiveWiFiData(byte b);
void onReceiveSerialData(byte b);

void setup() {
  // Setup serial port
  Serial.begin(9600);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
    }
  }

  WiFiClient client;

  if (!client.connect(host, port)) {
    delay(1000);
    return;
  }

  while (client.connected()) {
    if (client.available() > 0) {
      onReceiveWiFiData(client.read());
    }
    if (Serial.available() > 0) {
      onReceiveSerialData(Serial.read());
    }
  }
}

void onReceiveWiFiData(byte b) {
  Serial.write(b);
}

void onReceiveSerialData(byte b) {
  Serial.write(b);
}
