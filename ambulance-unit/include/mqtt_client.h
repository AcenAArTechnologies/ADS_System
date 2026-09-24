#pragma once
#include <Arduino.h>

// MQTT over WiFi only (the ambulance unit connects to a WiFi hotspot; GSM is
// used solely for SMS, see gsm.h).
typedef void (*MqttMessageCallback)(const char *topic, const String &payload);

void mqttInit(MqttMessageCallback onMessage);
void mqttLoop();
bool mqttIsConnected();
bool mqttPublish(const char *topic, const String &payload, bool qos1 = false);
