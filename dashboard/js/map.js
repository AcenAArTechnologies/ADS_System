const map = L.map('map').setView([20.5937, 78.9629], 5); // default: India-wide view, adjust as needed
L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
  attribution: '&copy; OpenStreetMap contributors',
}).addTo(map);

const accidentMarkers = new Map(); // accident_id -> marker
const ambulanceMarkers = new Map(); // device_id -> marker
const routeLines = new Map(); // ambulance_id -> polyline

const accidentIcon = L.divIcon({ className: 'accident-icon', html: '🚨', iconSize: [24, 24] });
const ambulanceIcon = L.divIcon({ className: 'ambulance-icon', html: '🚑', iconSize: [24, 24] });

function upsertAccidentMarker(a) {
  if (accidentMarkers.has(a.id)) {
    accidentMarkers.get(a.id).setLatLng([a.lat, a.lon]);
  } else {
    const m = L.marker([a.lat, a.lon], { icon: accidentIcon }).addTo(map);
    m.bindPopup(`Accident ${a.id}<br/>Device: ${a.device_id}<br/>Detected: ${formatServerTime(a.created_at)}`);
    accidentMarkers.set(a.id, m);
  }
  accidentMarkers.get(a.id)._data = a;
  renderAccidentList();
}

function upsertAmbulanceMarker(loc) {
  if (ambulanceMarkers.has(loc.device_id)) {
    ambulanceMarkers.get(loc.device_id).setLatLng([loc.lat, loc.lon]);
  } else {
    const m = L.marker([loc.lat, loc.lon], { icon: ambulanceIcon }).addTo(map);
    m.bindPopup(`Ambulance ${loc.device_id}`);
    ambulanceMarkers.set(loc.device_id, m);
  }
  ambulanceMarkers.get(loc.device_id)._last = loc;
  renderAmbulanceList();
}

function drawRoute(assignment) {
  if (!assignment.geometry) return;
  const latlngs = assignment.geometry.coordinates.map(([lon, lat]) => [lat, lon]);
  if (routeLines.has(assignment.ambulance_id)) map.removeLayer(routeLines.get(assignment.ambulance_id));
  const line = L.polyline(latlngs, { color: '#a3242b', weight: 4 }).addTo(map);
  routeLines.set(assignment.ambulance_id, line);
}

function formatServerTime(createdAt) {
  if (!createdAt) return 'unknown time';
  // created_at is stored as UTC (SQLite datetime('now')); mark it explicitly so
  // the Date parser doesn't treat it as local time.
  const iso = createdAt.includes('T') ? createdAt : `${createdAt.replace(' ', 'T')}Z`;
  const d = new Date(iso);
  if (Number.isNaN(d.getTime())) return createdAt;
  return d.toLocaleString();
}

function renderAccidentList() {
  const ul = document.getElementById('accident-list');
  ul.innerHTML = '';
  for (const [id, m] of accidentMarkers) {
    const li = document.createElement('li');
    const data = m._data || {};
    li.textContent = `${id.slice(0, 8)} — ${m.getLatLng().lat.toFixed(4)}, ${m.getLatLng().lng.toFixed(4)} — ${formatServerTime(data.created_at)}`;
    ul.appendChild(li);
  }
}

function renderAmbulanceList() {
  const ul = document.getElementById('ambulance-list');
  ul.innerHTML = '';
  for (const [id, m] of ambulanceMarkers) {
    const li = document.createElement('li');
    const loc = m._last || {};
    li.textContent = `${id} — ${loc.status || 'unknown'} (${(loc.speed_kmh ?? 0).toFixed(0)} km/h)`;
    ul.appendChild(li);
  }
}

async function loadInitial() {
  const accidents = await fetch('/api/accidents?active=true').then((r) => r.json());
  accidents.forEach(upsertAccidentMarker);
  const ambulances = await fetch('/api/ambulances').then((r) => r.json());
  ambulances.filter((a) => a.lat != null).forEach((a) => upsertAmbulanceMarker({ ...a, device_id: a.device_id }));
}

function connectWs() {
  const proto = location.protocol === 'https:' ? 'wss' : 'ws';
  const ws = new WebSocket(`${proto}://${location.host}/ws`);
  const statusEl = document.getElementById('conn-status');

  ws.onopen = () => {
    statusEl.textContent = 'live';
    statusEl.className = 'status ok';
  };
  ws.onclose = () => {
    statusEl.textContent = 'disconnected';
    statusEl.className = 'status err';
    setTimeout(connectWs, 3000);
  };
  ws.onmessage = (evt) => {
    const { type, payload } = JSON.parse(evt.data);
    if (type === 'accident_created') upsertAccidentMarker(payload);
    else if (type === 'ambulance_location') upsertAmbulanceMarker(payload);
    else if (type === 'assignment_created') drawRoute(payload);
  };
}

loadInitial();
connectWs();
