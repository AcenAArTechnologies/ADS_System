#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "camera.h"
#include "sd_buffer.h"
#include "telegram.h"

static bool triggered = false;
static bool retriedPendingAfterBoot = false;

void setup() {
  Serial.begin(115200);
  pinMode(TRIGGER_PIN, INPUT_PULLDOWN);

  if (!cameraInit()) {
    Serial.println("Camera init failed");
  }
  if (!sdBufferInit()) {
    Serial.println("SD init failed - clips cannot be saved");
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED && !retriedPendingAfterBoot) {
    retriedPendingAfterBoot = true;
    telegramRetryPending(); // upload any clip left over from a prior power loss / WiFi outage
  }

  if (!sdBufferIsRecording()) {
    if (digitalRead(TRIGGER_PIN) == HIGH && !triggered) {
      triggered = true;
      Serial.println("Trigger received - saving clip");
      sdBufferStartRecording();
    } else if (digitalRead(TRIGGER_PIN) == LOW) {
      triggered = false; // re-arm once the trigger pulse ends
    }
    sdBufferPollIdle();
  } else {
    if (sdBufferPollRecording()) {
      String path = sdBufferLastClipPath();
      Serial.println("Clip saved: " + path);
      if (WiFi.status() == WL_CONNECTED) {
        bool ok = telegramUploadVideo(path);
        Serial.println(ok ? "Uploaded to Telegram" : "Upload failed - will retry on reconnect/reboot");
      } else {
        Serial.println("No WiFi - clip kept on SD for retry");
      }
    }
  }
}
