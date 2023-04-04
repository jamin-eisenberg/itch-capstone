/**
   This file is for the ItchBoard class and associated types & methods.
*/

#ifndef ITCH_BOARD
#define ITCH_BOARD

#include <SimpleStack.h>
#include "BlockType.h"

#define BOARD_SIZE 16
#define LED_ROW_OFFSET 24
#define LIGHT_DELAY 2000 //ms

/**
   Represents a logical scope on the board.
*/
struct Scope {
  int startRow;
  ItchBlock block;
  int loopCount;
  bool execute;
};

int analogPins[] = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15};

/**
   Represents the primary ItchBoard.
*/
class ItchBoard {
  private:
    int executingRow; //TODO: Use this or delete it. Intended to help track which row should be lit up while we wait for robot to finish executing.
    int nextRow;
    SimpleStack<Scope> scopes;

  public:
    ItchBoard() : executingRow(BOARD_SIZE - 1), nextRow(BOARD_SIZE - 1), scopes(SimpleStack<Scope>(BOARD_SIZE)) {
      for (int row = A0; row <= A15; row++) {
        pinMode(row, INPUT);
      }
      for (int row = LED_ROW_OFFSET; row < LED_ROW_OFFSET + BOARD_SIZE; row++) {
        pinMode(row, OUTPUT);
      }
    }

    /**
       Flash the light on one of the rows to indicate that an error has occured.
    */
    void errorOnRow(int row) {
      int digitalRow = row + LED_ROW_OFFSET;
      for (int i = 0; i < 10; i++) {
        delay(500);
        digitalWrite(digitalRow, HIGH);
        delay(500);
        digitalWrite(digitalRow, LOW);
      }
    }

    /**
       Identify the next block on the board.
    */
    ItchBlock identifyBlock(int row, bool lightUpRow) {
      float Vin = 5;
      ItchBlock identifiedBlock(BlockType::NONE);
      int raw = analogRead(analogPins[row]);
      if (raw) {
        float R1 = 22000;
        float Vout = (raw * Vin) / 1024.0;
        int R2 = 1000000000;
        if (Vout >= 0.00001) {
          R2 = R1 * (Vin / Vout) - R1;
        }
//        Serial.print("Row ");
//        Serial.print(row);
//        Serial.print(" resistance = ");
//        Serial.println(R2);

        if (R2 > 5 && R2 <= 100) {
          identifiedBlock = ItchBlock(BlockType::HOOK_UP);
        }
        else if (R2 <= 300) {
          identifiedBlock = ItchBlock(BlockType::END_CONTROL);
        }
        else if (R2 <= 600) {
          identifiedBlock = ItchBlock(BlockType::HOOK_DOWN);
        }
        else if (R2 <= 2100) {
          identifiedBlock = ItchBlock(BlockType::ROTATE, BlockType::ROTATION_AMOUNT, R2 - 1000);
        }
        else if (R2 <= 3500) {
          if (R2 <= 3250) {
            identifiedBlock = ItchBlock(BlockType::FORWARD, BlockType::NUMBER, 1);
          }
          else if (R2 <= 3350) {
            identifiedBlock = ItchBlock(BlockType::FORWARD, BlockType::NUMBER, 2);
          }
          else {
            identifiedBlock = ItchBlock(BlockType::FORWARD, BlockType::NUMBER, 3);
          }
        }
        else if (R2 <= 5100) {
          if (R2 <= 4750) {
            identifiedBlock = ItchBlock(BlockType::IF, BlockType::CONDITION, Condition::CLOSE_TO_WALL);
          }
          else if (R2 <= 4850) {
            identifiedBlock = ItchBlock(BlockType::IF, BlockType::CONDITION, Condition::IS_RED);
          }
          else if (R2 <= 5000) {
            identifiedBlock = ItchBlock(BlockType::IF, BlockType::CONDITION, Condition::IS_GREEN);
          }
          else {
            identifiedBlock = ItchBlock(BlockType::IF, BlockType::CONDITION, Condition::IS_BLUE);
          }
        }
        else if (R2 <= 6000) {
          if (R2 <= 5750) {
            identifiedBlock = ItchBlock(BlockType::BACKWARD, BlockType::NUMBER, 1);
          }
          else if (R2 <= 5850) {
            identifiedBlock = ItchBlock(BlockType::BACKWARD, BlockType::NUMBER, 2);
          }
          else {
            identifiedBlock = ItchBlock(BlockType::BACKWARD, BlockType::NUMBER, 3);
          }
        }
        else if (R2 <= 9800) {
          if (R2 <= 9450) {
            identifiedBlock = ItchBlock(BlockType::UNTIL, BlockType::CONDITION, Condition::CLOSE_TO_WALL);
          }
          else if (R2 <= 9550) {
            identifiedBlock = ItchBlock(BlockType::UNTIL, BlockType::CONDITION, Condition::IS_RED);
          }
          else if (R2 <= 9700) {
            identifiedBlock = ItchBlock(BlockType::UNTIL, BlockType::CONDITION, Condition::IS_GREEN);
          }
          else {
            identifiedBlock = ItchBlock(BlockType::UNTIL, BlockType::CONDITION, Condition::IS_BLUE);
          }
        }
      }
//      Serial.print("Identified block as ");
//      Serial.println(blockNames[static_cast<int>(identifiedBlock.block)]);

      if (identifiedBlock.block != BlockType::NONE && lightUpRow) {
        digitalWrite(row + LED_ROW_OFFSET, HIGH);
//        delay(LIGHT_DELAY);
//        digitalWrite(row + LED_ROW_OFFSET, LOW);
      }
      return identifiedBlock;
    }

    /**
       Advance the board to the next command block, factoring in control flow and sensor data. Returns
       the next Command block, or a BlockType::NONE block if we have hit the end of execution and global loop
       is false..

       This function is rather gnarly, but basically skipping represents whether the current scope
        is an executing scope, and if a scope is created in an executing scope it is also an executing
        scope. However, when a condition is false and we need to 'skip' to the corresponding curly brace,
        skipping becomes false and all nested scopes are non-executing. Once the executing scope ends,
        skipping becomes false again and we return to the executing environment. Special case of Otherwise
        will end the scope and then flip whether we are in an executing environment.
    */
    ItchBlock getNextCommand(SensorData &data) {
      digitalWrite(executingRow + LED_ROW_OFFSET, LOW);
      
      bool skipping = false;
      ItchBlock nextBlock;

      do {
        if (nextRow < 0) {
          return ItchBlock(BlockType::NONE);
        }
        nextBlock = identifyBlock(nextRow, true);
        if (nextBlock.block == BlockType::END_CONTROL) {
          Scope *currentScope;
          if (!scopes.pop(currentScope)) {
            errorOnRow(nextRow);
          } else {
            if (!skipping && currentScope->block.block != BlockType::IF && currentScope->block.block != BlockType::OTHERWISE) {
              if (!(identifyBlock(currentScope->startRow, true) == currentScope->block)) { // Just applies to looping blocks
                // The start block of the scope has been swapped or removed during execution, continue from end brace as though loop ended
                errorOnRow(currentScope->startRow);
              } else if (currentScope->block.argument == BlockType::NUMBER && currentScope->loopCount > 0) { // This can only be the Repeat _ Times block
                scopes.push({currentScope->startRow, currentScope->block, currentScope->loopCount--, currentScope->execute});
                nextRow = currentScope->startRow;
              } else if ((currentScope->block.block == BlockType::UNTIL && !conditionValid(static_cast<Condition>(currentScope->block.argumentValue), data))
                         || currentScope->block.block == BlockType::FOREVER) {
                scopes.push(*currentScope);
                nextRow = currentScope->startRow;
              }
            }
            if (skipping && currentScope->execute) {
              skipping = false;
            }
          }
        }
        else if (nextBlock.getControlType() == ControlType::CONTROL) {
          if (nextBlock.block == BlockType::OTHERWISE) {
            // `Otherwise` both ends a scope and begins a new scope, with a flipped execution w.r.t. the previous scope
            Scope* currentScope;
            if (!scopes.pop(currentScope) || currentScope->block.block != BlockType::IF) {
              // Woah there! The open stack isn't an `if`, so the `otherwise` isn't valid! Carefully place scope back on the stack, back away slowly...
              errorOnRow(nextRow);
              scopes.push(*currentScope);
            } else if (currentScope->execute) {
              skipping != skipping;
              scopes.push({nextRow, nextBlock, 0, true});
            } else {
              scopes.push({nextRow, nextBlock, 0, false});
            }
          } else if (nextBlock.argument == BlockType::CONDITION) {
            scopes.push({nextRow, nextBlock, 0, !skipping}); // Only marked as an 'execution scope' if we aren't in a skip phase; same as below
            auto condition = static_cast<Condition>(nextBlock.argumentValue);
            if (conditionValid(condition, data) == (nextBlock.block == BlockType::UNTIL)) {
              // Logical XOR to deal with the UNTIL logical inversion of conditions.
              skipping = true;
            }
          } else if (nextBlock.argument == BlockType::NUMBER) {
            scopes.push({nextRow, nextBlock, nextBlock.argumentValue, !skipping});
          }
        }

        executingRow = nextRow;
        nextRow--;
      } while (!skipping && nextBlock.getControlType() != ControlType::COMMAND);

      digitalWrite(executingRow + LED_ROW_OFFSET, HIGH);
      return nextBlock;
    }

    /**
       Reset the board state, back to the first row.
    */
    void resetBoard() {
      digitalWrite(executingRow + LED_ROW_OFFSET, LOW);
      nextRow = BOARD_SIZE - 1;
      executingRow = nextRow;
      scopes.empty();
    }
};

#endif
