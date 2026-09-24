#pragma once

// ---- Identity ----
#define DEVICE_ID "AMB-001"

// ---- WiFi ----
// This is the Ambulance Unit's OWN hotspot connection, independent of the
// Vehicle Unit's (which has its own WIFI_SSID/PASSWORD in its own
// config.h). The two units do not need to share a network - each just
// needs its own path to the MQTT broker. GSM below is SMS-only, never a
// data/MQTT transport. Distance/ETA on the server is computed purely from
// each device's GPS lat/lon in its MQTT payload - it does not depend on
// which WiFi network either device is on.
#define WIFI_SSID "your-ambulance-hotspot-ssid"
#define WIFI_PASSWORD "your-ambulance-hotspot-password"

// ---- MQTT ----
// IMPORTANT: if this unit and the Vehicle Unit are on two different
// hotspots (e.g. two different phones), a private LAN address like
// 192.168.1.100 will NOT be reachable from both. Point MQTT_HOST at a
// broker with a public IP/domain (a small cloud VM running Mosquitto, or a
// public test broker for development) so both units can reach it from
// wherever their own hotspot's internet connection goes.
#define MQTT_HOST "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""
#define MQTT_TOPIC_LOCATION "ambulance/" DEVICE_ID "/location"
#define MQTT_TOPIC_ASSIGNMENT "ambulance/" DEVICE_ID "/assignment"

// ---- GSM / SIM800L (SMS only) ----
#define DISPATCH_PHONE_NUMBER "+911234567890" // periodic location SMS destination

// ---- Pin assignments (generic ESP32 DevKit) — CONFIRM AGAINST YOUR WIRING ----
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17
#define GPS_BAUD 9600

#define GSM_RX_PIN 18
#define GSM_TX_PIN 19
#define GSM_BAUD 9600
#define GSM_RESET_PIN 5

// ---- Timing ----
#define GPS_READ_INTERVAL_MS 1000
#define LOCATION_PUBLISH_INTERVAL_MS 7000   // 5-10s configurable window
#define SMS_LOCATION_INTERVAL_MS 300000     // every 5 minutes
#define MQTT_RETRY_QUEUE_MAX 10
