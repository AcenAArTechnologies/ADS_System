#include "oled.h"
#include "config.h"
#include <U8g2lib.h>

static U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, OLED_SCL_PIN, OLED_SDA_PIN);

void oledInit() {
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);
}

void oledUpdate(const OledStatus &status) {
  u8g2.clearBuffer();

  u8g2.drawStr(0, 10, "ADS Vehicle Unit");
  u8g2.drawHLine(0, 12, 128);

  char line[32];
  snprintf(line, sizeof(line), "GPS: %s", status.gpsLocked ? "LOCKED" : "SEARCHING");
  u8g2.drawStr(0, 26, line);

  if (status.gsmSignalPercent >= 0) {
    snprintf(line, sizeof(line), "GSM: %d%%", status.gsmSignalPercent);
  } else {
    snprintf(line, sizeof(line), "GSM: --");
  }
  u8g2.drawStr(0, 38, line);

  snprintf(line, sizeof(line), "State: %s", status.armed ? "ARMED" : "ALERTING");
  u8g2.drawStr(0, 50, line);

  snprintf(line, sizeof(line), "Last: %s", status.lastEvent.c_str());
  u8g2.drawStr(0, 62, line);

  u8g2.sendBuffer();
}
