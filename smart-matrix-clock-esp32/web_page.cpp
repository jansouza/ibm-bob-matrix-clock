#include "web_page.h"

// Raw-string literal R"=====(...)=====" so we can embed quotes, newlines, and
// all HTML/CSS/JS without any escaping.  The page is fully self-contained
// (no external resources) and communicates with the ESP32 via fetch().

const char WEB_PAGE_HTML[] = R"=====(
<!DOCTYPE html>
<html lang="pt">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>Smart Matrix Clock</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,"Segoe UI",system-ui,sans-serif;font-size:15px;
  line-height:1.6;background:#0d1117;color:#c9d1d9}
header{background:#161b22;border-bottom:1px solid #30363d;padding:12px 20px;
  display:flex;align-items:center;gap:12px}
header h1{font-size:18px;font-weight:600;color:#e6edf3}
header span{font-size:12px;color:#8b949e;background:#21262d;
  padding:2px 8px;border-radius:20px;font-family:monospace}
.container{max-width:680px;margin:0 auto;padding:20px}
.tabs{display:flex;gap:2px;border-bottom:1px solid #30363d;margin-bottom:20px}
.tab{padding:8px 16px;cursor:pointer;font-size:14px;border:none;
  background:none;border-bottom:2px solid transparent;color:#8b949e;
  transition:color .15s}
.tab:hover{color:#c9d1d9}
.tab.active{color:#58a6ff;border-bottom-color:#58a6ff;font-weight:500}
.panel{display:none}
.panel.active{display:block}
.card{background:#161b22;border:1px solid #30363d;border-radius:6px;
  padding:16px;margin-bottom:14px}
.card h2{font-size:14px;font-weight:600;margin-bottom:12px;color:#8b949e;
  text-transform:uppercase;letter-spacing:.05em}
.field{margin-bottom:12px}
.field label{display:block;font-size:13px;font-weight:500;
  margin-bottom:4px;color:#c9d1d9}
.field input,.field select{width:100%;padding:7px 10px;
  border:1px solid #30363d;border-radius:4px;font-size:14px;
  background:#0d1117;color:#c9d1d9}
.field input:focus,.field select:focus{outline:none;
  border-color:#58a6ff;box-shadow:0 0 0 2px rgba(88,166,255,.15)}
.field .hint{font-size:12px;color:#8b949e;margin-top:3px}
.field select option{background:#161b22;color:#c9d1d9}
.row{display:grid;grid-template-columns:1fr 1fr;gap:12px}
.row3{display:grid;grid-template-columns:1fr 1fr 1fr;gap:12px}
.btn{display:inline-block;padding:7px 16px;border-radius:4px;
  font-size:14px;font-weight:500;cursor:pointer;border:1px solid transparent;
  transition:background .15s}
.btn-primary{background:#238636;color:#fff;border-color:#238636}
.btn-primary:hover{background:#2ea043}
.btn-secondary{background:#21262d;color:#c9d1d9;border-color:#30363d}
.btn-secondary:hover{background:#30363d}
.btn-danger{background:#161b22;color:#f85149;border-color:#f85149}
.btn-danger:hover{background:#3d1e1e}
.actions{display:flex;gap:8px;justify-content:flex-end;margin-top:4px}
.badge{display:inline-block;padding:2px 8px;border-radius:12px;
  font-size:12px;font-weight:500}
.badge-ok{background:#1a4731;color:#3fb950}
.badge-warn{background:#3d2e00;color:#e3b341}
.preview{background:#000;color:#ff9a00;font-family:monospace;
  font-size:28px;letter-spacing:.15em;padding:12px 20px;
  border-radius:4px;text-align:center;min-height:54px;
  display:flex;align-items:center;justify-content:center;
  border:1px solid #30363d}
.status-bar{display:flex;gap:12px;flex-wrap:wrap;font-size:13px;
  margin-bottom:16px;color:#8b949e}
.status-bar span{display:flex;align-items:center;gap:4px}
.toggle-row{display:flex;align-items:center;justify-content:space-between;
  padding:4px 0}
.toggle{position:relative;display:inline-block;width:40px;height:22px}
.toggle input{opacity:0;width:0;height:0}
.slider-toggle{position:absolute;cursor:pointer;inset:0;background:#30363d;
  border-radius:22px;transition:.2s}
.slider-toggle:before{content:"";position:absolute;height:16px;width:16px;
  left:3px;bottom:3px;background:#8b949e;border-radius:50%;transition:.2s}
input:checked + .slider-toggle{background:#238636}
input:checked + .slider-toggle:before{transform:translateX(18px);background:#fff}
.key-box{font-family:monospace;font-size:13px;background:#0d1117;
  padding:6px 10px;border:1px solid #30363d;border-radius:4px;
  word-break:break-all;color:#c9d1d9}
.info-row{display:flex;justify-content:space-between;padding:5px 0;
  border-bottom:1px solid #21262d;font-size:14px}
.info-row:last-child{border:none}
.info-label{color:#8b949e}
.toast{position:fixed;bottom:20px;right:20px;background:#e6edf3;color:#0d1117;
  padding:10px 18px;border-radius:6px;font-size:14px;
  display:none;z-index:999;max-width:280px}
/* Range slider */
.range-field{margin-bottom:12px}
.range-field label{display:block;font-size:13px;font-weight:500;
  margin-bottom:4px;color:#c9d1d9}
.range-row{display:flex;align-items:center;gap:10px}
.range-row input[type=range]{flex:1;-webkit-appearance:none;appearance:none;
  height:6px;border-radius:3px;background:#30363d;outline:none;cursor:pointer}
.range-row input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;
  appearance:none;width:18px;height:18px;border-radius:50%;
  background:#58a6ff;cursor:pointer;border:2px solid #0d1117}
.range-row input[type=range]::-moz-range-thumb{width:18px;height:18px;
  border-radius:50%;background:#58a6ff;cursor:pointer;border:2px solid #0d1117}
.range-val{min-width:32px;text-align:right;font-size:14px;font-weight:600;
  color:#58a6ff;font-family:monospace}
</style>
</head>
<body>
<header>
  <h1>&#9719; Smart Matrix Clock</h1>
  <span id="ip-badge">...</span>
</header>
<div class="container">
  <div class="status-bar">
    <span>NTP: <strong id="st-ntp">--</strong></span>
    <span>Slot: <strong id="st-slot">--</strong></span>
    <span>WiFi: <strong id="st-ssid">--</strong></span>
  </div>

  <div class="tabs">
    <button class="tab active" data-tab="clock">&#128336; Rel&#243;gio</button>
    <button class="tab" data-tab="alert">&#128276; Alerta</button>
    <button class="tab" data-tab="weather">&#127780; Clima</button>
    <button class="tab" data-tab="quotes">&#128200; Cota&#231;&#245;es</button>
    <button class="tab" data-tab="network">&#128246; Rede</button>
    <button class="tab" data-tab="apikey">&#128273; API</button>
  </div>

  <!-- ── CLOCK TAB ────────────────────────────────────────────────────── -->
  <div class="panel active" id="tab-clock">
    <div class="card">
      <h2>Preview ao vivo</h2>
      <div class="preview" id="live-preview">--:--</div>
    </div>

    <div class="card">
      <h2>Hora e Fuso</h2>
      <div class="field">
        <label>Fuso hor&#225;rio (IANA)</label>
        <select id="cfg-timezone"></select>
      </div>
      <div class="row">
        <div class="field">
          <label>Idioma</label>
          <select id="cfg-language">
            <option value="pt">Portugu&#234;s</option>
            <option value="en">English</option>
          </select>
        </div>
        <div class="field">
          <label>Servidor NTP</label>
          <input id="cfg-ntp-server" type="text" maxlength="63"/>
        </div>
      </div>
    </div>

    <div class="card">
      <h2>Data</h2>
      <div class="toggle-row" style="margin-bottom:10px">
        <span style="font-size:13px;font-weight:500;color:#c9d1d9">Exibir data periodicamente</span>
        <label class="toggle"><input type="checkbox" id="cfg-date-en"/><span class="slider-toggle"></span></label>
      </div>
      <div id="date-options">
        <div class="field">
          <label>Intervalo de exibi&#231;&#227;o (segundos)</label>
          <input id="cfg-date-interval" type="number" min="5" max="300"/>
        </div>
      </div>
    </div>

    <div class="card">
      <h2>Display</h2>
      <div class="range-field">
        <label>Brilho: <span id="brightness-val">4</span> / 15</label>
        <div class="range-row">
          <span style="font-size:12px;color:#8b949e">0</span>
          <input id="cfg-brightness" type="range" min="0" max="15" value="4"/>
          <span style="font-size:12px;color:#8b949e">15</span>
          <span class="range-val" id="brightness-val-badge">4</span>
        </div>
      </div>
      <div class="range-field">
        <label>Velocidade de scroll: <span id="speed-val">50</span> ms/frame</label>
        <div class="range-row">
          <span style="font-size:12px;color:#8b949e">R&#225;pido</span>
          <input id="cfg-scroll-speed" type="range" min="10" max="200" value="50"/>
          <span style="font-size:12px;color:#8b949e">Lento</span>
          <span class="range-val" id="speed-val-badge">50</span>
        </div>
        <div class="hint" style="margin-top:6px">10 ms = mais r&#225;pido &mdash; 200 ms = mais lento</div>
      </div>
    </div>

    <div class="actions">
      <button class="btn btn-primary" id="btn-save-clock">Salvar configura&#231;&#245;es</button>
    </div>
  </div>

  <!-- ── ALERT TAB ───────────────────────────────────────────────────── -->
  <div class="panel" id="tab-alert">
    <div class="card">
      <h2>Mensagem de Alerta</h2>
      <div class="field">
        <label>Mensagem (m&#225;x. 127 chars)</label>
        <input id="alert-msg" type="text" maxlength="127" placeholder="Digite uma mensagem..."/>
      </div>
      <div class="row3">
        <div class="field">
          <label>Modo de exibi&#231;&#227;o</label>
          <select id="alert-mode">
            <option value="0">&#8618; Scroll</option>
            <option value="1">&#10022; Piscar</option>
            <option value="2">&#9632; Est&#225;tico</option>
            <option value="3">&#10022;&#8618; Piscar + Scroll</option>
          </select>
        </div>
        <div class="field">
          <label>Dura&#231;&#227;o (segundos)</label>
          <input id="alert-duration" type="number" min="1" max="60" value="5"/>
          <div class="hint">Tempo de exibi&#231;&#227;o no display</div>
        </div>
        <div class="field" style="display:flex;align-items:flex-end">
          <button class="btn btn-primary" id="btn-send-alert" style="width:100%">Enviar</button>
        </div>
      </div>
    </div>
  </div>

  <!-- ── WEATHER TAB ──────────────────────────────────────────────────── -->
  <div class="panel" id="tab-weather">
    <div class="card">
      <h2>Slot de Clima</h2>
      <div class="toggle-row">
        <span>Habilitar slot de clima</span>
        <label class="toggle"><input type="checkbox" id="cfg-weather-en"/><span class="slider-toggle"></span></label>
      </div>
    </div>
    <div class="card">
      <h2>Localiza&#231;&#227;o</h2>
      <div class="row">
        <div class="field">
          <label>Latitude</label>
          <input id="cfg-weather-lat" type="number" step="0.0001" min="-90" max="90"/>
        </div>
        <div class="field">
          <label>Longitude</label>
          <input id="cfg-weather-lon" type="number" step="0.0001" min="-180" max="180"/>
        </div>
      </div>
    </div>
    <div class="card">
      <h2>Configura&#231;&#245;es</h2>
      <div class="row">
        <div class="field">
          <label>Intervalo de atualiza&#231;&#227;o (minutos)</label>
          <input id="cfg-weather-update" type="number" min="5" max="1440"/>
        </div>
        <div class="field">
          <label>Tempo de exibi&#231;&#227;o (segundos)</label>
          <input id="cfg-weather-display" type="number" min="5" max="300"/>
        </div>
      </div>
      <div class="field">
        <label>Unidade de temperatura</label>
        <select id="cfg-temp-unit">
          <option value="C">Celsius (&#176;C)</option>
          <option value="F">Fahrenheit (&#176;F)</option>
        </select>
      </div>
    </div>
    <div class="actions">
      <button class="btn btn-primary" id="btn-save-weather">Salvar configura&#231;&#245;es</button>
    </div>
  </div>

  <!-- ── QUOTES TAB ───────────────────────────────────────────────────── -->
  <div class="panel" id="tab-quotes">
    <div class="card">
      <h2>Slot de Cota&#231;&#245;es</h2>
      <div class="toggle-row">
        <span>Habilitar slot de cota&#231;&#245;es</span>
        <label class="toggle"><input type="checkbox" id="cfg-quotes-en"/><span class="slider-toggle"></span></label>
      </div>
    </div>
    <div class="card">
      <h2>Ativos (tickers)</h2>
      <div class="field">
        <label>Tickers separados por v&#237;rgula</label>
        <input id="cfg-quotes-tickers" type="text" placeholder="PETR4.SA,AAPL,BTC-USD" maxlength="120"/>
        <div class="hint">Ex.: PETR4.SA, AAPL, BTC-USD &mdash; m&#225;x. 8 tickers</div>
      </div>
    </div>
    <div class="card">
      <h2>Configura&#231;&#245;es</h2>
      <div class="row">
        <div class="field">
          <label>Intervalo de atualiza&#231;&#227;o (minutos)</label>
          <input id="cfg-quotes-update" type="number" min="5" max="1440"/>
        </div>
        <div class="field">
          <label>Tempo de exibi&#231;&#227;o (segundos)</label>
          <input id="cfg-quotes-display" type="number" min="5" max="300"/>
        </div>
      </div>
    </div>
    <div class="actions">
      <button class="btn btn-primary" id="btn-save-quotes">Salvar configura&#231;&#245;es</button>
    </div>
  </div>

  <!-- ── NETWORK TAB ──────────────────────────────────────────────────── -->
  <div class="panel" id="tab-network">
    <div class="card">
      <h2>Conex&#227;o atual</h2>
      <div id="net-info">
        <div class="info-row"><span class="info-label">SSID</span><span id="net-ssid">--</span></div>
        <div class="info-row"><span class="info-label">IP</span><span id="net-ip">--</span></div>
        <div class="info-row"><span class="info-label">Status</span><span id="net-status">--</span></div>
      </div>
    </div>
    <div class="card">
      <h2>Nova rede WiFi</h2>
      <div class="field">
        <label>SSID</label>
        <input id="wifi-ssid" type="text" maxlength="32" autocomplete="off"/>
      </div>
      <div class="field">
        <label>Senha</label>
        <input id="wifi-pass" type="password" maxlength="64" autocomplete="new-password"/>
      </div>
      <div class="actions">
        <button class="btn btn-primary" id="btn-save-wifi">Salvar e reiniciar</button>
      </div>
    </div>
  </div>

  <!-- ── API TAB ──────────────────────────────────────────────────────── -->
  <div class="panel" id="tab-apikey">
    <div class="card">
      <h2>Autentica&#231;&#227;o de API</h2>
      <div class="toggle-row">
        <span>Exigir chave de API nos endpoints <code>/api/*</code></span>
        <label class="toggle"><input type="checkbox" id="cfg-api-auth"/><span class="slider-toggle"></span></label>
      </div>
      <p style="margin-top:10px;font-size:13px;color:#8b949e">
        Quando ativado, todas as requisi&#231;&#245;es de escrita e leitura de estado devem incluir o header
        <code>X-API-Key: &lt;chave&gt;</code>. A interface web e os endpoints
        <code>GET /api/config</code> e <code>GET /api/timezones</code> permanecem acess&#237;veis sem autentica&#231;&#227;o.
      </p>
    </div>
    <div class="card">
      <h2>Chave de API</h2>
      <div class="key-box" id="api-key-box">...</div>
      <div class="actions" style="margin-top:10px">
        <button class="btn btn-danger" id="btn-regen-key">Regenerar chave</button>
        <button class="btn btn-primary" id="btn-save-api">Salvar configura&#231;&#245;es</button>
      </div>
    </div>
  </div>

</div><!-- /container -->

<div class="toast" id="toast"></div>

<script>
// ── Utilities ────────────────────────────────────────────────────────────────
function showToast(msg, ms) {
  var t = document.getElementById('toast');
  t.textContent = msg;
  t.style.display = 'block';
  setTimeout(function(){ t.style.display = 'none'; }, ms || 2500);
}

function val(id) { return document.getElementById(id).value; }
function setVal(id, v) { document.getElementById(id).value = v; }
function checked(id) { return document.getElementById(id).checked; }
function setChecked(id, v) { document.getElementById(id).checked = !!v; }
function setText(id, v) { document.getElementById(id).textContent = v; }

// ── Range sliders live update ────────────────────────────────────────────────
document.getElementById('cfg-brightness').addEventListener('input', function() {
  setText('brightness-val', this.value);
  setText('brightness-val-badge', this.value);
});
document.getElementById('cfg-scroll-speed').addEventListener('input', function() {
  setText('speed-val', this.value);
  setText('speed-val-badge', this.value);
});

// ── Date enabled toggle ───────────────────────────────────────────────────────
document.getElementById('cfg-date-en').addEventListener('change', function() {
  document.getElementById('date-options').style.opacity = this.checked ? '1' : '0.4';
  document.getElementById('date-options').style.pointerEvents = this.checked ? '' : 'none';
});

// ── Tab switching ────────────────────────────────────────────────────────────
document.querySelectorAll('.tab').forEach(function(btn) {
  btn.addEventListener('click', function() {
    document.querySelectorAll('.tab').forEach(function(b){ b.classList.remove('active'); });
    document.querySelectorAll('.panel').forEach(function(p){ p.classList.remove('active'); });
    btn.classList.add('active');
    document.getElementById('tab-' + btn.dataset.tab).classList.add('active');
  });
});

// ── Load timezones dropdown ──────────────────────────────────────────────────
function loadTimezones(selected) {
  fetch('/api/timezones').then(function(r){ return r.json(); }).then(function(data) {
    var sel = document.getElementById('cfg-timezone');
    sel.innerHTML = '';
    data.forEach(function(tz) {
      var opt = document.createElement('option');
      opt.value = tz;
      opt.textContent = tz;
      if (tz === selected) opt.selected = true;
      sel.appendChild(opt);
    });
  });
}

// ── Load config ──────────────────────────────────────────────────────────────
function loadConfig() {
  fetch('/api/config').then(function(r){ return r.json(); }).then(function(c) {
    loadTimezones(c.timezone);
    setVal('cfg-language', c.language);
    setVal('cfg-ntp-server', c.ntp_server);
    setVal('cfg-date-interval', Math.round((c.date_interval_ms || 30000) / 1000));

    var dateEn = c.date_enabled !== false;
    setChecked('cfg-date-en', dateEn);
    document.getElementById('date-options').style.opacity = dateEn ? '1' : '0.4';
    document.getElementById('date-options').style.pointerEvents = dateEn ? '' : 'none';

    var bri = c.brightness !== undefined ? c.brightness : 4;
    setVal('cfg-brightness', bri);
    setText('brightness-val', bri);
    setText('brightness-val-badge', bri);

    var spd = c.scroll_speed_ms !== undefined ? c.scroll_speed_ms : 50;
    setVal('cfg-scroll-speed', spd);
    setText('speed-val', spd);
    setText('speed-val-badge', spd);

    setChecked('cfg-weather-en', c.weather_enabled);
    setVal('cfg-weather-lat', c.weather_lat !== undefined ? c.weather_lat : '');
    setVal('cfg-weather-lon', c.weather_lon !== undefined ? c.weather_lon : '');
    setVal('cfg-weather-update', Math.round((c.weather_update_ms || 600000) / 60000));
    setVal('cfg-weather-display', Math.round((c.weather_display_ms || 30000) / 1000));
    setVal('cfg-temp-unit', c.temp_unit || 'C');

    setChecked('cfg-quotes-en', c.quotes_enabled);
    setVal('cfg-quotes-tickers', c.quotes_tickers || '');
    setVal('cfg-quotes-update', Math.round((c.quotes_update_ms || 600000) / 60000));
    setVal('cfg-quotes-display', Math.round((c.quotes_display_ms || 30000) / 1000));

    setChecked('cfg-api-auth', c.api_auth_enabled);
  });
}

// ── Poll status ──────────────────────────────────────────────────────────────
function pollStatus() {
  fetch('/api/status').then(function(r){ return r.json(); }).then(function(s) {
    var preview = document.getElementById('live-preview');
    if (s.time_str) preview.textContent = s.time_str;
    setText('st-ntp', s.ntp_synced ? 'Sincronizado' : 'Aguardando');
    setText('st-slot', 'slot ' + s.active_slot);
    setText('st-ssid', s.ssid || '--');
    setText('ip-badge', s.ip || '--');
    setText('net-ssid', s.ssid || '--');
    setText('net-ip', s.ip || '--');
    setText('net-status', s.ntp_synced ? 'Conectado' : 'Sem sync');
  }).catch(function(){});
}

// ── Save clock config ────────────────────────────────────────────────────────
document.getElementById('btn-save-clock').addEventListener('click', function() {
  var body = {
    timezone: val('cfg-timezone'),
    language: val('cfg-language'),
    ntp_server: val('cfg-ntp-server'),
    date_interval_ms: parseInt(val('cfg-date-interval'), 10) * 1000,
    date_enabled: checked('cfg-date-en'),
    brightness: parseInt(val('cfg-brightness'), 10),
    scroll_speed_ms: parseInt(val('cfg-scroll-speed'), 10)
  };
  fetch('/api/config', {method:'POST', headers:{'Content-Type':'application/json'},
    body: JSON.stringify(body)}).then(function(r){ return r.json(); }).then(function(r) {
    showToast(r.ok ? 'Salvo!' : ('Erro: ' + (r.error || '?')));
  }).catch(function(){ showToast('Erro de rede'); });
});

// ── Send alert ───────────────────────────────────────────────────────────────
document.getElementById('btn-send-alert').addEventListener('click', function() {
  var msg = val('alert-msg');
  if (!msg) return;
  var mode = parseInt(val('alert-mode'), 10);
  var dur  = parseInt(val('alert-duration'), 10) * 1000;
  var body = { message: msg, mode: mode, duration_ms: dur };
  fetch('/api/alert', {method:'POST', headers:{'Content-Type':'application/json'},
    body: JSON.stringify(body)}).then(function(r){ return r.json(); }).then(function(r) {
    showToast(r.ok ? 'Alerta enviado!' : ('Erro: ' + (r.error || '?')));
    if (r.ok) setVal('alert-msg', '');
  }).catch(function(){ showToast('Erro de rede'); });
});

// ── Save weather config ──────────────────────────────────────────────────────
document.getElementById('btn-save-weather').addEventListener('click', function() {
  var body = {
    weather_enabled: checked('cfg-weather-en'),
    weather_lat: parseFloat(val('cfg-weather-lat')) || 0,
    weather_lon: parseFloat(val('cfg-weather-lon')) || 0,
    weather_update_ms: parseInt(val('cfg-weather-update'), 10) * 60000,
    weather_display_ms: parseInt(val('cfg-weather-display'), 10) * 1000,
    temp_unit: val('cfg-temp-unit')
  };
  fetch('/api/config', {method:'POST', headers:{'Content-Type':'application/json'},
    body: JSON.stringify(body)}).then(function(r){ return r.json(); }).then(function(r) {
    showToast(r.ok ? 'Salvo!' : ('Erro: ' + (r.error || '?')));
  }).catch(function(){ showToast('Erro de rede'); });
});

// ── Save quotes config ───────────────────────────────────────────────────────
document.getElementById('btn-save-quotes').addEventListener('click', function() {
  var body = {
    quotes_enabled: checked('cfg-quotes-en'),
    quotes_tickers: val('cfg-quotes-tickers'),
    quotes_update_ms: parseInt(val('cfg-quotes-update'), 10) * 60000,
    quotes_display_ms: parseInt(val('cfg-quotes-display'), 10) * 1000
  };
  fetch('/api/config', {method:'POST', headers:{'Content-Type':'application/json'},
    body: JSON.stringify(body)}).then(function(r){ return r.json(); }).then(function(r) {
    showToast(r.ok ? 'Salvo!' : ('Erro: ' + (r.error || '?')));
  }).catch(function(){ showToast('Erro de rede'); });
});

// ── Save WiFi ────────────────────────────────────────────────────────────────
document.getElementById('btn-save-wifi').addEventListener('click', function() {
  var ssid = val('wifi-ssid');
  var pass = val('wifi-pass');
  if (!ssid) { showToast('SSID n\u00e3o pode ser vazio'); return; }
  if (!confirm('Isso vai reiniciar o dispositivo. Continuar?')) return;
  fetch('/api/wifi', {method:'POST', headers:{'Content-Type':'application/json'},
    body: JSON.stringify({ssid: ssid, password: pass})}).then(function(r){ return r.json(); }).then(function(r) {
    showToast(r.ok ? 'Salvo! Reiniciando...' : ('Erro: ' + (r.error || '?')), 4000);
  }).catch(function(){ showToast('Erro de rede'); });
});

// ── Regenerate API key ───────────────────────────────────────────────────────
document.getElementById('btn-regen-key').addEventListener('click', function() {
  if (!confirm('Gerar nova chave de API? A chave atual ser\u00e1 invalidada.')) return;
  fetch('/api/config', {method:'POST', headers:{'Content-Type':'application/json'},
    body: JSON.stringify({regen_api_key: true})}).then(function(r){ return r.json(); }).then(function(r) {
    if (r.ok && r.api_key) {
      document.getElementById('api-key-box').textContent = r.api_key;
      showToast('Nova chave gerada!');
    } else {
      showToast('Erro: ' + (r.error || '?'));
    }
  }).catch(function(){ showToast('Erro de rede'); });
});

// ── Save API auth config ─────────────────────────────────────────────────────
document.getElementById('btn-save-api').addEventListener('click', function() {
  var body = { api_auth_enabled: checked('cfg-api-auth') };
  fetch('/api/config', {method:'POST', headers:{'Content-Type':'application/json'},
    body: JSON.stringify(body)}).then(function(r){ return r.json(); }).then(function(r) {
    showToast(r.ok ? 'Salvo!' : ('Erro: ' + (r.error || '?')));
  }).catch(function(){ showToast('Erro de rede'); });
});

// ── Init ─────────────────────────────────────────────────────────────────────
loadConfig();
pollStatus();
setInterval(pollStatus, 1000);
</script>
</body>
</html>
)=====";
