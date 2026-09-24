const path = require('path');
const fs = require('fs');
const { DatabaseSync } = require('node:sqlite');
const config = require('../config');

const dir = path.dirname(config.dbPath);
if (!fs.existsSync(dir)) fs.mkdirSync(dir, { recursive: true });

const db = new DatabaseSync(config.dbPath);
db.exec('PRAGMA journal_mode = WAL');

db.exec(`
CREATE TABLE IF NOT EXISTS accidents (
  id TEXT PRIMARY KEY,
  device_id TEXT NOT NULL,
  timestamp TEXT NOT NULL,
  lat REAL NOT NULL,
  lon REAL NOT NULL,
  severity TEXT,
  impact_g REAL,
  video_ref TEXT,
  status TEXT NOT NULL DEFAULT 'active',
  assigned_ambulance_id TEXT,
  created_at TEXT NOT NULL DEFAULT (datetime('now'))
);

CREATE TABLE IF NOT EXISTS ambulances (
  device_id TEXT PRIMARY KEY,
  lat REAL,
  lon REAL,
  speed_kmh REAL,
  status TEXT NOT NULL DEFAULT 'available',
  last_seen TEXT
);

CREATE TABLE IF NOT EXISTS ambulance_locations (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  device_id TEXT NOT NULL,
  timestamp TEXT NOT NULL,
  lat REAL NOT NULL,
  lon REAL NOT NULL,
  speed_kmh REAL
);

CREATE TABLE IF NOT EXISTS assignments (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  accident_id TEXT NOT NULL,
  ambulance_id TEXT NOT NULL,
  distance_km REAL,
  eta_minutes REAL,
  created_at TEXT NOT NULL DEFAULT (datetime('now'))
);
`);

module.exports = db;
