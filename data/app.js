// =====================================================
// app.js — Battery Experiments web UI
// =====================================================

let ws = null;
let wsReconnectTimer = null;

function connectWS() {
  const proto = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
  const url = `${proto}//${window.location.host}/ws`;
  ws = new WebSocket(url);

  ws.onopen = () => {
    setStatus(true);
    if (wsReconnectTimer) { clearTimeout(wsReconnectTimer); wsReconnectTimer = null; }
  };
  ws.onclose = () => {
    setStatus(false);
    wsReconnectTimer = setTimeout(connectWS, 3000);
  };
  ws.onerror = () => { setStatus(false); };
  ws.onmessage = (ev) => {
    try {
      const s = JSON.parse(ev.data);
      updateUI(s);
    } catch (e) { console.error(e); }
  };
}

function setStatus(online) {
  document.getElementById('dot-ws').classList.toggle('online', online);
  document.getElementById('ws-status').textContent = online ? 'online' : 'offline';
}

function fmt(v, digits = 2) {
  if (v === undefined || v === null) return '--';
  return Number(v).toFixed(digits);
}

function updateUI(s) {
  document.getElementById('uptime').textContent = `uptime: ${s.uptime_s}с`;
  document.getElementById('soc').textContent = fmt(s.soc, 1);
  document.getElementById('vbat').textContent = fmt(s.V_bat, 2);
  const I_charge_mA = s.I_charge * 1000;
  const I_load_mA = s.I_load * 1000;
  document.getElementById('icharge').textContent = fmt(I_charge_mA, 0);
  document.getElementById('iload').textContent = fmt(I_load_mA, 0);

  const exp = s.experiment;
  const expStatus = document.getElementById('exp-status');

  if (exp.running) {
    expStatus.classList.remove('hidden');
    document.getElementById('exp-running-name').textContent = exp.name;
    document.getElementById('exp-elapsed').textContent = exp.elapsed_s;
    document.getElementById('exp-start-soc').textContent = fmt(exp.startSOC, 1);
    document.getElementById('exp-current-soc').textContent = fmt(s.soc, 1);
    document.getElementById('exp-delta-soc').textContent = fmt(exp.deltaSOC, 1);
    document.getElementById('exp-ein').textContent = fmt(exp.energy_in_Wh, 3);
    document.getElementById('exp-eout').textContent = fmt(exp.energy_out_Wh, 3);
    document.getElementById('exp-samples').textContent = exp.samples;
  } else {
    expStatus.classList.add('hidden');
  }

  // Highlight active experiment in grid
  document.querySelectorAll('.exp-btn').forEach(b => {
    const t = parseInt(b.dataset.type);
    b.classList.toggle('active', exp.running && exp.type === t);
  });
}

// ============= API actions =============

async function startExperiment(type) {
  let param = 0;
  if (type === 6) {
    param = parseFloat(document.getElementById('e6-param').value) || 30;
  }
  const fd = new FormData();
  fd.append('type', type);
  fd.append('param', param);
  const r = await fetch('/api/experiment/start', { method: 'POST', body: fd });
  const j = await r.json();
  if (!j.ok) alert('Failed to start. Stop current experiment first.');
}

async function stopExperiment() {
  await fetch('/api/experiment/stop', { method: 'POST' });
}

async function setSOC(value) {
  const v = (value !== undefined) ? value : parseFloat(document.getElementById('soc-set').value);
  const fd = new FormData();
  fd.append('value', v);
  await fetch('/api/soc/set', { method: 'POST', body: fd });
}

async function refreshFiles() {
  const r = await fetch('/api/files');
  const arr = await r.json();
  const ul = document.getElementById('files-list');
  if (!arr.length) { ul.innerHTML = '<li>(порожньо)</li>'; return; }
  ul.innerHTML = arr.map(f =>
    `<li><span>${f.name} <small>(${f.size} B)</small></span>
     <a href="/api/download?file=${encodeURIComponent(f.name)}">завантажити</a></li>`
  ).join('');
}

async function refreshLog() {
  const r = await fetch('/api/log');
  const arr = await r.json();
  const div = document.getElementById('log');
  if (!arr.length) { div.innerHTML = '<div class="log-line">(порожньо)</div>'; return; }
  div.innerHTML = arr.reverse().map(e =>
    `<div class="log-line">
       <span class="log-time">${(e.t / 1000).toFixed(0)}s</span>
       <span class="log-type">[${e.type}]</span>
       <span>${e.msg}</span>
     </div>`
  ).join('');
}

// Wire up experiment buttons
document.querySelectorAll('.exp-btn').forEach(btn => {
  btn.addEventListener('click', e => {
    // Don't trigger on E6 input click
    if (e.target.tagName === 'INPUT') return;
    const type = parseInt(btn.dataset.type);
    if (confirm(`Запустити експеримент ${btn.querySelector('.exp-id').textContent}?`)) {
      startExperiment(type);
    }
  });
});

document.getElementById('btn-stop').addEventListener('click', () => {
  if (confirm('Зупинити поточний експеримент?')) stopExperiment();
});

// Initial load
connectWS();
refreshFiles();
refreshLog();
setInterval(() => { refreshFiles(); refreshLog(); }, 30000);
