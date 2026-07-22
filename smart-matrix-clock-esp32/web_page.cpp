/*
 * Smart Matrix Clock
 * Copyright (c) 2026 Jan Souza
 *
 * Licensed under the MIT License. See the LICENSE file
 * in the project root for full license information.
 */

#include "web_page.h"

// Raw-string literal R"=====(...)=====" so we can embed quotes, newlines, and
// all HTML/CSS/JS without any escaping.  The page is fully self-contained
// (no external resources) and communicates with the ESP32 via fetch().

const char WEB_PAGE_HTML[] = R"=====(
<!DOCTYPE html>
<html lang="en">
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
  border:1px solid #30363d;overflow-wrap:anywhere}
.preview-overflow-text{color:#6e7681}
.icon-row{display:flex;flex-wrap:wrap;gap:6px}
.icon-btn{background:#161b22;border:1px solid #30363d;color:#c9d1d9;
  border-radius:4px;padding:6px 10px;font-size:16px;cursor:pointer;
  min-width:36px;line-height:1}
.icon-btn:hover{background:#21262d;border-color:#58a6ff}
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
.lang-select{margin-left:auto;padding:4px 8px;border:1px solid #30363d;
  border-radius:4px;font-size:13px;background:#0d1117;color:#c9d1d9;
  cursor:pointer}
</style>
</head>
<body>
<header>
  <h1>&#9719; Smart Matrix Clock</h1>
  <span id="ip-badge">...</span>
  <select class="lang-select" id="ui-lang-select"></select>
</header>
<div class="container">
  <div class="status-bar">
    <span><span data-i18n="status.ntp">NTP</span>: <strong id="st-ntp">--</strong></span>
    <span><span data-i18n="status.slot">Slot</span>: <strong id="st-slot">--</strong></span>
    <span><span data-i18n="status.wifi">WiFi</span>: <strong id="st-ssid">--</strong></span>
  </div>

  <div class="tabs">
    <button class="tab active" data-tab="clock">&#128336; <span data-i18n="tab.clock">Clock</span></button>
    <button class="tab" data-tab="alert">&#128276; <span data-i18n="tab.alert">Alert</span></button>
    <button class="tab" data-tab="weather">&#127780; <span data-i18n="tab.weather">Weather</span></button>
    <button class="tab" data-tab="quotes">&#128200; <span data-i18n="tab.quotes">Quotes</span></button>
    <button class="tab" data-tab="network">&#128246; <span data-i18n="tab.network">Network</span></button>
  </div>

  <!-- ── CLOCK TAB ────────────────────────────────────────────────────── -->
  <div class="panel active" id="tab-clock">
    <div class="card">
      <h2 data-i18n="clock.preview">Live preview</h2>
      <div class="preview" id="live-preview">--:--</div>
    </div>

    <div class="card">
      <h2 data-i18n="clock.timeAndTz">Time and Timezone</h2>
      <div class="field">
        <label data-i18n="clock.timezone">Timezone (IANA)</label>
        <select id="cfg-timezone"></select>
      </div>
      <div class="row">
        <div class="field">
          <label data-i18n="clock.dateLanguage">Date language</label>
          <select id="cfg-language">
            <option value="pt">Portugu&#234;s</option>
            <option value="en">English</option>
          </select>
        </div>
        <div class="field">
          <label data-i18n="clock.ntpServer">NTP server</label>
          <input id="cfg-ntp-server" type="text" maxlength="63"/>
        </div>
      </div>
    </div>

    <div class="card">
      <h2 data-i18n="clock.date">Date</h2>
      <div class="toggle-row" style="margin-bottom:10px">
        <span style="font-size:13px;font-weight:500;color:#c9d1d9" data-i18n="clock.showDatePeriodically">Show date periodically</span>
        <label class="toggle"><input type="checkbox" id="cfg-date-en"/><span class="slider-toggle"></span></label>
      </div>
      <div id="date-options">
        <div class="field">
          <label data-i18n="clock.dateInterval">Display interval (seconds)</label>
          <input id="cfg-date-interval" type="number" min="5" max="300"/>
        </div>
      </div>
    </div>

    <div class="card">
      <h2 data-i18n="clock.display">Display</h2>
      <div class="range-field">
        <label><span data-i18n="clock.brightness">Brightness</span>: <span id="brightness-val">4</span> / 15</label>
        <div class="range-row">
          <span style="font-size:12px;color:#8b949e">0</span>
          <input id="cfg-brightness" type="range" min="0" max="15" value="4"/>
          <span style="font-size:12px;color:#8b949e">15</span>
          <span class="range-val" id="brightness-val-badge">4</span>
        </div>
      </div>
      <div class="range-field">
        <label><span data-i18n="clock.scrollSpeed">Scroll speed</span>: <span id="speed-val">50</span> ms/frame</label>
        <div class="range-row">
          <span style="font-size:12px;color:#8b949e" data-i18n="clock.fast">Fast</span>
          <input id="cfg-scroll-speed" type="range" min="10" max="200" value="50"/>
          <span style="font-size:12px;color:#8b949e" data-i18n="clock.slow">Slow</span>
          <span class="range-val" id="speed-val-badge">50</span>
        </div>
        <div class="hint" style="margin-top:6px" data-i18n="clock.scrollHint">10 ms = fastest &mdash; 200 ms = slowest</div>
      </div>
    </div>

    <div class="actions">
      <button class="btn btn-primary" id="btn-save-clock" data-i18n="common.saveSettings">Save settings</button>
    </div>
  </div>

  <!-- ── ALERT TAB ───────────────────────────────────────────────────── -->
  <div class="panel" id="tab-alert">
    <div class="card">
      <h2 data-i18n="alert.title">Alert Message</h2>
      <div class="field">
        <label data-i18n="alert.message">Message (max. 127 chars)</label>
        <input id="alert-msg" type="text" maxlength="127" data-i18n-ph="alert.placeholder" placeholder="Type a message..."/>
      </div>
      <div class="field">
        <label data-i18n="alert.icons">Icons</label>
        <div id="alert-icon-row" class="icon-row"></div>
      </div>
      <div class="field">
        <label data-i18n="alert.preview">Preview</label>
        <div class="preview" id="alert-preview">&nbsp;</div>
        <div class="hint" id="alert-preview-overflow"></div>
      </div>
      <div class="row3">
        <div class="field">
          <label data-i18n="alert.mode">Display mode</label>
          <select id="alert-mode">
            <option value="0" data-i18n="alert.modeScroll">&#8618; Scroll</option>
            <option value="1" data-i18n="alert.modeBlink">&#10022; Blink</option>
            <option value="2" data-i18n="alert.modeStatic">&#9632; Static</option>
            <option value="3" data-i18n="alert.modeBlinkScroll">&#10022;&#8618; Blink + Scroll</option>
          </select>
        </div>
        <div class="field">
          <label data-i18n="alert.duration">Duration (seconds)</label>
          <input id="alert-duration" type="number" min="1" max="60" value="5"/>
          <div class="hint" data-i18n="alert.durationHint">Time shown on the display</div>
        </div>
        <div class="field" style="display:flex;align-items:flex-end">
          <button class="btn btn-primary" id="btn-send-alert" style="width:100%" data-i18n="alert.send">Send</button>
        </div>
      </div>
    </div>
  </div>

  <!-- ── WEATHER TAB ──────────────────────────────────────────────────── -->
  <div class="panel" id="tab-weather">
    <div class="card">
      <h2 data-i18n="weather.slotTitle">Weather Slot</h2>
      <div class="toggle-row">
        <span data-i18n="weather.enable">Enable weather slot</span>
        <label class="toggle"><input type="checkbox" id="cfg-weather-en"/><span class="slider-toggle"></span></label>
      </div>
    </div>
    <div class="card">
      <h2 data-i18n="weather.location">Location</h2>
      <div class="row">
        <div class="field">
          <label data-i18n="weather.latitude">Latitude</label>
          <input id="cfg-weather-lat" type="number" step="0.0001" min="-90" max="90"/>
        </div>
        <div class="field">
          <label data-i18n="weather.longitude">Longitude</label>
          <input id="cfg-weather-lon" type="number" step="0.0001" min="-180" max="180"/>
        </div>
      </div>
    </div>
    <div class="card">
      <h2 data-i18n="common.settings">Settings</h2>
      <div class="row">
        <div class="field">
          <label data-i18n="common.updateInterval">Update interval (minutes)</label>
          <input id="cfg-weather-update" type="number" min="5" max="1440"/>
        </div>
        <div class="field">
          <label data-i18n="common.displayInterval">Display time (seconds)</label>
          <input id="cfg-weather-display" type="number" min="5" max="300"/>
        </div>
      </div>
      <div class="field">
        <label data-i18n="weather.tempUnit">Temperature unit</label>
        <select id="cfg-temp-unit">
          <option value="C">Celsius (&#176;C)</option>
          <option value="F">Fahrenheit (&#176;F)</option>
        </select>
      </div>
    </div>
    <div class="actions">
      <button class="btn btn-primary" id="btn-save-weather" data-i18n="common.saveSettings">Save settings</button>
    </div>
  </div>

  <!-- ── QUOTES TAB ───────────────────────────────────────────────────── -->
  <div class="panel" id="tab-quotes">
    <div class="card">
      <h2 data-i18n="quotes.slotTitle">Quotes Slot</h2>
      <div class="toggle-row">
        <span data-i18n="quotes.enable">Enable quotes slot</span>
        <label class="toggle"><input type="checkbox" id="cfg-quotes-en"/><span class="slider-toggle"></span></label>
      </div>
    </div>
    <div class="card">
      <h2 data-i18n="quotes.assets">Assets (tickers)</h2>
      <div class="field">
        <label data-i18n="quotes.tickersLabel">Comma-separated tickers</label>
        <input id="cfg-quotes-tickers" type="text" placeholder="PETR4.SA,AAPL,BTC-USD" maxlength="120"/>
        <div class="hint" data-i18n="quotes.tickersHint">E.g.: PETR4.SA, AAPL, BTC-USD &mdash; max. 8 tickers</div>
      </div>
    </div>
    <div class="card">
      <h2 data-i18n="common.settings">Settings</h2>
      <div class="row">
        <div class="field">
          <label data-i18n="common.updateInterval">Update interval (minutes)</label>
          <input id="cfg-quotes-update" type="number" min="5" max="1440"/>
        </div>
        <div class="field">
          <label data-i18n="common.displayInterval">Display time (seconds)</label>
          <input id="cfg-quotes-display" type="number" min="5" max="300"/>
        </div>
      </div>
    </div>
    <div class="actions">
      <button class="btn btn-primary" id="btn-save-quotes" data-i18n="common.saveSettings">Save settings</button>
    </div>
  </div>

  <!-- ── NETWORK TAB ──────────────────────────────────────────────────── -->
  <div class="panel" id="tab-network">
    <div class="card">
      <h2 data-i18n="network.currentConnection">Current connection</h2>
      <div id="net-info">
        <div class="info-row"><span class="info-label">SSID</span><span id="net-ssid">--</span></div>
        <div class="info-row"><span class="info-label">IP</span><span id="net-ip">--</span></div>
        <div class="info-row"><span class="info-label" data-i18n="network.status">Status</span><span id="net-status">--</span></div>
      </div>
    </div>
    <div class="card">
      <h2 data-i18n="network.newWifi">New WiFi network</h2>
      <div class="field">
        <label>SSID</label>
        <input id="wifi-ssid" type="text" maxlength="32" autocomplete="off"/>
      </div>
      <div class="field">
        <label data-i18n="network.password">Password</label>
        <input id="wifi-pass" type="password" maxlength="64" autocomplete="new-password"/>
      </div>
      <div class="actions">
        <button class="btn btn-primary" id="btn-save-wifi" data-i18n="network.saveAndRestart">Save and restart</button>
      </div>
    </div>
  </div>

</div><!-- /container -->

<div class="toast" id="toast"></div>

<script>
// ── I18N ─────────────────────────────────────────────────────────────────────
// One dictionary object per supported language, keyed by the same dot-path
// keys used in data-i18n attributes throughout the page. To add a new
// language: add one more key here (e.g. I18N.es = {...}), add a matching
// <option> in the #ui-lang-select population loop below (already automatic,
// see populateLangSelect()), and add the code to the firmware's
// isUiLanguageValid() table in persistence.cpp.
var I18N = {
  en: {
    'status.ntp': 'NTP', 'status.slot': 'Slot', 'status.wifi': 'WiFi',
    'tab.clock': 'Clock', 'tab.alert': 'Alert', 'tab.weather': 'Weather',
    'tab.quotes': 'Quotes', 'tab.network': 'Network',
    'clock.preview': 'Live preview',
    'clock.timeAndTz': 'Time and Timezone',
    'clock.timezone': 'Timezone (IANA)',
    'clock.dateLanguage': 'Date language',
    'clock.ntpServer': 'NTP server',
    'clock.date': 'Date',
    'clock.showDatePeriodically': 'Show date periodically',
    'clock.dateInterval': 'Display interval (seconds)',
    'clock.display': 'Display',
    'clock.brightness': 'Brightness',
    'clock.scrollSpeed': 'Scroll speed',
    'clock.fast': 'Fast', 'clock.slow': 'Slow',
    'clock.scrollHint': '10 ms = fastest &mdash; 200 ms = slowest',
    'alert.title': 'Alert Message',
    'alert.message': 'Message (max. 127 chars)',
    'alert.placeholder': 'Type a message...',
    'alert.mode': 'Display mode',
    'alert.modeScroll': '&#8618; Scroll',
    'alert.modeBlink': '&#10022; Blink',
    'alert.modeStatic': '&#9632; Static',
    'alert.modeBlinkScroll': '&#10022;&#8618; Blink + Scroll',
    'alert.icons': 'Icons',
    'alert.preview': 'Preview',
    'alert.multiScreen': 'Message is longer than one screen — it will scroll',
    'alert.duration': 'Duration (seconds)',
    'alert.durationHint': 'Time shown on the display',
    'alert.send': 'Send',
    'weather.slotTitle': 'Weather Slot',
    'weather.enable': 'Enable weather slot',
    'weather.location': 'Location',
    'weather.latitude': 'Latitude', 'weather.longitude': 'Longitude',
    'weather.tempUnit': 'Temperature unit',
    'quotes.slotTitle': 'Quotes Slot',
    'quotes.enable': 'Enable quotes slot',
    'quotes.assets': 'Assets (tickers)',
    'quotes.tickersLabel': 'Comma-separated tickers',
    'quotes.tickersHint': 'E.g.: PETR4.SA, AAPL, BTC-USD &mdash; max. 8 tickers',
    'network.currentConnection': 'Current connection',
    'network.status': 'Status',
    'network.newWifi': 'New WiFi network',
    'network.password': 'Password',
    'network.saveAndRestart': 'Save and restart',
    'common.settings': 'Settings',
    'common.updateInterval': 'Update interval (minutes)',
    'common.displayInterval': 'Display time (seconds)',
    'common.saveSettings': 'Save settings',
    'status.ntpSynced': 'Synced', 'status.ntpWaiting': 'Waiting',
    'status.connected': 'Connected', 'status.noSync': 'No sync',
    'toast.saved': 'Saved!', 'toast.error': 'Error: ',
    'toast.networkError': 'Network error',
    'toast.alertSent': 'Alert sent!',
    'toast.ssidEmpty': 'SSID cannot be empty',
    'toast.confirmRestart': 'This will restart the device. Continue?',
    'toast.savedRestarting': 'Saved! Restarting...'
  },
  pt: {
    'status.ntp': 'NTP', 'status.slot': 'Slot', 'status.wifi': 'WiFi',
    'tab.clock': 'Rel&#243;gio', 'tab.alert': 'Alerta', 'tab.weather': 'Clima',
    'tab.quotes': 'Cota&#231;&#245;es', 'tab.network': 'Rede',
    'clock.preview': 'Preview ao vivo',
    'clock.timeAndTz': 'Hora e Fuso',
    'clock.timezone': 'Fuso hor&#225;rio (IANA)',
    'clock.dateLanguage': 'Idioma da data',
    'clock.ntpServer': 'Servidor NTP',
    'clock.date': 'Data',
    'clock.showDatePeriodically': 'Exibir data periodicamente',
    'clock.dateInterval': 'Intervalo de exibi&#231;&#227;o (segundos)',
    'clock.display': 'Display',
    'clock.brightness': 'Brilho',
    'clock.scrollSpeed': 'Velocidade de scroll',
    'clock.fast': 'R&#225;pido', 'clock.slow': 'Lento',
    'clock.scrollHint': '10 ms = mais r&#225;pido &mdash; 200 ms = mais lento',
    'alert.title': 'Mensagem de Alerta',
    'alert.message': 'Mensagem (m&#225;x. 127 chars)',
    'alert.placeholder': 'Digite uma mensagem...',
    'alert.mode': 'Modo de exibi&#231;&#227;o',
    'alert.modeScroll': '&#8618; Scroll',
    'alert.modeBlink': '&#10022; Piscar',
    'alert.modeStatic': '&#9632; Est&#225;tico',
    'alert.modeBlinkScroll': '&#10022;&#8618; Piscar + Scroll',
    'alert.icons': '&#205;cones',
    'alert.preview': 'Pr&#233;-visualiza&#231;&#227;o',
    'alert.multiScreen': 'Mensagem maior que uma tela &#8212; ir&#225; rolar (scroll)',
    'alert.duration': 'Dura&#231;&#227;o (segundos)',
    'alert.durationHint': 'Tempo de exibi&#231;&#227;o no display',
    'alert.send': 'Enviar',
    'weather.slotTitle': 'Slot de Clima',
    'weather.enable': 'Habilitar slot de clima',
    'weather.location': 'Localiza&#231;&#227;o',
    'weather.latitude': 'Latitude', 'weather.longitude': 'Longitude',
    'weather.tempUnit': 'Unidade de temperatura',
    'quotes.slotTitle': 'Slot de Cota&#231;&#245;es',
    'quotes.enable': 'Habilitar slot de cota&#231;&#245;es',
    'quotes.assets': 'Ativos (tickers)',
    'quotes.tickersLabel': 'Tickers separados por v&#237;rgula',
    'quotes.tickersHint': 'Ex.: PETR4.SA, AAPL, BTC-USD &mdash; m&#225;x. 8 tickers',
    'network.currentConnection': 'Conex&#227;o atual',
    'network.status': 'Status',
    'network.newWifi': 'Nova rede WiFi',
    'network.password': 'Senha',
    'network.saveAndRestart': 'Salvar e reiniciar',
    'common.settings': 'Configura&#231;&#245;es',
    'common.updateInterval': 'Intervalo de atualiza&#231;&#227;o (minutos)',
    'common.displayInterval': 'Tempo de exibi&#231;&#227;o (segundos)',
    'common.saveSettings': 'Salvar configura&#231;&#245;es',
    'status.ntpSynced': 'Sincronizado', 'status.ntpWaiting': 'Aguardando',
    'status.connected': 'Conectado', 'status.noSync': 'Sem sync',
    'toast.saved': 'Salvo!', 'toast.error': 'Erro: ',
    'toast.networkError': 'Erro de rede',
    'toast.alertSent': 'Alerta enviado!',
    'toast.ssidEmpty': 'SSID não pode ser vazio',
    'toast.confirmRestart': 'Isso vai reiniciar o dispositivo. Continuar?',
    'toast.savedRestarting': 'Salvo! Reiniciando...'
  }
};

var currentLang = 'en';

// Look up a translation key in the active language, falling back to `en`
// and finally to the key itself so a missing/older string never renders blank.
function t(key) {
  var dict = I18N[currentLang] || I18N.en;
  return dict[key] !== undefined ? dict[key] : (I18N.en[key] !== undefined ? I18N.en[key] : key);
}

// Apply `lang` to every data-i18n / data-i18n-ph element on the page.
// Falls back to 'en' if `lang` isn't a known dictionary (e.g. a future
// firmware saved a language this page build doesn't recognise yet).
function applyLang(lang) {
  currentLang = I18N[lang] ? lang : 'en';
  document.documentElement.lang = currentLang;
  document.querySelectorAll('[data-i18n]').forEach(function(el) {
    el.innerHTML = t(el.dataset.i18n);
  });
  document.querySelectorAll('[data-i18n-ph]').forEach(function(el) {
    el.setAttribute('placeholder', t(el.dataset.i18nPh));
  });
  var sel = document.getElementById('ui-lang-select');
  if (sel) sel.value = currentLang;
  if (typeof updateAlertPreview === 'function') updateAlertPreview();
}

// Populate the header language selector from the dictionaries available,
// so adding a language to I18N is enough for it to show up in the UI.
var LANG_LABELS = { en: 'English', pt: 'Português' };
function populateLangSelect() {
  var sel = document.getElementById('ui-lang-select');
  Object.keys(I18N).forEach(function(code) {
    var opt = document.createElement('option');
    opt.value = code;
    opt.textContent = LANG_LABELS[code] || code;
    sel.appendChild(opt);
  });
  sel.addEventListener('change', function() {
    applyLang(this.value);
    fetch('/api/config', {method:'POST', headers:{'Content-Type':'application/json'},
      body: JSON.stringify({ui_language: this.value})}).catch(function(){});
  });
}

// ── Utilities ────────────────────────────────────────────────────────────────
function showToast(msg, ms) {
  var toastEl = document.getElementById('toast');
  toastEl.textContent = msg;
  toastEl.style.display = 'block';
  setTimeout(function(){ toastEl.style.display = 'none'; }, ms || 2500);
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
    applyLang(c.ui_language || 'en');
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
  });
}

// ── Poll status ──────────────────────────────────────────────────────────────
function pollStatus() {
  fetch('/api/status').then(function(r){ return r.json(); }).then(function(s) {
    var preview = document.getElementById('live-preview');
    if (s.time_str) preview.textContent = s.time_str;
    setText('st-ntp', s.ntp_synced ? t('status.ntpSynced') : t('status.ntpWaiting'));
    setText('st-slot', 'slot ' + s.active_slot);
    setText('st-ssid', s.ssid || '--');
    setText('ip-badge', s.ip || '--');
    setText('net-ssid', s.ssid || '--');
    setText('net-ip', s.ip || '--');
    setText('net-status', s.ntp_synced ? t('status.connected') : t('status.noSync'));
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
    showToast(r.ok ? t('toast.saved') : (t('toast.error') + (r.error || '?')));
  }).catch(function(){ showToast(t('toast.networkError')); });
});

// ── Alert icon tags ────────────────────────────────────────────────────────────
// Mirrors the firmware's icon table in text_encoding.cpp (expandIconTags()).
// glyph = Unicode look-alike shown on the button and in the preview;
// tag   = literal [name] inserted into the message text.
var ALERT_ICONS = [
  { tag: 'heart',       glyph: '♥' },
  { tag: 'diamond',     glyph: '♦' },
  { tag: 'club',        glyph: '♣' },
  { tag: 'spade',       glyph: '♠' },
  { tag: 'bullet',      glyph: '•' },
  { tag: 'smile',       glyph: '☺' },
  { tag: 'star',        glyph: '★' },
  { tag: 'arrow_right', glyph: '▶' },
  { tag: 'arrow_left',  glyph: '◀' },
  { tag: 'note',        glyph: '♪' }
];
var ALERT_ICON_BY_TAG = {};
ALERT_ICONS.forEach(function(i) { ALERT_ICON_BY_TAG[i.tag] = i.glyph; });

function populateAlertIcons() {
  var row = document.getElementById('alert-icon-row');
  ALERT_ICONS.forEach(function(icon) {
    var btn = document.createElement('button');
    btn.type = 'button';
    btn.className = 'icon-btn';
    btn.textContent = icon.glyph;
    btn.title = icon.tag;
    btn.addEventListener('click', function() {
      var input = document.getElementById('alert-msg');
      var tagText = '[' + icon.tag + ']';
      var start = input.selectionStart != null ? input.selectionStart : input.value.length;
      var end = input.selectionEnd != null ? input.selectionEnd : input.value.length;
      input.value = input.value.slice(0, start) + tagText + input.value.slice(end);
      var caret = start + tagText.length;
      input.focus();
      input.setSelectionRange(caret, caret);
      updateAlertPreview();
    });
    row.appendChild(btn);
  });
}

// Resolve [name] tags to their preview glyph (or leave the literal tag if
// unknown), same rule the firmware applies when resolving to CP437 bytes.
function resolveAlertPreview(msg) {
  return msg.replace(/\[([a-z_]+)\]/g, function(match, name) {
    return ALERT_ICON_BY_TAG[name] !== undefined ? ALERT_ICON_BY_TAG[name] : match;
  });
}

// Per-character pixel widths of the MD_MAX72XX system font (USE_NEW_FONT
// table, ASCII 32-126), plus the 1px inter-character gap the display
// hardware always inserts between glyphs. Mirrors _charsFittingOnScreen()
// in display.cpp so the preview matches what the physical display actually
// fits on one screen before it needs to scroll.
var DISPLAY_WIDTH_PX = 32; // 4 chained 8x8 modules
var FONT_CHAR_WIDTH = {
  32:2, 33:1, 34:3, 35:5, 36:5, 37:5, 38:5, 39:1, 40:3, 41:3, 42:5, 43:5,
  44:2, 45:4, 46:2, 47:5, 48:5, 49:3, 50:5, 51:5, 52:5, 53:5, 54:5, 55:5,
  56:5, 57:5, 58:2, 59:2, 60:3, 61:4, 62:3, 63:5, 64:5, 65:5, 66:5, 67:5,
  68:5, 69:5, 70:5, 71:5, 72:5, 73:3, 74:5, 75:5, 76:5, 77:5, 78:5, 79:5,
  80:5, 81:5, 82:5, 83:5, 84:5, 85:5, 86:5, 87:5, 88:5, 89:5, 90:5, 91:3,
  92:5, 93:3, 94:5, 95:4, 96:3, 97:4, 98:4, 99:4, 100:4, 101:4, 102:4,
  103:4, 104:4, 105:1, 106:3, 107:4, 108:1, 109:5, 110:4, 111:4, 112:4,
  113:4, 114:4, 115:4, 116:3, 117:4, 118:4, 119:5, 120:4, 121:4, 122:4,
  123:4, 124:1, 125:4, 126:4
};
// CP437 control-byte glyph widths the firmware maps [name] icon tags to
// (see ICON_TABLE in text_encoding.cpp) — these render as the low-ASCII
// symbol glyphs (heart, spades, notes, ...), not as their Unicode preview
// look-alike, so their real display width must be looked up separately.
var ICON_GLYPH_WIDTH = {
  heart: 5, diamond: 5, club: 5, spade: 5, bullet: 4, smile: 5,
  star: 5, arrow_right: 5, arrow_left: 5, note: 5
};

// Split msg into display tokens: each [name] icon tag becomes one token
// (rendered on the real display as a single glyph), every other character
// is its own token. Keeps the tag-vs-rendered-width mapping correct so the
// screen-fit calculation matches the firmware instead of measuring the
// Unicode look-alike glyph used only for the on-screen preview.
function tokenizeAlertMessage(msg) {
  var tokens = [];
  var re = /\[([a-z_]+)\]/g;
  var lastIndex = 0, m;
  while ((m = re.exec(msg)) !== null) {
    for (var i = lastIndex; i < m.index; i++) tokens.push({ text: msg[i], width: charPixelWidth(msg[i]) });
    var name = m[1];
    if (ALERT_ICON_BY_TAG[name] !== undefined) {
      tokens.push({ text: ALERT_ICON_BY_TAG[name], width: ICON_GLYPH_WIDTH[name] !== undefined ? ICON_GLYPH_WIDTH[name] : 5 });
    } else {
      for (var j = 0; j < m[0].length; j++) tokens.push({ text: m[0][j], width: charPixelWidth(m[0][j]) });
    }
    lastIndex = re.lastIndex;
  }
  for (var k = lastIndex; k < msg.length; k++) tokens.push({ text: msg[k], width: charPixelWidth(msg[k]) });
  return tokens;
}

function charPixelWidth(ch) {
  var code = ch.charCodeAt(0);
  return FONT_CHAR_WIDTH[code] !== undefined ? FONT_CHAR_WIDTH[code] : 5;
}

// How many leading tokens fit within one display width.
function tokensFittingOnScreen(tokens) {
  var used = 0, count = 0;
  for (var i = 0; i < tokens.length; i++) {
    var w = tokens[i].width + (count > 0 ? 1 : 0);
    if (used + w > DISPLAY_WIDTH_PX) break;
    used += w;
    count++;
  }
  return count;
}

function updateAlertPreview() {
  var msg = val('alert-msg');
  var preview = document.getElementById('alert-preview');
  var overflowEl = document.getElementById('alert-preview-overflow');

  if (!msg) {
    preview.textContent = '\xa0';
    if (overflowEl) overflowEl.textContent = '';
    return;
  }

  var tokens = tokenizeAlertMessage(msg);
  var fitCount = tokensFittingOnScreen(tokens);
  var fits = fitCount >= tokens.length;

  preview.innerHTML = '';
  var firstScreen = document.createElement('span');
  firstScreen.textContent = tokens.slice(0, fitCount).map(function(tk){ return tk.text; }).join('');
  preview.appendChild(firstScreen);
  if (!fits) {
    var rest = document.createElement('span');
    rest.className = 'preview-overflow-text';
    rest.textContent = tokens.slice(fitCount).map(function(tk){ return tk.text; }).join('');
    preview.appendChild(rest);
  }

  if (overflowEl) {
    overflowEl.textContent = fits ? '' : t('alert.multiScreen');
  }
}

populateAlertIcons();
document.getElementById('alert-msg').addEventListener('input', updateAlertPreview);

// ── Send alert ───────────────────────────────────────────────────────────────
document.getElementById('btn-send-alert').addEventListener('click', function() {
  var msg = val('alert-msg');
  if (!msg) return;
  var mode = parseInt(val('alert-mode'), 10);
  var dur  = parseInt(val('alert-duration'), 10) * 1000;
  var body = { message: msg, mode: mode, duration_ms: dur };
  fetch('/api/alert', {method:'POST', headers:{'Content-Type':'application/json'},
    body: JSON.stringify(body)}).then(function(r){ return r.json(); }).then(function(r) {
    showToast(r.ok ? t('toast.alertSent') : (t('toast.error') + (r.error || '?')));
  }).catch(function(){ showToast(t('toast.networkError')); });
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
    showToast(r.ok ? t('toast.saved') : (t('toast.error') + (r.error || '?')));
  }).catch(function(){ showToast(t('toast.networkError')); });
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
    showToast(r.ok ? t('toast.saved') : (t('toast.error') + (r.error || '?')));
  }).catch(function(){ showToast(t('toast.networkError')); });
});

// ── Save WiFi ────────────────────────────────────────────────────────────────
document.getElementById('btn-save-wifi').addEventListener('click', function() {
  var ssid = val('wifi-ssid');
  var pass = val('wifi-pass');
  if (!ssid) { showToast(t('toast.ssidEmpty')); return; }
  if (!confirm(t('toast.confirmRestart'))) return;
  fetch('/api/wifi', {method:'POST', headers:{'Content-Type':'application/json'},
    body: JSON.stringify({ssid: ssid, password: pass})}).then(function(r){ return r.json(); }).then(function(r) {
    showToast(r.ok ? t('toast.savedRestarting') : (t('toast.error') + (r.error || '?')), 4000);
  }).catch(function(){ showToast(t('toast.networkError')); });
});

// ── Init ─────────────────────────────────────────────────────────────────────
populateLangSelect();
loadConfig();
pollStatus();
setInterval(pollStatus, 1000);
</script>
</body>
</html>
)=====";
