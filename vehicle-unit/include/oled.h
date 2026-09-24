#pragma once
#include <Arduino.h>

struct OledStatus {
  bool gpsLocked = false;
  int gsmSignalPercent = -1;   // -1 = unknown
  bool wifiConnected = false;
  bool mqttConnected = false;
  bool armed = true;
  String lastEvent = "none";
};

void oledInit();

// AcenAAr branded splash screen, shown once at boot. Blocks for a short,
// fixed hold time so the logo is actually readable.
void oledSplash();

// Boot checklist: oledBootBegin() clears/starts the list, then one
// oledBootStatus() call per subsystem as setup() brings it up. Each call
// redraws the whole checklist so progress is visible line-by-line.
void oledBootBegin();
void oledBootStatus(const char *label, bool ok);

void oledUpdate(const OledStatus &status);
