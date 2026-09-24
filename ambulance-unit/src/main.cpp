#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "config.h"
#include "gps.h"
#include "gsm.h"
#include "mqtt_client.h"

static uint32_t lastLocationPublish = 0;
static uint32_t lastSmsSent = 0;
static String assignedAccidentId = "";
static double assignedEtaMinutes = -1;

static void onMqttMessage(const char *topic, const String &payload) {
  if (String(topic) == MQTT_TOPIC_ASSIGNMENT) {
    JsonDocument doc;
    if (deserializeJson(doc, payload) == DeserializationError::Ok) {
      assignedAccidentId = doc["accident_id"].as<String>();
      assignedEtaMinutes = doc["eta_minutes"] | -1.0;
      Serial.printf("Assigned to accident %s, ETA %.1f min\n", assignedAccidentId.c_str(), assignedEtaMinutes);
    }
  }
}

static void publishLocation() {
  GpsFix fix = gpsGetFix();
  if (!fix.valid) return;

  JsonDocument doc;
  doc["device_id"] = DEVICE_ID;
  doc["timestamp"] = (uint32_t)time(nullptr);
  doc["lat"] = fix.lat;
  doc["lon"] = fix.lon;
  doc["speed_kmh"] = fix.speedKmh;
  doc["status"] = assignedAccidentId.length() ? "assigned" : "available";

  String out;
  serializeJson(doc, out);
  mqttPublish(MQTT_TOPIC_LOCATION, out, false);
}

static void sendLocationSms() {
  GpsFix fix = gpsGetFix();
  if (!fix.valid) return;
  String link = "https://www.openstreetmap.org/?mlat=" + String(fix.lat, 6) + "&mlon=" + String(fix.lon, 6);
  String text = "Ambulance " DEVICE_ID " location: " + link;
  gsmSendSms(DISPATCH_PHONE_NUMBER, text);
}

void setup() {
  Serial.begin(115200);

  gpsInit();
  gsmInit();
  gsmWaitForNetwork();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  mqttInit(onMqttMessage);
}

void loop() {
  gpsPoll();
  mqttLoop();

  uint32_t now = millis();

  if (now - lastLocationPublish >= LOCATION_PUBLISH_INTERVAL_MS) {
    lastLocationPublish = now;
    publishLocation();
  }

  if (now - lastSmsSent >= SMS_LOCATION_INTERVAL_MS) {
    lastSmsSent = now;
    sendLocationSms();
  }
}
