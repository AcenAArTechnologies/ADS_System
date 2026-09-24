#include "gps.h"
#include "config.h"
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

static TinyGPSPlus gps;
static HardwareSerial gpsSerial(1);
static GpsFix currentFix;

void gpsInit() {
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
}

void gpsPoll() {
  while (gpsSerial.available() > 0) {
    if (gps.encode(gpsSerial.read())) {
      if (gps.location.isValid() && gps.location.isUpdated()) {
        currentFix.valid = true;
        currentFix.lat = gps.location.lat();
        currentFix.lon = gps.location.lng();
        currentFix.satellites = gps.satellites.isValid() ? gps.satellites.value() : 0;
        currentFix.speedKmh = gps.speed.isValid() ? gps.speed.kmph() : 0;
      }
    }
  }
}

GpsFix gpsGetFix() { return currentFix; }
bool gpsHasFix() { return currentFix.valid; }
