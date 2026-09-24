const fetch = require('node-fetch');
const config = require('../config');

async function notifyTelegram(text) {
  if (!config.telegram.botToken || !config.telegram.chatId) return;
  const url = `https://api.telegram.org/bot${config.telegram.botToken}/sendMessage`;
  try {
    await fetch(url, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ chat_id: config.telegram.chatId, text }),
    });
  } catch (err) {
    console.error('[telegram] notify failed:', err.message);
  }
}

module.exports = { notifyTelegram };
