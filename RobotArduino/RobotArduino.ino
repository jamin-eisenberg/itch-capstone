/**************************************************************************************
 * Library Declarations
 **************************************************************************************/

#include <ArduinoJson.h>
#include <arduino-timer.h>
#include <Wire.h>

#include "BlockType.h"
#include "Adafruit_TCS34725.h"

/**************************************************************************************
 * Data Structures
 **************************************************************************************/
 
// Which Side of the robot you are referencing, oriented as looking from hook to pusher.
enum Side {
  RIGHT = 0b01,
  LEFT = 0b10,
  BOTH = 0b11,
};

/**************************************************************************************
 * Global Variables
 **************************************************************************************/

/* Initialise with specific int time and gain values */
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);
Timer<1, micros> stopTimer{};

/**************************************************************************************
 * Constants
 **************************************************************************************/
// NOTE: Pins are not freely chosen. Analog 4 and 5 are used for the I2C protocol (see Wire library),
//    and the motor enable pins and servo pin need to be PWM pins, which the nano only has a handful of.
//    Everything else can be moved around.

// These will go away when I get around to using the IR encoder and a better controller
#define ROBOT_SPEED 190
#define ROBOT_TURN_SPEED 200
#define NUMBER_TO_MOVE_TIME 50
#define NUMBER_TO_ROTATION_DELAY 30
 
// IR sensors for motor encoder, pins and constants
const int LEFT_IR_PIN = A1;
const int RIGHT_IR_PIN = A2;
// Ultrasonic Sensor pins and constants.
const int ECHO_PIN = 6;
const int TRIG_PIN = 7;
// Using the speed of sound at 50% humidity and 20 degrees C, cm/us.
const double SOUND_SPEED_CONSTANT = 0.0344;
const double WALL_THRESHOLD = 8.0;
// Servo pin and hook constants.
const int SERVO_PIN = 9;
const int HOOK_MIN = 0;
const int HOOK_MAX = 100;
const bool HOOK_UP = true;
// Motor A connections
const int RIGHT_EN = 10;
const int RIGHT_DIR_1 = 2;
const int RIGHT_DIR_2 = 3;
// Motor B connections.
const int LEFT_EN = 11;
const int LEFT_DIR_1 = 4;
const int LEFT_DIR_2 = 5;

/**************************************************************************************
 * Function forward declarations
 **************************************************************************************/

void onReceiveCommand(JsonDocument& doc);
void onRobotDone();

/**************************************************************************************
 * Sensor Helper Functions
 **************************************************************************************/

/**
 * Read the ultrasonic sensor, scaled roughly to centimeters. It may return -1 if out of range (> 4m or <2cm).
 */
 double readUltrasonicDistance() {
    float duration, distance;
    digitalWrite(TRIG_PIN, LOW); 
    delayMicroseconds(2);
   
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    duration = pulseIn(ECHO_PIN, HIGH); // Duration of pulse in microseconds
    distance = (duration / 2) * SOUND_SPEED_CONSTANT;
    
    if (distance >= 400 || distance <= 2){
      return -1; // Out of range
    }
    else {
      return distance;
    }
 }

 /**
 * Identify which colors the block sees, and load them into the given data.
 */
void colorIdentify(SensorData &data){
  float red, green, blue;
  tcs.getRGB(&red, &green, &blue);
  float lux = tcs.calculateLux(red, green, blue);
  
  data.isRed = (red >= 120 && lux <= 30);
  data.isGreen = (green >= 120 && lux >= 120);
  data.isBlue = (blue >= 100);
}


/**
 * Return the sensor data of the robot
 */
SensorData getSensorData() {
  SensorData data;
  auto distanceToWall = readUltrasonicDistance();
  data.closeToWall = (distanceToWall >= 0 && distanceToWall < WALL_THRESHOLD);
  colorIdentify(data);
  return data;
}

/**************************************************************************************
 * Actuator Helper Functions
 **************************************************************************************/

/**
 * Lower level, sets the states for each passed pin according to given motorSpeed.
 *  Use speedControl or stopMotor instead.
 */
void setMotorPins(int dir1, int dir2, int en, int motorSpeed) {
  if (motorSpeed < 0) {
    digitalWrite(dir1, LOW);
    digitalWrite(dir2, HIGH);
  } else if (motorSpeed > 0) {
    digitalWrite(dir1, HIGH);
    digitalWrite(dir2, LOW);
  } else {
    digitalWrite(dir1, LOW);
    digitalWrite(dir2, LOW);
  }
  analogWrite(en, abs(motorSpeed));
}

/**
 * Control speed and direction of an individual motor.
 * @param motor Set to RIGHT, LEFT, or BOTH.
 * @param motorSpeed positive for forward, negative for backward, -255 to 255.
 */
void speedControl(Side motor, int motorSpeed) {
  int dir1, dir2, enable = 0;
  if (motor & RIGHT) {
    setMotorPins(RIGHT_DIR_1, RIGHT_DIR_2, RIGHT_EN, motorSpeed);
  }
  if (motor & LEFT){
    setMotorPins(LEFT_DIR_1, LEFT_DIR_2, LEFT_EN, motorSpeed);
  }
}

/**
 * Stop a motor.
 * @param motor Set to RIGHT, LEFT, or BOTH.
 */
void stopMotor(Side motor) {
  speedControl(motor, 0);
}

int hookPos = HOOK_MIN;
void setHook(bool hookUp) {
  int dir = -1;
  if (hookUp)
    dir = 1;
  if (hookPos == HOOK_MIN) {
    hookPos++;
  } else if (hookPos == HOOK_MAX) {
    hookPos--;
  }
  for (; (hookPos < HOOK_MAX) && (hookPos > HOOK_MIN); hookPos += dir) {
    analogWrite(SERVO_PIN, hookPos); 
    delay(10);
  }
}

/**************************************************************************************
 * Main Setup and Loop functions
 **************************************************************************************/

void setup() {
  Wire.begin();
  Serial.begin(9600);
  while(!Serial) {}

  // Set Ultrasonic sensor pin modes
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Set all the motor control pins to outputs
  pinMode(RIGHT_EN, OUTPUT);
  pinMode(LEFT_EN, OUTPUT);
  pinMode(RIGHT_DIR_1, OUTPUT);
  pinMode(RIGHT_DIR_2, OUTPUT);
  pinMode(LEFT_DIR_1, OUTPUT);
  pinMode(LEFT_DIR_2, OUTPUT);

  // Set Servo pin to output
  pinMode(SERVO_PIN, OUTPUT);

  // Initialize pin values and global vars
  setHook(false); 
  stopMotor(BOTH);
  
  /* Configures TSL2550 (RGB sensor) with Extended Range */
  tcs.begin();
}

void loop() {
    StaticJsonDocument<96> doc;

    DeserializationError error = deserializeJson(doc, Serial);

    if (!error) {
      onReceiveCommand(doc);
    } else {
//      Serial.println(error.c_str());
    }
  }

  stopTimer.tick();
}

/**************************************************************************************
 * Communications methods
 **************************************************************************************/

void onReceiveCommand(JsonDocument& doc) {
  ItchBlock b = doc.as<ItchBlock>();
  stopTimer.cancel(); // Kill any existing timer and stop motors.
  stopMotor(BOTH);
  switch(b.block) {
    case BlockType::ROTATE:
      speedControl(RIGHT, -ROBOT_TURN_SPEED);
      speedControl(LEFT, ROBOT_TURN_SPEED);
      stopTimer.in(b.argumentValue * NUMBER_TO_ROTATION_DELAY, [](void*) -> bool { stopMotor(BOTH); return false; });
      break;
    case BlockType::FORWARD:
      speedControl(BOTH, ROBOT_SPEED);
      stopTimer.in(b.argumentValue * NUMBER_TO_MOVE_TIME, [](void*) -> bool { stopMotor(BOTH); return false; });
      break;
    case BlockType::BACKWARD:
      speedControl(BOTH, -ROBOT_SPEED);
      stopTimer.in(b.argumentValue * NUMBER_TO_MOVE_TIME, [](void*) -> bool { stopMotor(BOTH); return false; });
      break;
    case BlockType::STOP:
      stopMotor(BOTH);
      break;
    case BlockType::HOOK_UP:
      setHook(true);
      break;
    case BlockType::HOOK_DOWN:
      setHook(false);
      break;
    default:
      stopMotor(BOTH);
  }
  onRobotDone();
}

void onRobotDone() {
  StaticJsonDocument<64> doc;
  doc = getSensorData();
  serializeJson(doc, Serial);
}
