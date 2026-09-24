#pragma once
#include <Arduino.h>

struct ImuReading {
  float accelMagG = 1.0f;   // magnitude of acceleration vector in g
  float gyroMagDps = 0.0f;  // magnitude of angular velocity in deg/s
};

bool mpuInit();
void mpuPoll();                // call frequently from loop; non-blocking on its own interval
ImuReading mpuGetLatest();
// Simple threshold-based crash heuristic. Structured so a TFLite Micro
// classifier can later replace the body of this function without touching callers.
bool mpuDetectCrash(const ImuReading &r);
