const mqtt = require('mqtt');
const config = require('../config');

function connectMqtt() {
  const client = mqtt.connect(config.mqtt.url, {
    username: config.mqtt.username,
    password: config.mqtt.password,
    clientId: `ads-server-${Math.random().toString(16).slice(2, 10)}`,
    reconnectPeriod: 3000,
  });

  client.on('connect', () => {
    console.log(`[mqtt] connected to ${config.mqtt.url}`);
    client.subscribe('vehicle/+/accident', { qos: 1 });
    client.subscribe('vehicle/+/status', { qos: 0 });
    client.subscribe('ambulance/+/location', { qos: 0 });
  });

  client.on('reconnect', () => console.log('[mqtt] reconnecting...'));
  client.on('error', (err) => console.error('[mqtt] error', err.message));

  return client;
}

module.exports = { connectMqtt };
