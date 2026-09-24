const http = require('http');
const express = require('express');
const path = require('path');
const config = require('./config');
const { connectMqtt } = require('./mqtt/client');
const { registerHandlers } = require('./mqtt/handlers');
const { initWs } = require('./ws/broadcast');
const accidentsRouter = require('./routes/accidents');
const ambulancesRouter = require('./routes/ambulances');
const vehiclesRouter = require('./routes/vehicles');

const app = express();
app.use(express.json());

const mqttClient = connectMqtt();
registerHandlers(mqttClient);

app.use('/api/accidents', accidentsRouter);
app.use('/api/ambulances', ambulancesRouter);
app.use('/api/vehicles', vehiclesRouter(mqttClient));
app.get('/api/health', (req, res) => res.json({ ok: true }));

// Serve the Leaflet dashboard as static files.
app.use('/', express.static(path.join(__dirname, '..', '..', 'dashboard')));

const server = http.createServer(app);
initWs(server);

server.listen(config.port, () => {
  console.log(`[server] listening on http://localhost:${config.port}`);
});
