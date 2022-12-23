void getFullBoardState() {
  onReceiveStateRequest(FULL_STATE);
}

void updateEnabledStatus() {
  String enabledArg = server.arg(0);
  enabledArg.toLowerCase();
  bool validArg = enabledArg == "true" || enabledArg == "false";
  if (server.args() == 1 && validArg) {
    bool enable = enabledArg == "true";
    const char* msg = enable ? "Board enable message sent" : "Board disable message sent";
    onReceiveEnabledMessage(enable);
    server.send(200, "text/plain", msg);
  } else {
    server.send(400, "text/plain", "There should be one argument: enabled=<true or false>");
  }
}
