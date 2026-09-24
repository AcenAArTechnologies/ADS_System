#include "mqtt_client.h"
#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>

struct QueuedMsg {
  String topic;
  String payload;
};

static WiFiClient wifiClient;
static PubSubClient mqtt(wifiClient);
static QueuedMsg queue[MQTT_RETRY_QUEUE_MAX];
static int queueCount = 0;
static uint32_t lastConnectAttempt = 0;
static MqttMessageCallback userCallback = nullptr;

static void internalCallback(char *topic, byte *payload, unsigned int length) {
  if (!userCallback) return;
  String msg;
  msg.reserve(length);
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  userCallback(topic, msg);
}

static void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void mqttInit(MqttMessageCallback onMessage) {
  userCallback = onMessage;
  ensureWifi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(internalCallback);
}

static bool ensureConnected() {
  if (WiFi.status() != WL_CONNECTED) return false;
  if (mqtt.connected()) return true;

  uint32_t now = millis();
  if (now - lastConnectAttempt < 3000) return false;
  lastConnectAttempt = now;

  String clientId = String(DEVICE_ID) + "-" + String((uint32_t)esp_random(), HEX);
  bool ok;
  if (strlen(MQTT_USERNAME) > 0) {
    ok = mqtt.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD);
  } else {
    ok = mqtt.connect(clientId.c_str());
  }
  if (ok) {
    mqtt.subscribe(MQTT_TOPIC_ASSIGNMENT, 1);
  }
  return ok;
}

void mqttLoop() {
  ensureWifi();
  if (ensureConnected()) {
    mqtt.loop();
    while (queueCount > 0 && mqtt.connected()) {
      QueuedMsg &m = queue[0];
      if (mqtt.publish(m.topic.c_str(), m.payload.c_str())) {
        for (int i = 1; i < queueCount; i++) queue[i - 1] = queue[i];
        queueCount--;
      } else {
        break;
      }
    }
  }
}

bool mqttIsConnected() { return mqtt.connected(); }

static void queueForRetry(const char *topic, const String &payload) {
  if (queueCount >= MQTT_RETRY_QUEUE_MAX) {
    for (int i = 1; i < queueCount; i++) queue[i - 1] = queue[i];
    queueCount--;
  }
  queue[queueCount].topic = topic;
  queue[queueCount].payload = payload;
  queueCount++;
}

bool mqttPublish(const char *topic, const String &payload, bool qos1) {
  (void)qos1;
  if (!ensureConnected()) {
    queueForRetry(topic, payload);
    return false;
  }
  if (!mqtt.publish(topic, payload.c_str())) {
    queueForRetry(topic, payload);
    return false;
  }
  return true;
}
