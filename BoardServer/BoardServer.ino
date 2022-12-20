#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>


// Set WiFi credentials
// TODO change to Pi creds
#define WIFI_SSID "Wendys Customers"
#define WIFI_PASS "123Dogfood!"

// Set AP credentials
#define AP_SSID "red-board"
#define AP_PASS "capstone"

#define CONSOLE_HOST_NAME "consolehost.local:5000"

bool enabled;

ESP8266WebServer server(80);

WiFiClient client;
HTTPClient http;

void setup()
{
  // Setup serial port
  Serial.begin(115200);
  Serial.println();

  // Begin Access Point
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS);

  // Begin WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  serverSetup();

  Serial.print("[HTTP] begin...\n");
  Serial.println("http://" CONSOLE_HOST_NAME "/boards/" AP_SSID);
  http.begin(client, "http://" CONSOLE_HOST_NAME "/boards/" AP_SSID);
  http.addHeader("Content-Type", "text/plain");
  Serial.print("[HTTP] POST...\n");
  int httpCode = http.POST("http://" + WiFi.localIP().toString());

  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] POST... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      const String& payload = http.getString();
      Serial.println("received payload:\n<<");
      Serial.println(payload);
      Serial.println(">>");
    }
  } else {
    Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

void loop() {
  server.handleClient();
}
