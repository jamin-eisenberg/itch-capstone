void handleNotFound() {
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void serverSetup() {
  // Connecting to WiFi...
  logger->print("Connecting to ");
  logger->print(WIFI_SSID);
  int maxWait = 10000;
  int wait = 0;
  while (WiFi.status() != WL_CONNECTED && wait < maxWait)
  {
    wait += 100;
    delay(100);
    logger->print(".");
  }

  
  server.on("/", HTTP_GET, onReceiveStateRequest);
  server.on("/", HTTP_PATCH, updateEnabledStatus);
  server.on("/sensordata", HTTP_POST, onReceiveSensorData);
  server.onNotFound(handleNotFound);
  server.begin();

  if (WiFi.status() != WL_CONNECTED) {
    logger->println();
    logger->println("Could not connect to Itch! Timeout");
    return;
  }

  // Connected to WiFi
  logger->println();
  logger->println("Connected!");
  logger->print("IP address for network ");
  logger->print(WIFI_SSID);
  logger->print(" : ");
  logger->println(WiFi.localIP());
  logger->print("IP address for network ");
  logger->print(AP_SSID);
  logger->print(" : ");
  logger->println(WiFi.softAPIP());
  WiFi.hostByName("consolehost.mshome.net", consolehostIP);
  logger->print("Console host IP: ");
  logger->println(consolehostIP);
}

void registerWithConsoleHost() {
  if (WiFi.status() != WL_CONNECTED) {
    logger->println("Not connected to wiFi, ignoring console host.");
    return;
  }
  logger->println("Registering with Console Host.");
  logger->println("POSTing to URL: http://" + consolehostIP.toString() + CONSOLE_HOST_PORT_STR + "/boards/" AP_SSID);
  http.begin(client, "http://" + consolehostIP.toString() + CONSOLE_HOST_PORT_STR + "/boards/" AP_SSID);
  //  http.setTimeout(10000); // give it 10 secs
  http.addHeader("Content-Type", "text/plain");
  int httpCode = http.POST("http://" + WiFi.localIP().toString());

  if (httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    logger->printf("[HTTP] POST... code: %d\n", httpCode);
  } else {
    logger->printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}
