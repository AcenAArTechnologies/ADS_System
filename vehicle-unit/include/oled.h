#pragma once
#include <Arduino.h>

struct OledStatus {
  bool gpsLocked = false;
  int gsmSignalPercent = -1;   // -1 = unknown
  bool armed = true;
  String lastEvent = "none";
};

void oledInit();
void oledUpdate(const OledStatus &status);
