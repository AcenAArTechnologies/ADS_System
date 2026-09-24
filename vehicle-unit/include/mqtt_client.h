#pragma once
#include <Arduino.h>

// MQTT over WiFi only (the vehicle unit connects to a WiFi hotspot; GSM is
// used solely for SMS, see gsm.h). Publishes that fail while WiFi is down
// are queued and retried once connectivity returns.
void mqttInit();
void mqttLoop();                      // call every loop iteration; handles (re)connect + queue flush
bool mqttIsConnected();
bool mqttPublish(const char *topic, const String &payload, bool qos1 = false);
void mqttQueueForRetry(const char *topic, const String &payload); // used when publish fails outright
