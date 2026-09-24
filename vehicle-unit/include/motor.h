#pragma once
#include <Arduino.h>

// L298N-driven differential drive (two DC motors, one per side).
void motorInit();
void motorStop();
void motorApplyDriveCommand(const String &direction, uint8_t speed);
