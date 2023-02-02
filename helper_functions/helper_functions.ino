// This file is a bunch of helper functions for querying sensor states and setting actuators.
// NOTE: Pins are not freely chosen. Analog 4 and 5 are used for the I2C protocol (see Wire library),
//    and the motor enable pins and servo pin need to be PWM pins, which the nano only has a handful of.
//    Everything else can be moved around.

// Which Side of the robot you are referencing, with Pusher being the front.
enum Side {
  RIGHT = 0b01,
  LEFT = 0b10,
  BOTH = 0b11,
};

// Ultrasonic Sensor pins and constants.
const int ECHO_PIN = 6;
const int TRIG_PIN = 7;
// Using the speed of sound at 50% humidity and 20 degrees C, cm/us.
const double SOUND_SPEED_CONSTANT = 0.0344;
const double WALL_THRESHOLD = 8.0; // TODO: Tweak and use in code
// Servo pin and hook constants.
const int SERVO_PIN = 9;
const int HOOK_MIN = 125;
const int HOOK_MAX = 255;
const bool HOOK_UP = true
// Motor A connections
const int RIGHT_EN = 10;
const int RIGHT_DIR_1 = 2;
const int RIGHT_DIR_2 = 3;
// Motor B connections.
const int LEFT_EN = 11;
const int LEFT_DIR_1 = 4;
const int LEFT_DIR_2 = 5;

/**
 * Read the ultrasonic sensor, scaled roughly to centimeters. It may return -1 if out of range (> 4m or <2cm).
 */
 double readUltrasonicDistance() {
    float duration, distance;
    digitalWrite(trigPin, LOW); 
    delayMicroseconds(2);
   
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    duration = pulseIn(echoPin, HIGH); // Duration of pulse in microseconds
    distance = (duration / 2) * SOUND_SPEED_CONSTANT;
    
    if (distance >= 400 || distance <= 2){
      return -1; // Out of range
    }
    else {
      return distance;
    }
 }

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

int hookPos = 0;
void setHook(bool hookUp) {
  int dir = -1;
  if (hookUp)
    dir = 1;
  if (hookPos == 0) {
    hookPos++;
  } else if (hookPos == 255) {
    hookPos--;
  }
  for (; (hookPos < 255) && (hookPos > 0); hookPos += dir) {
    analogWrite(SERVO_PIN, hookPos); 
    delay(10);
  }
}

void setup() {
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

}

void loop() {
}
