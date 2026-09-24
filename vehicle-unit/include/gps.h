#pragma once
#include <Arduino.h>

struct GpsFix {
  bool valid = false;
  double lat = 0;
  double lon = 0;
  double speedKmh = 0;
  uint32_t satellites = 0;
};

void gpsInit();
void gpsPoll();             // call frequently from loop; non-blocking
GpsFix gpsGetFix();
bool gpsHasFix();
