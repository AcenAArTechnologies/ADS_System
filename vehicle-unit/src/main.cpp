#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "config.h"
#include "gps.h"
#include "gsm.h"
#include "mpu6050.h"
#include "oled.h"
#include "mqtt_client.h"
#include "motor.h"

enum class SystemState { ARMED, COUNTDOWN, ALERTING };

static SystemState state = SystemState::ARMED;
static uint32_t countdownStartMs = 0;
static uint32_t lastOledUpdate = 0;
static uint32_t lastStatusPublish = 0;
static uint32_t lastDriveCommandMs = 0;
static uint32_t alertUntilMs = 0;
static String lastEvent = "none";

// Drive commands are ignored (and motors force-stopped) once a crash is
// detected, so a stuck/queued command can't keep the vehicle moving into
// whatever it just hit.
static void onMqttMessage(const char *topic, const String &payload) {
  if (String(topic) != MQTT_TOPIC_DRIVE) return;

  JsonDocument doc;
  if (deserializeJson(doc, payload) != DeserializationError::Ok) return;

  String direction = doc["direction"] | "stop";
  int speed = doc["speed"] | DRIVE_SPEED_DEFAULT;
  speed = constrain(speed, 0, 255);

  lastDriveCommandMs = millis();
  if (state == SystemState::ARMED) {
    motorApplyDriveCommand(direction, (uint8_t)speed);
  } else {
    motorStop();
  }
}

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
  motorStop();
  lastEvent = "countdown";
}

static void cancelAlert() {
  state = SystemState::ARMED;
  soundBuzzer(false);
  lastEvent = "cancelled";
}

static void fireAlert() {
  state = SystemState::ALERTING;
  // Keep the buzzer (already sounding since enterCountdown()) going through
  // the alert itself, instead of the ALERTING check below silencing it on
  // the very same loop() pass it was set - the buzzer should still be
  // audible at the moment the accident is actually reported, not just
  // during the cancel window that preceded it.
  alertUntilMs = millis() + ALERT_HOLD_MS;
  motorStop();
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

  oledInit();
  oledSplash();
  oledBootBegin();

  gpsInit();
  oledBootStatus("GPS", true);

  gsmInit();
  bool gsmOk = gsmWaitForNetwork();
  oledBootStatus("GSM", gsmOk);

  bool mpuOk = mpuInit();
  oledBootStatus("MPU6050", mpuOk);
  if (!mpuOk) {
    Serial.println("MPU6050 init failed - check wiring");
  }

  motorInit();
  oledBootStatus("Motor", true);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint32_t wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < WIFI_CONNECT_TIMEOUT_MS) {
    delay(200);
  }
  bool wifiOk = WiFi.status() == WL_CONNECTED;
  oledBootStatus("WiFi", wifiOk);

  mqttInit(onMqttMessage);
  bool mqttOk = false;
  if (wifiOk) {
    uint32_t mqttStart = millis();
    while (!mqttIsConnected() && millis() - mqttStart < MQTT_CONNECT_TIMEOUT_MS) {
      mqttLoop();
      delay(200);
    }
    mqttOk = mqttIsConnected();
  }
  oledBootStatus("MQTT", mqttOk);

  delay(1200); // hold the final checklist on screen before switching to the run view
}

void loop() {
  gpsPoll();
  mpuPoll();
  mqttLoop();

  // Link-loss failsafe: stop driving if no drive command has arrived recently.
  if (lastDriveCommandMs != 0 && millis() - lastDriveCommandMs >= DRIVE_COMMAND_TIMEOUT_MS) {
    motorStop();
    lastDriveCommandMs = 0;
  }

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

  if (state == SystemState::ALERTING && millis() >= alertUntilMs) {
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
    status.wifiConnected = (WiFi.status() == WL_CONNECTED);
    status.mqttConnected = mqttIsConnected();
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
