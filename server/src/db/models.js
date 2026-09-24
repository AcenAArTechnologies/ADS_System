const db = require('./index');

const insertAccidentStmt = db.prepare(`INSERT INTO accidents
  (id, device_id, timestamp, lat, lon, severity, impact_g, video_ref, status)
  VALUES (?, ?, ?, ?, ?, ?, ?, ?, 'active')`);
const getAccidentStmt = db.prepare('SELECT * FROM accidents WHERE id = ?');
const listActiveAccidentsStmt = db.prepare("SELECT * FROM accidents WHERE status != 'resolved' ORDER BY created_at DESC");
const listAllAccidentsStmt = db.prepare('SELECT * FROM accidents ORDER BY created_at DESC LIMIT ?');
const assignAccidentStmt = db.prepare("UPDATE accidents SET status = 'assigned', assigned_ambulance_id = ? WHERE id = ?");
const resolveAccidentStmt = db.prepare("UPDATE accidents SET status = 'resolved' WHERE id = ?");
const setVideoRefStmt = db.prepare('UPDATE accidents SET video_ref = ? WHERE id = ?');

const Accidents = {
  insert(a) {
    insertAccidentStmt.run(
      a.id, a.device_id, a.timestamp, a.lat, a.lon, a.severity ?? null, a.impact_g ?? null, a.video_ref ?? null
    );
    return a;
  },
  get(id) {
    return getAccidentStmt.get(id);
  },
  listActive() {
    return listActiveAccidentsStmt.all();
  },
  listAll(limit = 100) {
    return listAllAccidentsStmt.all(limit);
  },
  assign(id, ambulanceId) {
    assignAccidentStmt.run(ambulanceId, id);
  },
  resolve(id) {
    resolveAccidentStmt.run(id);
  },
  setVideoRef(id, videoRef) {
    setVideoRefStmt.run(videoRef, id);
  },
};

const upsertAmbulanceStmt = db.prepare(`INSERT INTO ambulances (device_id, lat, lon, speed_kmh, status, last_seen)
  VALUES (?, ?, ?, ?, COALESCE(?, 'available'), ?)
  ON CONFLICT(device_id) DO UPDATE SET
    lat = excluded.lat, lon = excluded.lon, speed_kmh = excluded.speed_kmh,
    status = COALESCE(?, status), last_seen = excluded.last_seen`);
const insertAmbulanceLocationStmt = db.prepare(`INSERT INTO ambulance_locations (device_id, timestamp, lat, lon, speed_kmh)
  VALUES (?, ?, ?, ?, ?)`);
const setAmbulanceStatusStmt = db.prepare('UPDATE ambulances SET status = ? WHERE device_id = ?');
const getAmbulanceStmt = db.prepare('SELECT * FROM ambulances WHERE device_id = ?');
const listAmbulancesStmt = db.prepare('SELECT * FROM ambulances');
const listAvailableAmbulancesStmt = db.prepare("SELECT * FROM ambulances WHERE status = 'available' AND lat IS NOT NULL");

const Ambulances = {
  upsertLocation({ device_id, lat, lon, speed_kmh, status, timestamp }) {
    const st = status ?? null;
    upsertAmbulanceStmt.run(device_id, lat, lon, speed_kmh ?? null, st, timestamp, st);
    insertAmbulanceLocationStmt.run(device_id, timestamp, lat, lon, speed_kmh ?? null);
  },
  setStatus(deviceId, status) {
    setAmbulanceStatusStmt.run(status, deviceId);
  },
  get(deviceId) {
    return getAmbulanceStmt.get(deviceId);
  },
  listAll() {
    return listAmbulancesStmt.all();
  },
  listAvailable() {
    return listAvailableAmbulancesStmt.all();
  },
};

const insertAssignmentStmt = db.prepare(`INSERT INTO assignments (accident_id, ambulance_id, distance_km, eta_minutes)
  VALUES (?, ?, ?, ?)`);
const latestAssignmentStmt = db.prepare('SELECT * FROM assignments WHERE accident_id = ? ORDER BY created_at DESC LIMIT 1');

const Assignments = {
  insert({ accident_id, ambulance_id, distance_km, eta_minutes }) {
    insertAssignmentStmt.run(accident_id, ambulance_id, distance_km, eta_minutes);
  },
  latestForAccident(accidentId) {
    return latestAssignmentStmt.get(accidentId);
  },
};

module.exports = { Accidents, Ambulances, Assignments };
