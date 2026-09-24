#include "oled.h"
#include "config.h"
#include <U8g2lib.h>
#include <string.h>

static U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, OLED_SCL_PIN, OLED_SDA_PIN);

void oledInit() {
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);
}

// Small medical/alert cross-in-circle icon, drawn with primitives (no bitmap
// asset needed). Centered at (cx, cy) with the given radius.
static void drawCrossBadge(int cx, int cy, int r) {
  u8g2.drawCircle(cx, cy, r, U8G2_DRAW_ALL);
  int arm = r - 2;
  u8g2.drawBox(cx - 1, cy - arm, 2, arm * 2);
  u8g2.drawBox(cx - arm, cy - 1, arm * 2, 2);
}

void oledSplash() {
  u8g2.clearBuffer();
  u8g2.drawRFrame(0, 0, 128, 64, 4);

  drawCrossBadge(15, 32, 9);

  u8g2.setFont(u8g2_font_helvB14_tr);
  const char *brand = "AcenAAr";
  int bw = u8g2.getStrWidth(brand);
  u8g2.drawStr(30 + (128 - 30 - bw) / 2, 30, brand);

  u8g2.setFont(u8g2_font_6x10_tf);
  const char *tagline = "Accident Detection";
  int tw = u8g2.getStrWidth(tagline);
  u8g2.drawStr((128 - tw) / 2, 46, tagline);

  const char *sub = "System";
  int sw = u8g2.getStrWidth(sub);
  u8g2.drawStr((128 - sw) / 2, 58, sub);

  u8g2.sendBuffer();
  delay(2000);
}

struct BootItem {
  char label[13];
  bool ok;
};

static BootItem bootItems[8];
static int bootCount = 0;

static void drawBootChecklist() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 9, "AcenAAr - Booting");
  u8g2.drawHLine(0, 11, 128);

  u8g2.setFont(u8g2_font_5x7_tf);
  for (int i = 0; i < bootCount; i++) {
    int y = 21 + i * 8;
    u8g2.drawStr(0, y, bootItems[i].label);
    const char *st = bootItems[i].ok ? "OK" : "FAIL";
    int w = u8g2.getStrWidth(st);
    u8g2.drawStr(128 - w, y, st);
  }

  u8g2.sendBuffer();
}

void oledBootBegin() {
  bootCount = 0;
  drawBootChecklist();
}

void oledBootStatus(const char *label, bool ok) {
  int idx = -1;
  for (int i = 0; i < bootCount; i++) {
    if (strncmp(bootItems[i].label, label, sizeof(bootItems[i].label) - 1) == 0) {
      idx = i;
      break;
    }
  }
  if (idx < 0 && bootCount < (int)(sizeof(bootItems) / sizeof(bootItems[0]))) {
    idx = bootCount++;
    strncpy(bootItems[idx].label, label, sizeof(bootItems[idx].label) - 1);
    bootItems[idx].label[sizeof(bootItems[idx].label) - 1] = '\0';
  }
  if (idx >= 0) bootItems[idx].ok = ok;

  drawBootChecklist();
}

void oledUpdate(const OledStatus &status) {
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 9, "AcenAAr ADS");
  u8g2.drawHLine(0, 11, 128);

  u8g2.setFont(u8g2_font_5x7_tf);
  char line[32];

  snprintf(line, sizeof(line), "GPS: %s", status.gpsLocked ? "LOCKED" : "SEARCHING");
  u8g2.drawStr(0, 21, line);

  if (status.gsmSignalPercent >= 0) {
    snprintf(line, sizeof(line), "GSM: %d%%", status.gsmSignalPercent);
  } else {
    snprintf(line, sizeof(line), "GSM: --");
  }
  u8g2.drawStr(0, 29, line);

  snprintf(line, sizeof(line), "WiFi: %s   MQTT: %s",
           status.wifiConnected ? "OK" : "--",
           status.mqttConnected ? "OK" : "--");
  u8g2.drawStr(0, 37, line);

  snprintf(line, sizeof(line), "State: %s", status.armed ? "ARMED" : "ALERTING");
  u8g2.drawStr(0, 45, line);

  snprintf(line, sizeof(line), "Last: %s", status.lastEvent.c_str());
  u8g2.drawStr(0, 53, line);

  u8g2.sendBuffer();
}
