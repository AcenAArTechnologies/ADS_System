const { WebSocketServer } = require('ws');

let wss = null;

function initWs(server) {
  wss = new WebSocketServer({ server, path: '/ws' });
  wss.on('connection', (ws) => {
    ws.send(JSON.stringify({ type: 'hello', message: 'connected to ADS live feed' }));
  });
  return wss;
}

function broadcast(type, payload) {
  if (!wss) return;
  const msg = JSON.stringify({ type, payload });
  for (const client of wss.clients) {
    if (client.readyState === 1) client.send(msg);
  }
}

module.exports = { initWs, broadcast };
