#include <BlockType.h>
#include <SensorData.h>
#include <ArduinoJson.h>

#include "ItchBoard.h"

#define STOP_BUTTON 4
#define GO_BUTTON 3
#define GO_FOREVER_BUTTON 2

/**
   Structs used only in this file.
*/

/**
   The runstate of the board; fetchNext logically implies runItchCode.
*/
struct RunState {
  bool runItchCode; // True when board is in a running state.
  bool fetchNext; // True when we actively need to retrieve a command block.
  bool loopBoard; // True when the board was started on a forever loop.
};

/**
   Methods defined in this file.
*/

void sendBlock(ItchBlock command);
void sendBoardState();
void stopAll();

/**
   Global Variables
*/
ItchBoard itchBoard;
SensorData currentSensorData;
RunState state;

void setButtonStates() {
  pinMode(STOP_BUTTON, INPUT);
  pinMode(GO_BUTTON, INPUT);
  pinMode(GO_FOREVER_BUTTON, INPUT);
}

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  setButtonStates();
  itchBoard = ItchBoard();
  state = {false, false, false};

  // A little readiness indicator and helpful for testing
  for (int row = LED_ROW_OFFSET + BOARD_SIZE - 1; row >= LED_ROW_OFFSET; row--) {
    digitalWrite(row, HIGH);
    delay(200);
    digitalWrite(row, LOW);
  }
}

void loop() {
  if (digitalRead(STOP_BUTTON) == HIGH) {
    Serial.println("STOP BUTTON PRESSED");
    stopAll();
    delay(100);
  } else if (digitalRead(GO_BUTTON) == HIGH) {
    Serial.println("GO BUTTON PRESSED");
    state = {true, true, false};
    itchBoard.resetBoard();
    delay(100);
  } else if (digitalRead(GO_FOREVER_BUTTON) == HIGH) {
    Serial.println("FOREVER BUTTON PRESSED");
    state = {true, true, true};
    itchBoard.resetBoard();
    delay(100);
  }

  if (Serial1.available() > 0) {
    StaticJsonDocument<2000> doc;
    DeserializationError error = deserializeJson(doc, Serial1);
    if (error) { //TODO: A spot where error handling could be improved
      return;
    }

    if (doc.is<JsonObject>() && doc.containsKey("close to wall") && doc.containsKey("red")
        && doc.containsKey("green") && doc.containsKey("blue")) {
      Serial.println("Sensor Data recieved: ");
      serializeJson(doc, Serial);
      Serial.println();
      currentSensorData = doc.as<SensorData>();
      state.fetchNext = state.runItchCode;
    } else {
      Serial.print("Recieved board scan request: ");
      serializeJson(doc, Serial);
      Serial.println();
      Serial.print(doc.is<JsonObject>());
      Serial.print(" ");
      Serial.print(doc.containsKey("red"));
      Serial.print(" ");
      Serial.print(doc.containsKey("green"));
      Serial.print(" ");
      Serial.print(doc.containsKey("blue"));
      Serial.print(" ");
      Serial.println(doc.containsKey("close to wall"));
      sendBoardState();
    }
  }

  if (state.fetchNext) {
    ItchBlock nextCommandBlock = itchBoard.getNextCommand(currentSensorData);
    if (nextCommandBlock.getCategory() != BlockType::NONE) {
      sendBlock(nextCommandBlock);
      state.fetchNext = false;
    } else if (state.loopBoard) {
      itchBoard.resetBoard();
    } else {
      stopAll();
    }
  }
}

void stopAll() {
  state = {false, false, false};
  itchBoard.resetBoard();
  sendBlock(ItchBlock(BlockType::STOP));
}

void sendBlock(ItchBlock command) {
  StaticJsonDocument<64> robotCommand;
  robotCommand.set(command);

  Serial.print("Sending command: ");
  serializeJson(robotCommand, Serial);
  Serial.println();
  serializeJson(robotCommand, Serial1);
}

void sendBoardState() {
  StaticJsonDocument<2000> doc; // CHANGE SIZE

  JsonObject root = doc.to<JsonObject>();
  JsonArray state = root.createNestedArray("state");

  for (int row = BOARD_SIZE - 1; row >= 0; row--) {
    state.add(itchBoard.identifyBlock(row, false));
  }

  root["inactivity duration"] = 2311; //TODO; this.
  serializeJson(doc, Serial);
  serializeJson(doc, Serial1);
}
