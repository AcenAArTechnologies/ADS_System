#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "config.h"
#include "gps.h"
#include "gsm.h"
#include "mpu6050.h"
#include "oled.h"
#include "mqtt_client.h"

enum class SystemState { ARMED, COUNTDOWN, ALERTING };

static SystemState state = SystemState::ARMED;
static uint32_t countdownStartMs = 0;
static uint32_t lastOledUpdate = 0;
static uint32_t lastStatusPublish = 0;
static String lastEvent = "none";

static void soundBuzzer(bool on) {
  digitalWrite(BUZZER_PIN, on ? HIGH : LOW);
}

static void triggerCameraModule() {
  digitalWrite(CAMERA_TRIGGER_PIN, HIGH);
  delay(200); // long enough for the camera module's edge/level detector to catch it
  digitalWrite(CAMERA_TRIGGER_PIN, LOW);
}

static String buildAccidentPayload(const GpsFix &fix, const ImuReading &imu) {
  JsonDocument doc;
  doc["device_id"] = DEVICE_ID;
  doc["timestamp"] = (uint32_t)time(nullptr); // seconds since epoch; server can normalize
  doc["lat"] = fix.lat;
  doc["lon"] = fix.lon;
  doc["event_type"] = "accident";
  doc["severity"] = imu.accelMagG > (1.0f + IMPACT_G_THRESHOLD * 1.5f) ? "high" : "medium";
  doc["impact_g"] = imu.accelMagG;
  doc["video_ref"] = (const char *)nullptr;
  String out;
  serializeJson(doc, out);
  return out;
}

static void sendEmergencySms(const GpsFix &fix) {
  String link = "https://www.openstreetmap.org/?mlat=" + String(fix.lat, 6) + "&mlon=" + String(fix.lon, 6);
  String text = "ACCIDENT DETECTED\nDevice: " DEVICE_ID "\nLocation: " + link;
  gsmSendSms(EMERGENCY_PHONE_NUMBER, text);
}

static void enterCountdown() {
  state = SystemState::COUNTDOWN;
  countdownStartMs = millis();
  soundBuzzer(true);
  lastEvent = "countdown";
}

static void cancelAlert() {
  state = SystemState::ARMED;
  soundBuzzer(false);
  lastEvent = "cancelled";
}

static void fireAlert() {
  state = SystemState::ALERTING;
  GpsFix fix = gpsGetFix();
  ImuReading imu = mpuGetLatest();

  triggerCameraModule();
  sendEmergencySms(fix);

  String payload = buildAccidentPayload(fix, imu);
  mqttPublish(MQTT_TOPIC_ACCIDENT, payload, /*qos1=*/true);

  lastEvent = "ALERT SENT";
}

void setup() {
  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(CAMERA_TRIGGER_PIN, OUTPUT);
  pinMode(CANCEL_BUTTON_PIN, INPUT_PULLUP);
  digitalWrite(CAMERA_TRIGGER_PIN, LOW);

  gpsInit();
  gsmInit();
  gsmWaitForNetwork();
  oledInit();

  if (!mpuInit()) {
    Serial.println("MPU6050 init failed - check wiring");
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  mqttInit();
}

void loop() {
  gpsPoll();
  mpuPoll();
  mqttLoop();

  // Manual cancel button, active LOW.
  if (digitalRead(CANCEL_BUTTON_PIN) == LOW && state == SystemState::COUNTDOWN) {
    cancelAlert();
  }

  ImuReading imu = mpuGetLatest();
  if (state == SystemState::ARMED && mpuDetectCrash(imu)) {
    enterCountdown();
  }

  if (state == SystemState::COUNTDOWN && millis() - countdownStartMs >= ALERT_COUNTDOWN_MS) {
    fireAlert();
  }

  if (state == SystemState::ALERTING) {
    // Re-arm automatically after the alert has been sent; a real deployment
    // might wait for an external reset instead.
    soundBuzzer(false);
    state = SystemState::ARMED;
  }

  uint32_t now = millis();
  if (now - lastOledUpdate >= OLED_UPDATE_INTERVAL_MS) {
    lastOledUpdate = now;
    OledStatus status;
    status.gpsLocked = gpsHasFix();
    status.gsmSignalPercent = gsmSignalPercent();
    status.armed = (state == SystemState::ARMED);
    status.lastEvent = lastEvent;
    oledUpdate(status);
  }

  if (now - lastStatusPublish >= STATUS_PUBLISH_INTERVAL_MS) {
    lastStatusPublish = now;
    JsonDocument doc;
    doc["device_id"] = DEVICE_ID;
    doc["gps_locked"] = gpsHasFix();
    doc["state"] = (state == SystemState::ARMED) ? "armed" : (state == SystemState::COUNTDOWN ? "countdown" : "alerting");
    String out;
    serializeJson(doc, out);
    mqttPublish(MQTT_TOPIC_STATUS, out, false);
  }
}
