void getBoardStateJson() {
  // TODO actually get values
  server.send(200, "text/json", "{ \"inactivity-duration\": 10000, \"state\": [ { \"type\": \"statement\", \"name\": \"stop\", \"argument\": null } ] }");
}

void updateEnabledStatus() {
  // TODO use enabled var to determine whether robot actions happen
  String enabledArg = server.arg(0);
  enabledArg.toLowerCase();
  bool validArg = enabledArg == "true" || enabledArg == "false";
  if (server.args() == 1 && validArg) {
    enabled = enabledArg == "true";
    const char* msg = enabled ? "Board enabled" : "Board disabled";
    server.send(200, "text/plain", msg);
  } else {
    server.send(400, "text/plain", "There should be one argument: enabled=<true or false>");
  }
}
