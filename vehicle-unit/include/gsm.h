#pragma once
#include <Arduino.h>

// SIM800L is used for SMS only in this system - all MQTT/data traffic goes
// over WiFi (see mqtt_client.cpp). No GPRS/data connection is brought up.
void gsmInit();
bool gsmWaitForNetwork(uint32_t timeoutMs = 15000);
int  gsmSignalPercent();    // -1 if unknown
bool gsmSendSms(const char *number, const String &text);
