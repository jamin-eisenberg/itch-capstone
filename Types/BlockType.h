/**
 * File contains data structures to represent the Itch blocks.
 */

#ifndef BLOCK_TYPE
#define BLOCK_TYPE

#include <ArduinoJson.h>

#include "SensorData.h"

/**
 * Names of each block/block type, used in serialization.
 */
const char* blockNames[] = {
  "parameter",
  "rotate",
  "repeat",
  "move forward",
  "move backward",
  "if",
  "until",
  "statement",
  "stop",
  "hook up",
  "hook down",
  "otherwise",
  "forever",
  "}",
  "argument",
  "Condition",
  "Number",
  "Rotation Amount",
  "UNKNOWN",
  "No Block",
};

/**
 * Contains all possible blocks, and categorizations of the blocks.
 * NOTE: The ordering here should match the ordering in blockNames above,
 *  and the order of the groupings is depended on by ItchBlock::getCategory().
 */
enum class BlockType {
  /////////////////////////////////////////
  PARAMETER_BLOCK = 0,
  ROTATE = 1,
  REPEAT_TIMES = 2,
  FORWARD = 3,
  BACKWARD = 4,
  IF = 5,
  UNTIL = 6,
  /////////////////////////////////////////
  STATEMENT_BLOCK = 7,
  STOP = 8,
  HOOK_UP = 9,
  HOOK_DOWN = 10,
  OTHERWISE = 11,
  FOREVER = 12,
  END_CONTROL = 13,
  /////////////////////////////////////////
  ARGUMENT_BLOCK = 14,
  CONDITION = 15,
  NUMBER = 16,
  ROTATION_AMOUNT = 17, 
  /////////////////////////////////////////
  UNKNOWN_BLOCK = 18,  // Represents an error
  NONE = 19, // Represents the absence of a block
};


/**
 * Convert from a string to a blockType.
 */
BlockType nameToType(const char* blockName) {
   for (int i = 0; i < (sizeof(blockNames) / sizeof(blockNames[0])); i++) {
     if (!strcmp(blockNames[i], blockName)) {
       return static_cast<BlockType>(i);
     }
   }
}

/**
 * Represents all boolean condition type parameters.
 */
enum class Condition {
  CLOSE_TO_WALL = 0,
  IS_RED = 1,
  IS_GREEN = 2,
  IS_BLUE = 3,
};

/**
 * Translations from Condition enum to names.
 */
const char* conditionNames[] = {
  "Close to Wall",
  "Seeing Red",
  "Seeing Green",
  "Seeing Blue",
};

/**
 * Convert from a string to a blockType.
 */
Condition nameToCondition(const char* condition) {
   for (int i = 0; i < (sizeof(conditionNames) / sizeof(conditionNames[0])); i++) {
     if (!strcmp(conditionNames[i], condition)) {
       return static_cast<Condition>(i);
     }
   }
}

/**
 * Evaluate conditions with sensor data.
 */
bool conditionValid(Condition condition, SensorData &data) {
  switch (condition) {
    case Condition::CLOSE_TO_WALL:
      return data.closeToWall;
    case Condition::IS_RED:
      return data.isRed;
    case Condition::IS_GREEN:
      return data.isGreen;
    case Condition::IS_BLUE:
      return data.isBlue;
    default:
      return true;
  }
}

/**
 * Represents the control type of the block.
 */
enum class ControlType {
  COMMAND,
  CONTROL,
  END_CONTROL,
  NOOP
};

/**
 * Represents a single Itch Block, primarily for convenience.
 */
class ItchBlock {
  public:
  BlockType block;
  BlockType argument;
  int argumentValue;

  ItchBlock() : block(BlockType::UNKNOWN_BLOCK), argument(BlockType::NONE), argumentValue(0) {}
  
  ItchBlock(BlockType block) : block(block), argument(BlockType::NONE), argumentValue(0) {}

  ItchBlock(BlockType block, BlockType argument, int argumentValue) : block(block), argument(argument), argumentValue(argumentValue) {}

  ItchBlock(BlockType block, BlockType argument, Condition argumentValue) : block(block), argument(argument), argumentValue(static_cast<int>(argumentValue)) {}


  virtual ~ItchBlock() {};
  
  const char* getName() const {
    return blockNames[static_cast<int>(this->block)];
  }
  
  BlockType getCategory() const {
    if (this->block >= BlockType::UNKNOWN_BLOCK) {
      return this->block;
    } else if (this->block >= BlockType::ARGUMENT_BLOCK) {
      return BlockType::ARGUMENT_BLOCK;
    } else if (this->block >= BlockType::STATEMENT_BLOCK) {
      return BlockType::STATEMENT_BLOCK;
    } else if (this->block >= BlockType::PARAMETER_BLOCK) {
       // Note that if an ItchBlock is constructed which has a primary type of PARAMETER_BLOCK, it is an error.
      return BlockType::PARAMETER_BLOCK;
    }
  }

  ControlType getControlType() {
    switch (block) {
      case BlockType::ROTATE:
      case BlockType::FORWARD:
      case BlockType::BACKWARD:
      case BlockType::HOOK_UP:
      case BlockType::HOOK_DOWN:
      case BlockType::STOP:
        return ControlType::COMMAND;
      case BlockType::REPEAT_TIMES:
      case BlockType::UNTIL:
      case BlockType::FOREVER:
      case BlockType::IF:
      case BlockType::OTHERWISE:
      case BlockType::END_CONTROL:
        return ControlType::CONTROL;
      default:
        return ControlType::NOOP;
    }
  }

  bool operator ==(const ItchBlock &b) const {
    return this->block == b.block && this->argument == b.argument;
  }
};

bool convertToJson(const BlockType& src, JsonVariant dst) {
  return dst.set(blockNames[static_cast<int>(src)]);
}

void convertFromJson(JsonVariantConst src, BlockType& dst) {
  dst = nameToType(src.as<const char*>());
}

bool convertToJson(const ItchBlock& src, JsonVariant dst) {
  dst.to<JsonObject>();
  auto category = src.getCategory();
  dst["type"] = category;
  dst["name"] = src.getName();
  switch(src.argument) {
    case BlockType::ROTATION_AMOUNT:
    case BlockType::NUMBER:
      dst["argument"] = src.argumentValue;
      break;
    case BlockType::CONDITION:
      dst["argument"] = conditionNames[src.argumentValue];
      break;
    case BlockType::NONE:
      break;
    default:
      dst["argument"] = src.argument; // Something's probably gone wrong, but probably still useful to include
  }
  return true;
}

void convertFromJson(JsonVariantConst src, ItchBlock& dst) {
  dst.block = src["name"].as<BlockType>();
  if (src.containsKey("argument") && src["argument"] != nullptr) {
    if (src["argument"].is<int>()) {
      dst.argumentValue = src["argument"].as<int>();
      if (dst.block == BlockType::ROTATE) {
        dst.argument = BlockType::ROTATION_AMOUNT;
      } else {
        dst.argument = BlockType::NUMBER;
      }
    } else {
      dst.argumentValue = static_cast<int>(nameToCondition(src["argument"].as<const char*>()));
    }
  }
}

#endif
