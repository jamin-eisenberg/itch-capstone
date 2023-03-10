void updateEnabledStatus() {
  String enabledArg = server.arg(0);
  enabledArg.toLowerCase();
  bool validArg = enabledArg == "true" || enabledArg == "false";
  if (server.args() == 1 && validArg) {
    bool enable = enabledArg == "true";
    onReceiveEnabledMsg(enable);
  } else {
    server.send(400, "text/plain", "There should be one argument: enabled=<true or false>");
  }
}
