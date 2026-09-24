// D-pad remote control. The vehicle-unit firmware auto-stops if no drive
// command arrives within DRIVE_COMMAND_TIMEOUT_MS (500ms), so a held button
// must keep re-sending its direction, not send it once.
const RESEND_INTERVAL_MS = 250;

const deviceIdInput = document.getElementById('drive-device-id');
const speedInput = document.getElementById('drive-speed');
const statusEl = document.getElementById('drive-status');
const dpad = document.getElementById('dpad');

let resendTimer = null;
let activeDirection = null;

function sendDrive(direction) {
  const deviceId = deviceIdInput.value.trim();
  if (!deviceId) {
    statusEl.textContent = 'set a device id';
    statusEl.className = 'status err';
    return;
  }
  const speed = Number(speedInput.value);
  fetch(`/api/vehicles/${encodeURIComponent(deviceId)}/drive`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ direction, speed }),
  })
    .then((r) => {
      if (!r.ok) throw new Error(`HTTP ${r.status}`);
      statusEl.textContent = direction;
      statusEl.className = direction === 'stop' ? 'status' : 'status ok';
    })
    .catch((err) => {
      statusEl.textContent = `send failed: ${err.message}`;
      statusEl.className = 'status err';
    });
}

function startDriving(direction) {
  if (direction === activeDirection) return;
  activeDirection = direction;
  sendDrive(direction);
  clearInterval(resendTimer);
  resendTimer = setInterval(() => sendDrive(activeDirection), RESEND_INTERVAL_MS);
}

function stopDriving() {
  if (activeDirection === null) return;
  clearInterval(resendTimer);
  resendTimer = null;
  activeDirection = null;
  sendDrive('stop');
}

dpad.querySelectorAll('.dpad-btn').forEach((btn) => {
  const direction = btn.dataset.direction;
  if (direction === 'stop') {
    btn.addEventListener('click', stopDriving);
    return;
  }
  btn.addEventListener('pointerdown', (e) => {
    e.preventDefault();
    startDriving(direction);
  });
  btn.addEventListener('pointerup', stopDriving);
  btn.addEventListener('pointerleave', stopDriving);
  btn.addEventListener('pointercancel', stopDriving);
});

// Stop driving if the tab loses focus/visibility so a background tab never
// keeps sending a held direction indefinitely.
document.addEventListener('visibilitychange', () => {
  if (document.hidden) stopDriving();
});
