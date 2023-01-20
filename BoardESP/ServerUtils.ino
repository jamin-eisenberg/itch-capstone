void handleNotFound() {
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void serverSetup() {
  // Connecting to WiFi...
  Serial.print("Connecting to ");
  Serial.print(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }

  // Connected to WiFi
  Serial.println();
  Serial.println("Connected!");
  Serial.print("IP address for network ");
  Serial.print(WIFI_SSID);
  Serial.print(" : ");
  Serial.println(WiFi.localIP());
  Serial.print("IP address for network ");
  Serial.print(AP_SSID);
  Serial.print(" : ");
  Serial.print(WiFi.softAPIP());

  server.on("/", HTTP_GET, getFullBoardState);
  server.on("/", HTTP_PATCH, updateEnabledStatus);
  server.onNotFound(handleNotFound);
  server.begin();
}

void registerWithConsoleHost() {
  Serial.println("Registering with Console Host.");
  Serial.println("POSTing to URL: http://" CONSOLE_HOST_NAME "/boards/" AP_SSID);
  http.begin(client, "http://" CONSOLE_HOST_NAME "/boards/" AP_SSID);
//  http.setTimeout(10000); // give it 10 secs
  http.addHeader("Content-Type", "text/plain");
  int httpCode = http.POST("http://" + WiFi.localIP().toString());

  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] POST... code: %d\n", httpCode);
  } else {
    Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}
