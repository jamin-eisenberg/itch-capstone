/**
 * This file contains the SensorData struct and methods, used to communicate Robot sensing data.
 */

#ifndef SENSOR_DATA
#define SENSOR_DATA

/**
 * The sensor data returned by the robot.
 */
struct SensorData {
  bool closeToWall;
  bool isRed;
  bool isGreen;
  bool isBlue;
};

bool convertToJson(const SensorData& src, JsonVariant dst) {
  bool success = dst.to<JsonObject>();
  success &= dst["close to wall"].set(src.closeToWall);
  success &= dst["red"].set(src.isRed);
  success &= dst["blue"].set(src.isBlue);
  success &= dst["green"].set(src.isGreen);
  return success;
}

void convertFromJson(JsonVariantConst src, SensorData& dst) {
  dst.closeToWall = src["close to wall"].as<bool>();
  dst.isRed = src["red"].as<bool>();
  dst.isGreen = src["blue"].as<bool>();
  dst.isBlue = src["green"].as<bool>();
}

bool canConvertFromJson(JsonVariantConst src, const SensorData&) {
  return src.is<JsonObject>() && src.containsKey("close to wall") && src.containsKey("red")
          && src.containsKey("green") && src.containsKey("blue"); 
}

#endif
