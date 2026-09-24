#pragma once
#include <Arduino.h>

// SIM800L is used for SMS only - all MQTT/data traffic goes over WiFi
// (see mqtt_client.cpp). No GPRS/data connection is brought up.
void gsmInit();
bool gsmWaitForNetwork(uint32_t timeoutMs = 15000);
int  gsmSignalPercent();
bool gsmSendSms(const char *number, const String &text);
