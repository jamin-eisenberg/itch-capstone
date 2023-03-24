#include <BlockType.h>
#include <SensorData.h>
#include <ArduinoJson.h>

#include "ItchBoard.h"

#define STOP_BUTTON 3
#define GO_BUTTON 4
#define GO_FOREVER_BUTTON 5

/**
 * Structs used only in this file.
 */

/**
 * The runstate of the board; fetchNext logically implies runItchCode.
 */
struct RunState {
  bool runItchCode; // True when board is in a running state.
  bool fetchNext; // True when we actively need to retrieve a command block.
  bool loopBoard; // True when the board was started on a forever loop.
};

/**
 * Methods defined in this file.
 */

void sendBlock(ItchBlock command);
void sendBoardState();
void stopAll();

/**
 * Global Variables
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
}

void loop() {
  if (digitalRead(STOP_BUTTON) == HIGH) {
    Serial.println("STOP BUTTON PRESSED");
    stopAll();
  } else if (digitalRead(GO_BUTTON) == HIGH) {
    Serial.println("GO BUTTON PRESSED");
    state = {true, true, false};
    itchBoard.resetBoard();
  } else if (digitalRead(GO_FOREVER_BUTTON) == HIGH) {
    Serial.println("FOREVER BUTTON PRESSED");
    state = {true, true, true};
    itchBoard.resetBoard();
  }
  
  if (Serial1.available() > 0) {
    StaticJsonDocument<2000> doc;
    DeserializationError error = deserializeJson(doc, Serial1);
    if (error) { //TODO: A spot where error handling could be improved
      return;
    }

    if (doc.is<SensorData>()) {
      currentSensorData = doc.as<SensorData>();
      state.fetchNext = state.runItchCode;
    } else {
      sendBoardState();
    }
  }

  if (state.fetchNext) {
    ItchBlock nextCommandBlock = itchBoard.getNextCommand(currentSensorData);
    if(nextCommandBlock.getCategory() != BlockType::NONE) {
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
  sendBlock(ItchBlock(BlockType::STOP));
}

void sendBlock(ItchBlock command) {
    StaticJsonDocument<64> robotCommand;
    robotCommand.set(command);
    
    serializeJson(robotCommand, Serial);
    Serial.println();
    serializeJson(robotCommand, Serial1);
}

void sendBoardState() {
  StaticJsonDocument<2000> doc; // CHANGE SIZE

  JsonObject root = doc.to<JsonObject>();
  JsonArray state = root.createNestedArray("state");

  for (int row = 0; row < BOARD_SIZE; row++) {
    state.add(itchBoard.identifyBlock(row, false));
  }
    
  root["inactivity duration"] = 2311; //TODO; this.
  serializeJson(doc, Serial);
  serializeJson(doc, Serial1);
}
