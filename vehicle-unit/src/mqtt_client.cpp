#include "mqtt_client.h"
#include "config.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

struct QueuedMsg {
  String topic;
  String payload;
};

static WiFiClientSecure wifiClient;
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
  // HiveMQ Cloud (and most managed brokers) require TLS on 8883; setInsecure()
  // skips CA validation, which is fine for a device that only ever talks to
  // this one known broker but is not a substitute for pinning the real cert.
  wifiClient.setInsecure();
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
  bool ok = strlen(MQTT_USERNAME) > 0
      ? mqtt.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)
      : mqtt.connect(clientId.c_str());
  if (ok) {
    mqtt.subscribe(MQTT_TOPIC_DRIVE, 1);
  }
  return ok;
}

void mqttLoop() {
  ensureWifi();
  if (ensureConnected()) {
    mqtt.loop();
    // Flush anything queued while WiFi/broker was unavailable.
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

bool mqttPublish(const char *topic, const String &payload, bool qos1) {
  // PubSubClient only supports QoS0 publish natively; qos1 flag is accepted
  // for API/topic-doc parity and future broker-side dup handling.
  (void)qos1;
  if (!ensureConnected()) {
    mqttQueueForRetry(topic, payload);
    return false;
  }
  if (!mqtt.publish(topic, payload.c_str())) {
    mqttQueueForRetry(topic, payload);
    return false;
  }
  return true;
}

void mqttQueueForRetry(const char *topic, const String &payload) {
  if (queueCount >= MQTT_RETRY_QUEUE_MAX) {
    // Drop oldest to make room - losing an old accident event queued behind
    // a newer one is worse, so newest always wins a full queue.
    for (int i = 1; i < queueCount; i++) queue[i - 1] = queue[i];
    queueCount--;
  }
  queue[queueCount].topic = topic;
  queue[queueCount].payload = payload;
  queueCount++;
}
