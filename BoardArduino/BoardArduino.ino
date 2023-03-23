#include <BlockType.h>
#include <SensorData.h>
#include <ArduinoJson.h>

#include "ItchBoard.h"

#define STOP_BUTTON 0
#define GO_BUTTON 1
#define GO_FOREVER_BUTTON 2

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
  setButtonStates();
  Serial.begin(9600);
  Serial1.begin(9600);

  for (int row = A0; row <= A15; row++) {
    pinMode(row, INPUT);
  }
  for (int row = 38; row < 38 + 16; row++) {
    pinMode(row, OUTPUT);
  }
  pinMode(STOP_BUTTON, INPUT);
  pinMode(GO_BUTTON, INPUT);
  pinMode(GO_FOREVER_BUTTON, INPUT);

  itchBoard = ItchBoard();
  state = {false, false, false};
}

void loop() {
  if (digitalRead(STOP_BUTTON) == HIGH) {
    stopAll();
  } else if (digitalRead(GO_BUTTON) == HIGH) {
    state = {true, true, false};
    itchBoard.resetBoard();
  } else if (digitalRead(GO_FOREVER_BUTTON) == HIGH) {
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
    serializeJson(robotCommand, Serial1);
}

void sendBoardState() { //TODO: Write this function, it's just a stub right now
  StaticJsonDocument<2000> doc; // CHANGE SIZE

  JsonObject root = doc.to<JsonObject>();
  JsonArray state = root.createNestedArray("state");

  ItchBlock block1(BlockType::IF, BlockType::CONDITION, Condition::IS_BLUE);
  state.add(block1);
  
  state.add(ItchBlock(BlockType::BACKWARD, BlockType::NUMBER, 1));
  
  root["inactivity duration"] = 2311;
  serializeJson(doc, Serial);
  serializeJson(doc, Serial1);
}
