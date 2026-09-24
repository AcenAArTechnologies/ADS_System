# Claude Code Prompt — IoT Accident Detection & Ambulance Navigation System

Copy everything below into Claude Code as your project brief.

---

## Project Overview

Build a full-stack IoT system called **"Accident Detection, Alert & Ambulance Navigation System"** with three parts:

1. **Vehicle Unit firmware** (ESP32-S3 + XIAO ESP32-S3 Sense camera)
2. **Ambulance Unit firmware** (ESP32)
3. **Backend Server** (Node.js/Python — your choice, propose one) that ingests data over MQTT, calculates distance/ETA using OpenStreetMap-based routing (no Google Maps, no paid APIs), stores locations, and exposes a live tracking dashboard/API.

All components have **independent internet connectivity** (WiFi/GPRS) — the vehicle unit and ambulance unit are NOT dependent on each other's network; they each talk to the server separately.

---

## 1. Vehicle Unit (ESP32-S3)

**Hardware:** ESP32-S3, NEO-6M/similar GPS module, SIM800L GSM module, MPU6050 accelerometer/gyroscope, buzzer, SH1106 OLED display (I2C, 128x64).

**Behavior:**
- Continuously read MPU6050 data and run a simple crash-detection algorithm (sudden deceleration / high-G impact / abnormal orientation change threshold-based, not ML for v1 — but structure the code so a TFLite Micro model could be swapped in later).
- Continuously read GPS coordinates.
- Show live status on the SH1106 OLED (GPS lock status, GSM signal, system armed/idle, last event).
- On accident detection:
  - Sound the buzzer.
  - Send an SMS via SIM800 GSM to a **configured emergency/home number** with the GPS coordinates and a Google-Maps-free plain lat/long link (e.g. `https://www.openstreetmap.org/?mlat=..&mlon=..`).
  - Publish an "accident event" payload (device ID, timestamp, GPS lat/long) to the **server** — over MQTT if WiFi is available, else queue and retry; also send the same payload via GSM/GPRS as a backup channel if WiFi is unavailable.
  - Include a manual cancel/reset button or short countdown (e.g. 10s) before alerting, to allow false-positive cancellation.
- Use FreeRTOS tasks (or simple non-blocking loops) so GPS reading, MPU6050 polling, GSM operations, and OLED updates don't block each other.

**Deliverables for this unit:** Arduino/PlatformIO project structure, a `config.h` for WiFi/MQTT/GSM/APN credentials and phone numbers, modular files per peripheral (gps.cpp, gsm.cpp, mpu6050.cpp, oled.cpp, mqtt_client.cpp), and a README explaining wiring/pinout assumptions (ask me to confirm pins before finalizing).

---

## 2. Vehicle Camera Module (XIAO ESP32-S3 Sense)

**Hardware:** Seeed XIAO ESP32-S3 Sense (onboard camera + onboard SD card slot).

**Behavior:**
- Continuously buffer video to a rolling in-memory/SD ring buffer so that when an accident trigger is received (from the main vehicle unit, via a simple GPIO signal, UART, or ESP-NOW/WiFi message), it can save **50 seconds of footage** (pre-trigger + post-trigger, split however makes sense e.g. 20s before / 30s after) to the onboard SD card as an MP4/AVI file.
- After saving, automatically upload the video file to a **Telegram bot/chat** using the Telegram Bot API (`sendVideo`), using the module's own WiFi connection.
- If Telegram upload fails (no WiFi), keep the file on SD and retry on next boot/reconnect.
- Keep this as a **separate, independent firmware project** from the main Vehicle Unit — communicate only via a simple trigger signal.

**Deliverables:** PlatformIO project, wiring/trigger-signal doc, Telegram bot setup instructions (how to create a bot via BotFather and get the chat ID), and clear notes on the memory/storage constraints of continuous 50s buffering on this chip (propose the most realistic approach given XIAO ESP32-S3 Sense's PSRAM/SD limits).

---

## 3. Ambulance Unit (ESP32)

**Hardware:** ESP32, GPS module, GSM module (e.g. SIM800L).

**Behavior:**
- Continuously read GPS location.
- Publish location to the **server** every N seconds (configurable, e.g. every 5–10s) via **MQTT** over its own WiFi/GPRS internet connection.
- Separately, send the ambulance's current location via **SMS to a configured home/dispatch number** through the GSM module (on a timer, e.g. every few minutes, or on request).
- Show basic status (optional OLED, or just serial/log output) — reuse the SH1106 driver from the vehicle unit if a display is added later.
- When the server assigns this ambulance to an accident event, optionally display/log the target accident location and ETA it receives back from the server (if you want bidirectional MQTT topics for that).

**Deliverables:** PlatformIO project mirroring the Vehicle Unit's structure (gps.cpp, gsm.cpp, mqtt_client.cpp, config.h).

---

## 4. Backend Server

**Stack:** Propose Node.js (Express + MQTT.js) or Python (FastAPI + paho-mqtt) — pick whichever you think is cleaner for this, and justify briefly.

**Responsibilities:**
- **MQTT broker integration:** connect to a broker (use a free/self-hostable one — e.g. Mosquitto running locally/on the same server, or a free public test broker for development) and subscribe to:
  - `vehicle/{device_id}/accident` — accident event payloads from vehicle units
  - `ambulance/{device_id}/location` — continuous location stream from ambulance units
- **Data storage:** persist accident events and ambulance location history (SQLite/PostgreSQL — propose one; SQLite is fine for a project-scale system).
- **Distance & ETA calculation using OpenStreetMap — NOT Google Maps, NOT any paid API:**
  - Use **OSRM** (Open Source Routing Machine) — either the free public demo server (`router.project-osrm.org`, for development only) or instructions to self-host OSRM with an OSM extract for production — to calculate driving distance and ETA between the ambulance's current location and the accident location.
  - Alternative/fallback: OpenRouteService's free tier (also OSM-based) if OSRM setup is too heavy — mention this as an option but default to OSRM since it's fully free/self-hostable with no API key required.
  - On each new ambulance location update (or on a timer), recalculate distance/ETA to the currently active accident location and store/broadcast it.
- **Assignment logic (basic):** when an accident event comes in, find the nearest available/free ambulance (by straight-line distance first pass, then confirm with OSRM route distance) and mark it as assigned to that accident.
- **Live API/dashboard:**
  - REST or WebSocket endpoints so a frontend map (using **Leaflet.js + OpenStreetMap tiles**, not Google Maps) can show: all ambulance positions live, the active accident location(s), the calculated route line between assigned ambulance and accident, and live-updating ETA/distance.
  - A simple endpoint/page to view accident video clips forwarded from Telegram (or just a link back to the Telegram message) alongside the accident record.
- **Notifications:** optional — server can also relay an alert (e.g. via its own Telegram bot or webhook) when a new accident is registered, independent of the vehicle unit's own SMS.

**Deliverables:** server project scaffold, `.env.example` for MQTT broker URL, DB connection, OSRM endpoint, Telegram bot token; a `docs/architecture.md` explaining the full data flow end-to-end; a minimal Leaflet-based dashboard page for live testing.

---

## Cross-cutting requirements

- Every unit (Vehicle Unit, Camera Module, Ambulance Unit) must be **independently connected to the internet** — do not assume they relay through each other except the one explicit local trigger signal between the Vehicle Unit and its camera module.
- Use **OpenStreetMap-based tools only** for maps/routing/geocoding anywhere in this project (Leaflet + OSM tiles for display, OSRM/OpenRouteService for routing) — explicitly avoid any Google Maps API usage.
- Keep each firmware project and the server as **separate folders/repos** with their own README, so they can be built, flashed, and deployed independently.
- Start by proposing the full architecture and a data-flow diagram (as text/mermaid) and confirming pin assignments, MQTT topic names, and payload JSON schemas with me **before** writing firmware code.
- Write clean, modular, well-commented code (Arduino/PlatformIO style for firmware) so this can be extended later with an ML-based crash classifier and a full mobile app.

---

## First step I want from you (Claude Code)

1. Propose the exact MQTT topic structure and JSON payload schema for accident events and ambulance location updates.
2. Propose the server tech stack and folder structure for the whole repo (monorepo with `/vehicle-unit`, `/camera-module`, `/ambulance-unit`, `/server`, `/dashboard`).
3. Then scaffold the server first (since everything else talks to it), followed by the Vehicle Unit firmware, then the Ambulance Unit firmware, then the Camera Module firmware.
