#include "motor.h"
#include "config.h"

static const int PWM_FREQ_HZ = 5000;
static const int PWM_RESOLUTION_BITS = 8; // duty 0-255
static const int LEFT_PWM_CHANNEL = 0;
static const int RIGHT_PWM_CHANNEL = 1;

static void setLeftMotor(bool forward, uint8_t speed) {
  digitalWrite(MOTOR_LEFT_IN1_PIN, forward ? HIGH : LOW);
  digitalWrite(MOTOR_LEFT_IN2_PIN, forward ? LOW : HIGH);
  ledcWrite(LEFT_PWM_CHANNEL, speed);
}

static void setRightMotor(bool forward, uint8_t speed) {
  digitalWrite(MOTOR_RIGHT_IN1_PIN, forward ? HIGH : LOW);
  digitalWrite(MOTOR_RIGHT_IN2_PIN, forward ? LOW : HIGH);
  ledcWrite(RIGHT_PWM_CHANNEL, speed);
}

void motorInit() {
  pinMode(MOTOR_LEFT_IN1_PIN, OUTPUT);
  pinMode(MOTOR_LEFT_IN2_PIN, OUTPUT);
  pinMode(MOTOR_RIGHT_IN1_PIN, OUTPUT);
  pinMode(MOTOR_RIGHT_IN2_PIN, OUTPUT);

  ledcSetup(LEFT_PWM_CHANNEL, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
  ledcAttachPin(MOTOR_LEFT_EN_PIN, LEFT_PWM_CHANNEL);
  ledcSetup(RIGHT_PWM_CHANNEL, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
  ledcAttachPin(MOTOR_RIGHT_EN_PIN, RIGHT_PWM_CHANNEL);

  motorStop();
}

void motorStop() {
  digitalWrite(MOTOR_LEFT_IN1_PIN, LOW);
  digitalWrite(MOTOR_LEFT_IN2_PIN, LOW);
  digitalWrite(MOTOR_RIGHT_IN1_PIN, LOW);
  digitalWrite(MOTOR_RIGHT_IN2_PIN, LOW);
  ledcWrite(LEFT_PWM_CHANNEL, 0);
  ledcWrite(RIGHT_PWM_CHANNEL, 0);
}

// Pivot turns (one side forward, one side back) rather than single-side-only
// turns, since the chassis this drives has no steering mechanism of its own.
void motorApplyDriveCommand(const String &direction, uint8_t speed) {
  if (direction == "forward") {
    setLeftMotor(true, speed);
    setRightMotor(true, speed);
  } else if (direction == "backward") {
    setLeftMotor(false, speed);
    setRightMotor(false, speed);
  } else if (direction == "left") {
    setLeftMotor(false, speed);
    setRightMotor(true, speed);
  } else if (direction == "right") {
    setLeftMotor(true, speed);
    setRightMotor(false, speed);
  } else {
    motorStop();
  }
}
