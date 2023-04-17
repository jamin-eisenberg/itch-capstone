/**************************************************************************************
   Library Declarations
 **************************************************************************************/

#include <MPU6050.h>
#include <ArduinoJson.h>
#include <arduino-timer.h>
#include <Wire.h>

#include "BlockType.h"
#include "Adafruit_TCS34725.h"

/**************************************************************************************
   Data Structures
 **************************************************************************************/

// Which Side of the robot you are referencing, oriented as looking from hook to pusher.
enum Side {
  RIGHT = 0b01,
  LEFT = 0b10,
  BOTH = 0b11,
};

/**************************************************************************************
   Global Variables
 **************************************************************************************/

// Device drivers
MPU6050 mpu;
/* Initialise with specific int time and gain values */
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_614MS, TCS34725_GAIN_1X);

//Control variables
unsigned long startTime;
int rotationAmount;
int driveDistance;
Timer<1, micros> controlTimer;

// Sensor averages
double leftSmoothed = 0;
double rightSmoothed = 0;
/**************************************************************************************
   Constants
 **************************************************************************************/
// NOTE: Pins are not freely chosen. Analog 4 and 5 are used for the I2C protocol (see Wire library),
//    and the motor enable pins and servo pin need to be PWM pins, which the nano only has a handful of.
//    Everything else can be moved around.

#define INCHES_PER_ENCODER_TICK 0.392699 // Approximately pi/8, determined by measurement of wheel radius
#define ROBOT_SPEED 220
#define ROBOT_TURN_SPEED 235
#define ROTATE_CONTROL_DELAY 50000UL  // Time in microseconds (this is longer than drive control delay because the gyro doesn't update as fast)
#define DRIVE_CONTROL_DELAY 100UL  // Time in microseconds
#define CONTROL_TIME_CUTOFF 10000L  // Time in milliseconds

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

const double LEFT_GAIN = 0.5;
const double RIGHT_GAIN = 0.5;
/**************************************************************************************
   Function forward declarations
 **************************************************************************************/

void onReceiveCommand(JsonDocument& doc);
void onRobotDone();

/**************************************************************************************
   Sensor Helper Functions
 **************************************************************************************/

/**
   Read the ultrasonic sensor, scaled roughly to centimeters. It may return -1 if out of range (> 4m or <2cm).
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

  if (distance >= 400 || distance <= 2) {
    return -1; // Out of range
  }
  else {
    return distance;
  }
}

/**
  Identify which colors the block sees, and load them into the given data.
*/
void colorIdentify(SensorData &data) {
  float red, green, blue;
  tcs.getRGB(&red, &green, &blue);
  float lux = tcs.calculateLux(red, green, blue);

  data.isRed = (red >= 120 && lux <= 30);
  data.isGreen = (green >= 120 && lux >= 120);
  data.isBlue = (blue >= 100);
}


/**
   Return the sensor data of the robot
*/
SensorData getSensorData() {
  SensorData data;
  auto distanceToWall = readUltrasonicDistance();
  data.closeToWall = (distanceToWall >= 0 && distanceToWall < WALL_THRESHOLD);
  colorIdentify(data);
  return data;
}

/**************************************************************************************
   Actuator Helper Functions
 **************************************************************************************/

/**
   Lower level, sets the states for each passed pin according to given motorSpeed.
    Use speedControl or stopMotor instead.
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
   Control speed and direction of an individual motor.
   @param motor Set to RIGHT, LEFT, or BOTH.
   @param motorSpeed positive for forward, negative for backward, -255 to 255.
*/
void speedControl(Side motor, int motorSpeed) {
  if (motor & RIGHT) {
    setMotorPins(RIGHT_DIR_1, RIGHT_DIR_2, RIGHT_EN, motorSpeed);
  }
  if (motor & LEFT) {
    setMotorPins(LEFT_DIR_1, LEFT_DIR_2, LEFT_EN, motorSpeed);
  }
}

/**
   Control speed of tbe motors.
   @param motor Set to RIGHT, LEFT, or BOTH.
   @param motorSpeed 0 to 255.
*/
void pureSpeedControl(Side motor, int motorSpeed) {
  if (motor & RIGHT) {
    analogWrite(RIGHT_EN, motorSpeed);
  }
  if (motor & LEFT) {
    analogWrite(LEFT_EN, motorSpeed);
  }
}

/**
   Stop a motor.
   @param motor Set to RIGHT, LEFT, or BOTH.
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
   Main Setup and Loop functions
 **************************************************************************************/

void setup() {
  Wire.begin();
  Serial.begin(9600);
  while (!Serial) {}

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

  mpu.initialize();
  // Calibrate gyroscope. The calibration must be at rest.
  mpu.setXGyroOffset(220);
  mpu.setYGyroOffset(76);
  mpu.setZGyroOffset(-85);

  controlTimer = Timer<1, micros>();
}

void loop() {
  if (Serial.available()) {
    StaticJsonDocument<96> doc;

    DeserializationError error = deserializeJson(doc, Serial);
    if (!error) {
      onReceiveCommand(doc);
    } else {
      Serial.println("ERROR");
      Serial.println(error.c_str());
    }
  }
  if (!controlTimer.empty()) {
    controlTimer.tick<void>();
  }
}

/**************************************************************************************
   Control methods
 **************************************************************************************/
bool under = false;
unsigned long lastTime = 0;
bool straightDriveControl(void*) {
  int leftEnc = analogRead(LEFT_IR_PIN);
  int rightEnc = analogRead(RIGHT_IR_PIN);
  leftSmoothed = (leftSmoothed * 4 + leftEnc) / 5.0;
  rightSmoothed = (rightSmoothed * 4 + rightEnc) / 5.0;
  leftEnc -= leftSmoothed;
  rightEnc -= rightSmoothed;

  if (leftEnc > 0 && under) {
    under = false;
    driveDistance -= 1;
  } else if (leftEnc < -0 && !under) {
    under = true;
  }
  if (rightEnc > 0 && under) {
    under = false;
    driveDistance -= 1;
  } else if (rightEnc < -0 && !under) {
    under = true;
  }

  if (abs(driveDistance + driveDistance) / 2 <= 1 || (millis() - startTime > CONTROL_TIME_CUTOFF)) {
    stopMotor(BOTH);
    onRobotDone();
    return false;

  }

  unsigned long now = millis();
  int az = round(mpu.getRotationZ() / 125.0);
  rotationAmount += ((now - lastTime) / 1000.0) * az;

  double leftSpeed = min(max(ROBOT_SPEED, ROBOT_SPEED + LEFT_GAIN * (36 * rotationAmount + driveDistance)), 255);
  double rightSpeed = min(max(ROBOT_SPEED, ROBOT_SPEED + RIGHT_GAIN * (36 * rotationAmount + driveDistance)), 255);

  pureSpeedControl(LEFT, leftSpeed);
  pureSpeedControl(RIGHT, rightSpeed);
  lastTime = now;
  return true;
}

bool rotateOnCenterControl(void*) {
  int az = round(mpu.getRotationZ() / 125.0);
  rotationAmount += (ROTATE_CONTROL_DELAY / 1000000.0) * az;
  int turnSign = ((rotationAmount > 0) ? (1) : (-1));
  if (abs(rotationAmount) <= 5 || (millis() - startTime > CONTROL_TIME_CUTOFF)) {
    stopMotor(BOTH);
    onRobotDone();
    return false;
  }
  speedControl(RIGHT, -ROBOT_TURN_SPEED * turnSign);
  speedControl(LEFT, ROBOT_TURN_SPEED * turnSign);
  return true;
}
/**************************************************************************************
   Communications methods
 **************************************************************************************/

void onReceiveCommand(JsonDocument& doc) {
  ItchBlock b = doc.as<ItchBlock>();

  controlTimer.cancel(); // Kill any existing timer and stop motors.
  stopMotor(BOTH);
  switch (b.block) {
    case BlockType::ROTATE:
      {
        rotationAmount = b.argumentValue;
        startTime = millis();
        controlTimer.every(ROTATE_CONTROL_DELAY, rotateOnCenterControl, NULL);
      }
      break;
    case BlockType::FORWARD:
      driveDistance = (int)(b.argumentValue / INCHES_PER_ENCODER_TICK);
      startTime = millis();
      lastTime = startTime;
      rotationAmount = 0;
      speedControl(BOTH, 1); // Sets the motor pins to forward
      controlTimer.every(DRIVE_CONTROL_DELAY, straightDriveControl, NULL);
      break;
    case BlockType::BACKWARD:
      driveDistance = (int)(b.argumentValue / INCHES_PER_ENCODER_TICK);
      startTime = millis();
      lastTime = startTime;
      rotationAmount = 0;
      speedControl(BOTH, -1); // Sets the motor pins to backward
      controlTimer.every(DRIVE_CONTROL_DELAY, straightDriveControl, NULL);
      break;
    case BlockType::STOP:
      stopMotor(BOTH);
      onRobotDone();
      break;
    case BlockType::HOOK_UP:
      setHook(true);
      onRobotDone();
      break;
    case BlockType::HOOK_DOWN:
      setHook(false);
      onRobotDone();
      break;
    default:
      stopMotor(BOTH);
  }
}

void onRobotDone() {
  StaticJsonDocument<64> doc;
  doc = getSensorData();
  serializeJson(doc, Serial);
}
