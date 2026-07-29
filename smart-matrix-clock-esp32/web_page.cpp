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
/* Weekday picker (slot scheduling) */
.day-picker{display:flex;gap:6px;flex-wrap:wrap}
.day-toggle{position:relative;width:34px;height:34px}
.day-toggle input{position:absolute;opacity:0;width:100%;height:100%;
  margin:0;cursor:pointer}
.day-toggle span{position:absolute;inset:0;display:flex;align-items:center;
  justify-content:center;border-radius:50%;background:#0d1117;
  border:1px solid #30363d;color:#8b949e;font-size:11px;font-weight:600;
  pointer-events:none}
.day-toggle input:checked + span{background:#238636;border-color:#238636;color:#fff}
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
    <span><strong id="st-rssi" style="font-family:monospace">--</strong></span>
  </div>

  <div class="tabs">
    <button class="tab active" data-tab="display">&#128421; <span data-i18n="tab.display">Display</span></button>
    <button class="tab" data-tab="clock">&#128336; <span data-i18n="tab.clock">Clock</span></button>
    <button class="tab" data-tab="message">&#128276; <span data-i18n="tab.message">Message</span></button>
    <button class="tab" data-tab="weather">&#127780; <span data-i18n="tab.weather">Weather</span></button>
    <button class="tab" data-tab="quotes">&#128200; <span data-i18n="tab.quotes">Quotes</span></button>
    <button class="tab" data-tab="network">&#128246; <span data-i18n="tab.network">Network</span></button>
  </div>

  <!-- ── DISPLAY TAB ──────────────────────────────────────────────────── -->
  <div class="panel active" id="tab-display">
    <div class="card">
      <h2 data-i18n="clock.preview">Live preview</h2>
      <div class="preview" id="live-preview">--:--</div>
      <div class="actions" style="justify-content:flex-start;margin-top:12px">
        <button class="btn btn-secondary" id="btn-preview-weather-2" data-i18n="clock.previewWeather">&#127780; Preview Weather</button>
        <button class="btn btn-secondary" id="btn-preview-quotes-2" data-i18n="clock.previewQuotes">&#128200; Preview Quotes</button>
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
      <div class="toggle-row" style="margin-top:14px;margin-bottom:4px">
        <span style="font-size:13px;font-weight:500;color:#c9d1d9" data-i18n="clock.nightBrightness">Night mode (auto dim)</span>
        <label class="toggle"><input type="checkbox" id="cfg-night-bri-en"/><span class="slider-toggle"></span></label>
      </div>
      <div id="night-bri-settings" style="display:none">
        <div class="range-field" style="margin-top:8px">
          <label><span data-i18n="clock.nightBrightnessLevel">Night brightness</span>: <span id="night-bri-val">0</span> / 15</label>
          <div class="range-row">
            <span style="font-size:12px;color:#8b949e">0</span>
            <input id="cfg-night-bri-level" type="range" min="0" max="15" value="0"/>
            <span style="font-size:12px;color:#8b949e">15</span>
            <span class="range-val" id="night-bri-val-badge">0</span>
          </div>
        </div>
        <div class="field" style="margin-top:8px">
          <label data-i18n="clock.nightStart">Night starts at</label>
          <input id="cfg-night-start" type="time" value="23:00"/>
        </div>
        <div class="field" style="margin-top:8px">
          <label data-i18n="clock.nightEnd">Night ends at</label>
          <input id="cfg-night-end" type="time" value="07:00"/>
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
      <div class="field" style="margin-top:14px">
        <label data-i18n="clock.locale">Locale (date, numbers, quotes)</label>
        <select id="cfg-locale">
          <option value="pt">Portugu&#234;s</option>
          <option value="en">English</option>
        </select>
      </div>
    </div>

    <div class="actions">
      <button class="btn btn-primary" id="btn-save-clock" data-i18n="common.saveSettings">Save settings</button>
    </div>
  </div>

  <!-- ── CLOCK TAB ────────────────────────────────────────────────────── -->
  <div class="panel" id="tab-clock">
    <div class="card">
      <h2 data-i18n="clock.timeAndTz">Time and Timezone</h2>
      <div class="field">
        <label data-i18n="clock.timezone">Timezone (IANA)</label>
        <select id="cfg-timezone"></select>
      </div>
      <div class="field">
        <label data-i18n="clock.ntpServer">NTP server</label>
        <input id="cfg-ntp-server" type="text" maxlength="63"/>
      </div>
    </div>

    <div class="card">
      <h2 data-i18n="clock.clockMode">Clock mode</h2>
      <div class="toggle-row" style="margin-bottom:10px">
        <span style="font-size:13px;font-weight:500;color:#c9d1d9" data-i18n="clock.showSeconds">Show seconds (HH:MM:SS)</span>
        <label class="toggle"><input type="checkbox" id="cfg-clock-mode"/><span class="slider-toggle"></span></label>
      </div>
      <div class="hint" data-i18n="clock.showSecondsHint">When enabled, the clock displays HH:MM:SS.</div>
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

    <div class="actions">
      <button class="btn btn-primary" id="btn-save-clock-tz" data-i18n="common.saveSettings">Save settings</button>
    </div>
  </div>

  <!-- ── MESSAGE TAB ──────────────────────────────────────────────────── -->
  <div class="panel" id="tab-message">
    <div class="card">
      <h2 data-i18n="message.title">Message</h2>
      <div class="field">
        <label data-i18n="message.text">Message (max. 127 chars)</label>
        <input id="message-text" type="text" maxlength="127" data-i18n-ph="message.placeholder" placeholder="Type a message..."/>
      </div>
      <div class="field">
        <label data-i18n="message.icons">Icons</label>
        <div id="message-icon-row" class="icon-row"></div>
      </div>
      <div class="field">
        <label data-i18n="message.preview">Preview</label>
        <div class="preview" id="message-preview">&nbsp;</div>
        <div class="hint" id="message-preview-overflow"></div>
      </div>
      <div class="row3">
        <div class="field">
          <label data-i18n="message.mode">Display mode</label>
          <select id="message-mode">
            <option value="0" data-i18n="message.modeScroll">&#8618; Scroll</option>
            <option value="1" data-i18n="message.modeBlink">&#10022; Blink</option>
            <option value="2" data-i18n="message.modeStatic">&#9632; Static</option>
            <option value="3" data-i18n="message.modeBlinkScroll">&#10022;&#8618; Blink + Scroll</option>
          </select>
        </div>
        <div class="field">
          <label data-i18n="message.duration">Duration (seconds)</label>
          <input id="message-duration" type="number" min="1" max="60" value="5"/>
          <div class="hint" data-i18n="message.durationHint">Time shown on the display</div>
        </div>
        <div class="field" style="display:flex;align-items:flex-end">
          <button class="btn btn-primary" id="btn-send-message" style="width:100%" data-i18n="message.send">Send</button>
        </div>
      </div>
      <div class="toggle-row">
        <span data-i18n="message.override">Override brightness/speed for this message</span>
        <label class="toggle"><input type="checkbox" id="message-override-en"/><span class="slider-toggle"></span></label>
      </div>
      <div id="message-override-fields" style="display:none">
        <div class="range-field">
          <label><span data-i18n="clock.brightness">Brightness</span>: <span id="message-brightness-val">15</span> / 15</label>
          <div class="range-row">
            <span style="font-size:12px;color:#8b949e">0</span>
            <input id="message-brightness" type="range" min="0" max="15" value="15"/>
            <span style="font-size:12px;color:#8b949e">15</span>
            <span class="range-val" id="message-brightness-val-badge">15</span>
          </div>
        </div>
        <div class="range-field">
          <label><span data-i18n="clock.scrollSpeed">Scroll speed</span>: <span id="message-speed-val">50</span> ms/frame</label>
          <div class="range-row">
            <span style="font-size:12px;color:#8b949e" data-i18n="clock.fast">Fast</span>
            <input id="message-scroll-speed" type="range" min="10" max="200" value="50"/>
            <span style="font-size:12px;color:#8b949e" data-i18n="clock.slow">Slow</span>
            <span class="range-val" id="message-speed-val-badge">50</span>
          </div>
        </div>
      </div>
    </div>
    <div class="card">
      <h2 data-i18n="message.historyTitle">Message History</h2>
      <div id="message-history-list" style="display:flex;flex-direction:column;gap:6px">
        <div style="color:#8b949e;font-size:13px" data-i18n="message.historyEmpty">No messages sent yet.</div>
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
      <div class="field">
        <label data-i18n="weather.citySearch">Search city or postal code</label>
        <div style="display:flex;gap:8px">
          <input id="cfg-city-search" type="text" style="flex:1" data-i18n-ph="weather.citySearchPlaceholder" placeholder="e.g. Sao Paulo, BR"/>
          <button class="btn btn-secondary" id="btn-city-search" data-i18n="weather.search">Search</button>
        </div>
        <div id="city-search-status" style="font-size:12px;margin-top:6px;color:#8b949e"></div>
        <div id="city-search-results" style="margin-top:6px;display:flex;flex-direction:column;gap:4px"></div>
      </div>
      <div class="row" style="margin-top:12px">
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
      <div class="row">
        <div class="field">
          <label data-i18n="common.scheduleStart">Show from</label>
          <input id="cfg-weather-sched-start" type="time"/>
        </div>
        <div class="field">
          <label data-i18n="common.scheduleEnd">Show until</label>
          <input id="cfg-weather-sched-end" type="time"/>
        </div>
      </div>
      <div class="hint" data-i18n="common.scheduleHint">Leave both empty to show at any time of day</div>
      <div class="field" style="margin-top:10px">
        <label data-i18n="common.scheduleDays">Days</label>
        <div class="day-picker" id="cfg-weather-days">
          <label class="day-toggle"><input type="checkbox" data-day="1" checked/><span data-i18n="common.day.mon">Mon</span></label>
          <label class="day-toggle"><input type="checkbox" data-day="2" checked/><span data-i18n="common.day.tue">Tue</span></label>
          <label class="day-toggle"><input type="checkbox" data-day="3" checked/><span data-i18n="common.day.wed">Wed</span></label>
          <label class="day-toggle"><input type="checkbox" data-day="4" checked/><span data-i18n="common.day.thu">Thu</span></label>
          <label class="day-toggle"><input type="checkbox" data-day="5" checked/><span data-i18n="common.day.fri">Fri</span></label>
          <label class="day-toggle"><input type="checkbox" data-day="6" checked/><span data-i18n="common.day.sat">Sat</span></label>
          <label class="day-toggle"><input type="checkbox" data-day="0" checked/><span data-i18n="common.day.sun">Sun</span></label>
        </div>
      </div>
    </div>
    <div class="actions">
      <button class="btn btn-secondary" id="btn-fetch-weather" data-i18n="weather.fetchNow">Refresh now</button>
      <button class="btn btn-secondary" id="btn-preview-weather" data-i18n="weather.previewOnDisplay">Preview on display</button>
      <button class="btn btn-primary" id="btn-save-weather" data-i18n="common.saveSettings">Save settings</button>
    </div>
    <div class="card" id="weather-cache-card" style="margin-top:14px">
      <h2 data-i18n="weather.cacheTitle">Last data</h2>
      <div id="weather-cache-info" style="font-size:13px;color:#8b949e" data-i18n="weather.noCache">No data yet</div>
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
      <div class="row">
        <div class="field">
          <label data-i18n="common.scheduleStart">Show from</label>
          <input id="cfg-quotes-sched-start" type="time"/>
        </div>
        <div class="field">
          <label data-i18n="common.scheduleEnd">Show until</label>
          <input id="cfg-quotes-sched-end" type="time"/>
        </div>
      </div>
      <div class="hint" data-i18n="common.scheduleHint">Leave both empty to show at any time of day</div>
      <div class="field" style="margin-top:10px">
        <label data-i18n="common.scheduleDays">Days</label>
        <div class="day-picker" id="cfg-quotes-days">
          <label class="day-toggle"><input type="checkbox" data-day="1" checked/><span data-i18n="common.day.mon">Mon</span></label>
          <label class="day-toggle"><input type="checkbox" data-day="2" checked/><span data-i18n="common.day.tue">Tue</span></label>
          <label class="day-toggle"><input type="checkbox" data-day="3" checked/><span data-i18n="common.day.wed">Wed</span></label>
          <label class="day-toggle"><input type="checkbox" data-day="4" checked/><span data-i18n="common.day.thu">Thu</span></label>
          <label class="day-toggle"><input type="checkbox" data-day="5" checked/><span data-i18n="common.day.fri">Fri</span></label>
          <label class="day-toggle"><input type="checkbox" data-day="6"/><span data-i18n="common.day.sat">Sat</span></label>
          <label class="day-toggle"><input type="checkbox" data-day="0"/><span data-i18n="common.day.sun">Sun</span></label>
        </div>
      </div>
    </div>
    <div class="actions">
      <button class="btn btn-secondary" id="btn-fetch-quotes" data-i18n="quotes.fetchNow">Refresh now</button>
      <button class="btn btn-secondary" id="btn-preview-quotes" data-i18n="quotes.previewOnDisplay">Preview on display</button>
      <button class="btn btn-primary" id="btn-save-quotes" data-i18n="common.saveSettings">Save settings</button>
    </div>
    <div class="card" id="quotes-cache-card" style="margin-top:14px">
      <h2 data-i18n="quotes.cacheTitle">Last data</h2>
      <div id="quotes-cache-info" style="font-size:13px;color:#8b949e" data-i18n="quotes.noCache">No data yet</div>
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
        <div class="info-row"><span class="info-label" data-i18n="network.rssi">Signal (RSSI)</span><span id="net-rssi">--</span></div>
      </div>
    </div>
    <div class="card">
      <h2 data-i18n="network.newWifi">New WiFi network</h2>
      <div class="field">
        <label>SSID</label>
        <div style="display:flex;gap:8px">
          <input id="wifi-ssid" type="text" maxlength="32" autocomplete="off" style="flex:1"/>
          <button class="btn btn-secondary" id="btn-wifi-scan" data-i18n="network.scanNetworks">Scan networks</button>
        </div>
      </div>
      <div id="wifi-scan-status" style="font-size:12px;color:#8b949e;margin-top:4px;display:none"></div>
      <div id="wifi-scan-results" style="margin-top:6px;display:flex;flex-direction:column;gap:4px"></div>
      <div class="field" style="margin-top:8px">
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
    'tab.display': 'Display', 'tab.clock': 'Clock', 'tab.message': 'Message', 'tab.weather': 'Weather',
    'tab.quotes': 'Quotes', 'tab.network': 'Network',
    'clock.preview': 'Live preview',
    'clock.timeAndTz': 'Time and Timezone',
    'clock.timezone': 'Timezone (IANA)',
    'clock.locale': 'Locale (date, numbers, quotes)',
    'clock.ntpServer': 'NTP server',
    'clock.date': 'Date',
    'clock.clockMode': 'Clock mode',
    'clock.showSeconds': 'Show seconds (HH:MM:SS)',
    'clock.showSecondsHint': 'When enabled, the clock displays HH:MM:SS and updates every second. The colon no longer blinks.',
    'clock.showDatePeriodically': 'Show date periodically',
    'clock.dateInterval': 'Display interval (seconds)',
    'clock.display': 'Display',
    'clock.brightness': 'Brightness',
    'clock.nightBrightness': 'Night mode (auto dim)',
    'clock.nightBrightnessLevel': 'Night brightness',
    'clock.nightStart': 'Night starts at',
    'clock.nightEnd': 'Night ends at',
    'clock.scrollSpeed': 'Scroll speed',
    'clock.fast': 'Fast', 'clock.slow': 'Slow',
    'clock.scrollHint': '10 ms = fastest &mdash; 200 ms = slowest',
    'clock.previewWeather': '&#127780; Preview Weather',
    'clock.previewQuotes': '&#128200; Preview Quotes',
    'message.title': 'Message',
    'message.text': 'Message (max. 127 chars)',
    'message.placeholder': 'Type a message...',
    'message.mode': 'Display mode',
    'message.modeScroll': '&#8618; Scroll',
    'message.modeBlink': '&#10022; Blink',
    'message.modeStatic': '&#9632; Static',
    'message.modeBlinkScroll': '&#10022;&#8618; Blink + Scroll',
    'message.icons': 'Icons',
    'message.preview': 'Preview',
    'message.multiScreen': 'Message is longer than one screen — it will scroll',
    'message.duration': 'Duration (seconds)',
    'message.durationHint': 'Time shown on the display',
    'message.override': 'Override brightness/speed for this message',
    'message.send': 'Send',
    'message.historyTitle': 'Message History',
    'message.historyEmpty': 'No messages sent yet.',
    'message.historyRefresh': 'Refresh',
    'weather.slotTitle': 'Weather Slot',
    'weather.enable': 'Enable weather slot',
    'weather.location': 'Location',
    'weather.latitude': 'Latitude', 'weather.longitude': 'Longitude',
    'weather.citySearch': 'Search city or postal code',
    'weather.citySearchPlaceholder': 'e.g. Sao Paulo, BR',
    'weather.search': 'Search',
    'weather.searching': 'Searching...',
    'weather.searchNoResults': 'No results found',
    'weather.searchError': 'Search failed — check your internet connection',
    'weather.searchEmpty': 'Type a city name or postal code',
    'weather.citySelected': 'Location set',
    'weather.tempUnit': 'Temperature unit',
    'weather.previewOnDisplay': 'Preview on display',
    'weather.fetchNow': 'Refresh now',
    'weather.cacheTitle': 'Last data',
    'weather.noCache': 'No data yet — enable slot and wait for first fetch',
    'weather.cacheStale': '* Stale — last fetch failed',
    'quotes.slotTitle': 'Quotes Slot',
    'quotes.enable': 'Enable quotes slot',
    'quotes.assets': 'Assets (tickers)',
    'quotes.tickersLabel': 'Comma-separated tickers',
    'quotes.tickersHint': 'E.g.: PETR4.SA, AAPL, BTC-USD &mdash; max. 8 tickers',
    'quotes.previewOnDisplay': 'Preview on display',
    'quotes.fetchNow': 'Refresh now',
    'quotes.cacheTitle': 'Last data',
    'quotes.noCache': 'No data yet — enable slot and wait for first fetch',
    'quotes.cacheStale': '* Stale — last fetch failed',
    'network.currentConnection': 'Current connection',
    'network.status': 'Status',
    'network.rssi': 'Signal (RSSI)',
    'network.rssiUnit': 'dBm',
    'network.rssiNoConn': 'Not connected',
    'network.newWifi': 'New WiFi network',
    'network.password': 'Password',
    'network.saveAndRestart': 'Save and restart',
    'network.scanNetworks': 'Scan networks',
    'network.scanning': 'Scanning...',
    'network.scanError': 'Scan failed',
    'network.noNetworks': 'No networks found',
    'network.signalStrength': 'Signal',
    'network.secured': '&#128274;',
    'network.open': '&#128275;',
    'common.settings': 'Settings',
    'common.updateInterval': 'Update interval (minutes)',
    'common.displayInterval': 'Display time (seconds)',
    'common.scheduleStart': 'Show from',
    'common.scheduleEnd': 'Show until',
    'common.scheduleHint': 'Leave both empty to show at any time of day',
    'common.scheduleDays': 'Days',
    'common.day.mon': 'Mon', 'common.day.tue': 'Tue', 'common.day.wed': 'Wed',
    'common.day.thu': 'Thu', 'common.day.fri': 'Fri', 'common.day.sat': 'Sat',
    'common.day.sun': 'Sun',
    'common.saveSettings': 'Save settings',
    'status.ntpSynced': 'Synced', 'status.ntpWaiting': 'Waiting',
    'status.connected': 'Connected', 'status.noSync': 'No sync',
    'toast.saved': 'Saved!', 'toast.error': 'Error: ',
    'toast.networkError': 'Network error',
    'toast.messageSent': 'Message sent!',
    'toast.previewSent': 'Sent to display!',
    'toast.fetchQueued': 'Refreshing…',
    'toast.ssidEmpty': 'SSID cannot be empty',
    'toast.confirmRestart': 'This will restart the device. Continue?',
    'toast.savedRestarting': 'Saved! Restarting...'
  },
  pt: {
    'status.ntp': 'NTP', 'status.slot': 'Slot', 'status.wifi': 'WiFi',
    'tab.display': 'Display', 'tab.clock': 'Rel&#243;gio', 'tab.message': 'Mensagem', 'tab.weather': 'Clima',
    'tab.quotes': 'Cota&#231;&#245;es', 'tab.network': 'Rede',
    'clock.preview': 'Preview ao vivo',
    'clock.timeAndTz': 'Hora e Fuso',
    'clock.timezone': 'Fuso hor&#225;rio (IANA)',
    'clock.locale': 'Idioma (data, n&#250;meros, cota&#231;&#245;es)',
    'clock.ntpServer': 'Servidor NTP',
    'clock.date': 'Data',
    'clock.clockMode': 'Modo do rel&#243;gio',
    'clock.showSeconds': 'Mostrar segundos (HH:MM:SS)',
    'clock.showSecondsHint': 'Quando ativado, o rel&#243;gio exibe HH:MM:SS.',
    'clock.showDatePeriodically': 'Exibir data periodicamente',
    'clock.dateInterval': 'Intervalo de exibi&#231;&#227;o (segundos)',
    'clock.display': 'Display',
    'clock.brightness': 'Brilho',
    'clock.nightBrightness': 'Modo noturno (auto brilho)',
    'clock.nightBrightnessLevel': 'Brilho noturno',
    'clock.nightStart': 'Anoitece \u00e0s',
    'clock.nightEnd': 'Amanhece \u00e0s',
    'clock.scrollSpeed': 'Velocidade de scroll',
    'clock.fast': 'R&#225;pido', 'clock.slow': 'Lento',
    'clock.scrollHint': '10 ms = mais r&#225;pido &mdash; 200 ms = mais lento',
    'clock.previewWeather': '&#127780; Ver Clima',
    'clock.previewQuotes': '&#128200; Ver Cota&#231;&#245;es',
    'message.title': 'Mensagem',
    'message.text': 'Mensagem (m&#225;x. 127 chars)',
    'message.placeholder': 'Digite uma mensagem...',
    'message.mode': 'Modo de exibi&#231;&#227;o',
    'message.modeScroll': '&#8618; Scroll',
    'message.modeBlink': '&#10022; Piscar',
    'message.modeStatic': '&#9632; Est&#225;tico',
    'message.modeBlinkScroll': '&#10022;&#8618; Piscar + Scroll',
    'message.icons': '&#205;cones',
    'message.preview': 'Pr&#233;-visualiza&#231;&#227;o',
    'message.multiScreen': 'Mensagem maior que uma tela &#8212; ir&#225; rolar (scroll)',
    'message.duration': 'Dura&#231;&#227;o (segundos)',
    'message.durationHint': 'Tempo de exibi&#231;&#227;o no display',
    'message.override': 'Sobrescrever brilho/velocidade para esta mensagem',
    'message.send': 'Enviar',
    'message.historyTitle': 'Hist&#243;rico de Mensagens',
    'message.historyEmpty': 'Nenhuma mensagem enviada ainda.',
    'message.historyRefresh': 'Atualizar',
    'weather.slotTitle': 'Slot de Clima',
    'weather.enable': 'Habilitar slot de clima',
    'weather.location': 'Localiza&#231;&#227;o',
    'weather.latitude': 'Latitude', 'weather.longitude': 'Longitude',
    'weather.citySearch': 'Buscar cidade ou CEP',
    'weather.citySearchPlaceholder': 'ex: Sao Paulo, BR',
    'weather.search': 'Buscar',
    'weather.searching': 'Buscando...',
    'weather.searchNoResults': 'Nenhum resultado encontrado',
    'weather.searchError': 'Falha na busca &#8212; verifique sua conex&#227;o com a internet',
    'weather.searchEmpty': 'Digite o nome de uma cidade ou CEP',
    'weather.citySelected': 'Localiza&#231;&#227;o definida',
    'weather.tempUnit': 'Unidade de temperatura',
    'weather.previewOnDisplay': 'Mostrar no display agora',
    'weather.fetchNow': 'Atualizar agora',
    'weather.cacheTitle': '&#218;ltimos dados',
    'weather.noCache': 'Sem dados ainda &#8212; habilite o slot e aguarde o primeiro fetch',
    'weather.cacheStale': '* Desatualizado &#8212; &#250;ltimo fetch falhou',
    'quotes.slotTitle': 'Slot de Cota&#231;&#245;es',
    'quotes.enable': 'Habilitar slot de cota&#231;&#245;es',
    'quotes.assets': 'Ativos (tickers)',
    'quotes.tickersLabel': 'Tickers separados por v&#237;rgula',
    'quotes.tickersHint': 'Ex.: PETR4.SA, AAPL, BTC-USD &mdash; m&#225;x. 8 tickers',
    'quotes.previewOnDisplay': 'Mostrar no display agora',
    'quotes.fetchNow': 'Atualizar agora',
    'quotes.cacheTitle': '&#218;ltimos dados',
    'quotes.noCache': 'Sem dados ainda &#8212; habilite o slot e aguarde o primeiro fetch',
    'quotes.cacheStale': '* Desatualizado &#8212; &#250;ltimo fetch falhou',
    'network.currentConnection': 'Conex&#227;o atual',
    'network.status': 'Status',
    'network.rssi': 'Sinal (RSSI)',
    'network.rssiUnit': 'dBm',
    'network.rssiNoConn': 'Sem conex&#227;o',
    'network.newWifi': 'Nova rede WiFi',
    'network.password': 'Senha',
    'network.saveAndRestart': 'Salvar e reiniciar',
    'network.scanNetworks': 'Varrer redes',
    'network.scanning': 'Varrendo...',
    'network.scanError': 'Falha na varredura',
    'network.noNetworks': 'Nenhuma rede encontrada',
    'network.signalStrength': 'Sinal',
    'network.secured': '&#128274;',
    'network.open': '&#128275;',
    'common.settings': 'Configura&#231;&#245;es',
    'common.updateInterval': 'Intervalo de atualiza&#231;&#227;o (minutos)',
    'common.displayInterval': 'Tempo de exibi&#231;&#227;o (segundos)',
    'common.scheduleStart': 'Exibir a partir de',
    'common.scheduleEnd': 'Exibir at&#233;',
    'common.scheduleHint': 'Deixe os dois campos vazios para exibir a qualquer hor&#225;rio',
    'common.scheduleDays': 'Dias',
    'common.day.mon': 'Seg', 'common.day.tue': 'Ter', 'common.day.wed': 'Qua',
    'common.day.thu': 'Qui', 'common.day.fri': 'Sex', 'common.day.sat': 'S&#225;b',
    'common.day.sun': 'Dom',
    'common.saveSettings': 'Salvar configura&#231;&#245;es',
    'status.ntpSynced': 'Sincronizado', 'status.ntpWaiting': 'Aguardando',
    'status.connected': 'Conectado', 'status.noSync': 'Sem sync',
    'toast.saved': 'Salvo!', 'toast.error': 'Erro: ',
    'toast.networkError': 'Erro de rede',
    'toast.messageSent': 'Mensagem enviada!',
    'toast.previewSent': 'Enviado para o display!',
    'toast.fetchQueued': 'Atualizando&#8230;',
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
  if (typeof updateMessagePreview === 'function') updateMessagePreview();
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

// Slot scheduling: <input type="time"> gives/takes "HH:MM"; the API stores minutes-of-day.
// Equal start/end means "no restriction" -> rendered as empty fields (see setScheduleFields).
function minToTimeStr(min) {
  var h = Math.floor(min / 60), m = min % 60;
  return (h < 10 ? '0' : '') + h + ':' + (m < 10 ? '0' : '') + m;
}
function timeStrToMin(str) {
  if (!str) return 0;
  var parts = str.split(':');
  return parseInt(parts[0], 10) * 60 + parseInt(parts[1], 10);
}
function setScheduleFields(startId, endId, startMin, endMin) {
  if (startMin === endMin) { setVal(startId, ''); setVal(endId, ''); return; }
  setVal(startId, minToTimeStr(startMin));
  setVal(endId, minToTimeStr(endMin));
}
// Weekday mask: bit N = day N enabled, N following struct tm's tm_wday (0=Sunday..6=Saturday).
function setDaysMask(containerId, mask) {
  document.querySelectorAll('#' + containerId + ' input').forEach(function(cb) {
    cb.checked = !!(mask & (1 << parseInt(cb.dataset.day, 10)));
  });
}
function getDaysMask(containerId) {
  var mask = 0;
  document.querySelectorAll('#' + containerId + ' input').forEach(function(cb) {
    if (cb.checked) mask |= (1 << parseInt(cb.dataset.day, 10));
  });
  return mask;
}

// ── Range sliders live update ────────────────────────────────────────────────
document.getElementById('cfg-brightness').addEventListener('input', function() {
  setText('brightness-val', this.value);
  setText('brightness-val-badge', this.value);
});
document.getElementById('cfg-night-bri-level').addEventListener('input', function() {
  setText('night-bri-val', this.value);
  setText('night-bri-val-badge', this.value);
});
document.getElementById('cfg-night-bri-en').addEventListener('change', function() {
  document.getElementById('night-bri-settings').style.display = this.checked ? '' : 'none';
});
document.getElementById('cfg-scroll-speed').addEventListener('input', function() {
  setText('speed-val', this.value);
  setText('speed-val-badge', this.value);
});
document.getElementById('message-brightness').addEventListener('input', function() {
  setText('message-brightness-val', this.value);
  setText('message-brightness-val-badge', this.value);
});
document.getElementById('message-scroll-speed').addEventListener('input', function() {
  setText('message-speed-val', this.value);
  setText('message-speed-val-badge', this.value);
});
document.getElementById('message-override-en').addEventListener('change', function() {
  document.getElementById('message-override-fields').style.display = this.checked ? '' : 'none';
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
    setVal('cfg-locale', c.locale);
    setVal('cfg-ntp-server', c.ntp_server);
    setVal('cfg-date-interval', Math.round((c.date_interval_ms || 30000) / 1000));

    setChecked('cfg-clock-mode', (c.clock_mode || 0) === 1);

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

    var nightEn = !!c.night_brightness_enabled;
    setChecked('cfg-night-bri-en', nightEn);
    document.getElementById('night-bri-settings').style.display = nightEn ? '' : 'none';
    var nightLvl = c.night_brightness_level !== undefined ? c.night_brightness_level : 0;
    setVal('cfg-night-bri-level', nightLvl);
    setText('night-bri-val', nightLvl);
    setText('night-bri-val-badge', nightLvl);
    setVal('cfg-night-start', minToTime(c.night_start_min !== undefined ? c.night_start_min : 1380));
    setVal('cfg-night-end',   minToTime(c.night_end_min   !== undefined ? c.night_end_min   : 420));

    setChecked('cfg-weather-en', c.weather_enabled);
    setVal('cfg-weather-lat', c.weather_lat !== undefined ? c.weather_lat : '');
    setVal('cfg-weather-lon', c.weather_lon !== undefined ? c.weather_lon : '');
    setVal('cfg-weather-update', Math.round((c.weather_update_ms || 600000) / 60000));
    setVal('cfg-weather-display', Math.round((c.weather_display_ms || 30000) / 1000));
    setVal('cfg-temp-unit', c.temp_unit || 'C');
    setScheduleFields('cfg-weather-sched-start', 'cfg-weather-sched-end',
      c.weather_sched_start || 0, c.weather_sched_end || 0);
    setDaysMask('cfg-weather-days', c.weather_sched_days !== undefined ? c.weather_sched_days : 0x7F);

    setChecked('cfg-quotes-en', c.quotes_enabled);
    setVal('cfg-quotes-tickers', c.quotes_tickers || '');
    setVal('cfg-quotes-update', Math.round((c.quotes_update_ms || 600000) / 60000));
    setVal('cfg-quotes-display', Math.round((c.quotes_display_ms || 30000) / 1000));
    setScheduleFields('cfg-quotes-sched-start', 'cfg-quotes-sched-end',
      c.quotes_sched_start !== undefined ? c.quotes_sched_start : 480,
      c.quotes_sched_end !== undefined ? c.quotes_sched_end : 1080);
    setDaysMask('cfg-quotes-days', c.quotes_sched_days !== undefined ? c.quotes_sched_days : 0x3E);
  });
}

// ── Poll status ──────────────────────────────────────────────────────────────
function rssiToBar(rssi) {
  if (rssi >= -55) return '▂▄▆█';
  if (rssi >= -67) return '▂▄▆\u200B';
  if (rssi >= -78) return '▂▄\u200B\u200B';
  return '▂\u200B\u200B\u200B';
}
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

    // WiFi RSSI — status bar (icon only) + Network tab (icon + dBm)
    var rssiConnected = (s.wifi_rssi !== null && s.wifi_rssi !== undefined);
    setText('st-rssi', rssiConnected ? rssiToBar(s.wifi_rssi) : '--');
    var rssiEl = document.getElementById('net-rssi');
    if (rssiEl) rssiEl.textContent = rssiConnected
      ? rssiToBar(s.wifi_rssi) + ' ' + s.wifi_rssi + ' ' + t('network.rssiUnit')
      : t('network.rssiNoConn');

    // Weather cache info
    var cacheEl = document.getElementById('weather-cache-info');
    if (cacheEl) {
      if (s.weather_cache_valid) {
        var staleNote = s.weather_cache_stale ? ' <span style="color:#e3b341">(' + t('weather.cacheStale') + ')</span>' : '';
        cacheEl.innerHTML = s.weather_preview + staleNote;
      } else {
        cacheEl.textContent = t('weather.noCache');
      }
    }
    ['btn-preview-weather', 'btn-preview-weather-2'].forEach(function(id) {
      var btn = document.getElementById(id);
      if (!btn) return;
      btn.disabled = !s.weather_cache_valid;
      btn.style.opacity = s.weather_cache_valid ? '1' : '0.4';
    });

    // Quotes cache info
    var qCacheEl = document.getElementById('quotes-cache-info');
    if (qCacheEl) {
      if (s.quotes_cache_valid) {
        var qStaleNote = s.quotes_cache_stale ? ' <span style="color:#e3b341">(' + t('quotes.cacheStale') + ')</span>' : '';
        qCacheEl.innerHTML = s.quotes_preview + qStaleNote;
      } else {
        qCacheEl.textContent = t('quotes.noCache');
      }
    }
    ['btn-preview-quotes', 'btn-preview-quotes-2'].forEach(function(id) {
      var btn = document.getElementById(id);
      if (!btn) return;
      btn.disabled = !s.quotes_cache_valid;
      btn.style.opacity = s.quotes_cache_valid ? '1' : '0.4';
    });
    // Firmware version footer
    if (s.firmware_version) setText('fw-version', s.firmware_version);
  }).catch(function(){});
}

// ── Helpers: convert minutes-of-day ↔ "HH:MM" time string ───────────────────
function minToTime(minutes) {
  var h = Math.floor(minutes / 60);
  var m = minutes % 60;
  return (h < 10 ? '0' : '') + h + ':' + (m < 10 ? '0' : '') + m;
}
function timeToMin(timeStr) {
  var parts = timeStr.split(':');
  return parseInt(parts[0], 10) * 60 + parseInt(parts[1], 10);
}

// ── Save clock config (shared by the Display and Clock tabs) ──────────────────
function saveClockConfig() {
  var body = {
    timezone: val('cfg-timezone'),
    locale: val('cfg-locale'),
    ntp_server: val('cfg-ntp-server'),
    date_interval_ms: parseInt(val('cfg-date-interval'), 10) * 1000,
    date_enabled: checked('cfg-date-en'),
    clock_mode: checked('cfg-clock-mode') ? 1 : 0,
    brightness: parseInt(val('cfg-brightness'), 10),
    scroll_speed_ms: parseInt(val('cfg-scroll-speed'), 10),
    night_brightness_enabled: checked('cfg-night-bri-en'),
    night_brightness_level: parseInt(val('cfg-night-bri-level'), 10),
    night_start_min: timeToMin(val('cfg-night-start') || '23:00'),
    night_end_min:   timeToMin(val('cfg-night-end')   || '07:00')
  };
  fetch('/api/config', {method:'POST', headers:{'Content-Type':'application/json'},
    body: JSON.stringify(body)}).then(function(r){ return r.json(); }).then(function(r) {
    showToast(r.ok ? t('toast.saved') : (t('toast.error') + (r.error || '?')));
  }).catch(function(){ showToast(t('toast.networkError')); });
}
document.getElementById('btn-save-clock').addEventListener('click', saveClockConfig);
document.getElementById('btn-save-clock-tz').addEventListener('click', saveClockConfig);

// ── Message icon tags ────────────────────────────────────────────────────────────
// Mirrors the firmware's icon table in text_encoding.cpp (expandIconTags()).
// glyph = Unicode look-alike shown on the button and in the preview;
// tag   = literal [name] inserted into the message text.
var MESSAGE_ICONS = [
  { tag: 'heart',       glyph: '♥' },
  { tag: 'diamond',     glyph: '♦' },
  { tag: 'spade',       glyph: '♠' },
  { tag: 'bullet',      glyph: '•' },
  { tag: 'star',        glyph: '❄' },
  { tag: 'arrow_right', glyph: '▶' },
  { tag: 'arrow_left',  glyph: '◀' },
  { tag: 'up',          glyph: '↑' },
  { tag: 'down',        glyph: '↓' },
  { tag: 'bell',        glyph: '▲' },
  { tag: 'warn',        glyph: '▼' }
];
var MESSAGE_ICON_BY_TAG = {};
MESSAGE_ICONS.forEach(function(i) { MESSAGE_ICON_BY_TAG[i.tag] = i.glyph; });

function populateMessageIcons() {
  var row = document.getElementById('message-icon-row');
  MESSAGE_ICONS.forEach(function(icon) {
    var btn = document.createElement('button');
    btn.type = 'button';
    btn.className = 'icon-btn';
    btn.textContent = icon.glyph;
    btn.title = icon.tag;
    btn.addEventListener('click', function() {
      var input = document.getElementById('message-text');
      var tagText = '[' + icon.tag + ']';
      var start = input.selectionStart != null ? input.selectionStart : input.value.length;
      var end = input.selectionEnd != null ? input.selectionEnd : input.value.length;
      input.value = input.value.slice(0, start) + tagText + input.value.slice(end);
      var caret = start + tagText.length;
      input.focus();
      input.setSelectionRange(caret, caret);
      updateMessagePreview();
    });
    row.appendChild(btn);
  });
}

// Resolve [name] tags to their preview glyph (or leave the literal tag if
// unknown), same rule the firmware applies when resolving to CP437 bytes.
function resolveMessagePreview(msg) {
  return msg.replace(/\[([a-z_]+)\]/g, function(match, name) {
    return MESSAGE_ICON_BY_TAG[name] !== undefined ? MESSAGE_ICON_BY_TAG[name] : match;
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
// (see _iconTags in text_encoding.cpp) — these render as the low-ASCII
// symbol glyphs (heart, spade, arrows, ...), not as their Unicode preview
// look-alike, so their real display width (read from MD_MAX72xx_font.cpp's
// _sysfont[] table) must be looked up separately.
var ICON_GLYPH_WIDTH = {
  heart: 5, diamond: 5, spade: 5, bullet: 4,
  star: 5, arrow_right: 5, arrow_left: 5, up: 5, down: 5, bell: 5, warn: 5
};

// Split msg into display tokens: each [name] icon tag becomes one token
// (rendered on the real display as a single glyph), every other character
// is its own token. Keeps the tag-vs-rendered-width mapping correct so the
// screen-fit calculation matches the firmware instead of measuring the
// Unicode look-alike glyph used only for the on-screen preview.
function tokenizeMessageText(msg) {
  var tokens = [];
  var re = /\[([a-z_]+)\]/g;
  var lastIndex = 0, m;
  while ((m = re.exec(msg)) !== null) {
    for (var i = lastIndex; i < m.index; i++) tokens.push({ text: msg[i], width: charPixelWidth(msg[i]) });
    var name = m[1];
    if (MESSAGE_ICON_BY_TAG[name] !== undefined) {
      tokens.push({ text: MESSAGE_ICON_BY_TAG[name], width: ICON_GLYPH_WIDTH[name] !== undefined ? ICON_GLYPH_WIDTH[name] : 5 });
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

function updateMessagePreview() {
  var msg = val('message-text');
  var preview = document.getElementById('message-preview');
  var overflowEl = document.getElementById('message-preview-overflow');

  if (!msg) {
    preview.textContent = '\xa0';
    if (overflowEl) overflowEl.textContent = '';
    return;
  }

  var tokens = tokenizeMessageText(msg);
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
    overflowEl.textContent = fits ? '' : t('message.multiScreen');
  }
}

populateMessageIcons();
document.getElementById('message-text').addEventListener('input', updateMessagePreview);

// ── Clock-mode toggle: update live preview format immediately ──────────────
document.getElementById('cfg-clock-mode').addEventListener('change', function() {
  var preview = document.getElementById('live-preview');
  var cur = preview.textContent;
  // Only reformat if we have a real time value (not placeholder)
  if (!/^\d{2}:\d{2}/.test(cur)) return;
  if (this.checked) {
    // HH:MM → HH:MM:SS  (append :00 as placeholder until next poll)
    if (cur.length === 5) preview.textContent = cur + ':00';
  } else {
    // HH:MM:SS → HH:MM
    if (cur.length === 8) preview.textContent = cur.substring(0, 5);
  }
});

// ── Send message ───────────────────────────────────────────────────────────────
document.getElementById('btn-send-message').addEventListener('click', function() {
  var msg = val('message-text');
  if (!msg) return;
  var mode = parseInt(val('message-mode'), 10);
  var dur  = parseInt(val('message-duration'), 10) * 1000;
  var body = { message: msg, mode: mode, duration_ms: dur };
  if (document.getElementById('message-override-en').checked) {
    body.brightness = parseInt(val('message-brightness'), 10);
    body.scroll_speed_ms = parseInt(val('message-scroll-speed'), 10);
  }
  fetch('/api/message', {method:'POST', headers:{'Content-Type':'application/json'},
    body: JSON.stringify(body)}).then(function(r){ return r.json(); }).then(function(r) {
    showToast(r.ok ? t('toast.messageSent') : (t('toast.error') + (r.error || '?')));
    if (r.ok) loadMessageHistory();
  }).catch(function(){ showToast(t('toast.networkError')); });
});

// ── Message history ─────────────────────────────────────────────────────────────
function formatMessageTimestamp(ts) {
  if (!ts) return '';
  var d = new Date(ts * 1000);
  var pad = function(n) { return String(n).padStart(2, '0'); };
  return d.getFullYear() + '-' + pad(d.getMonth()+1) + '-' + pad(d.getDate()) +
         ' ' + pad(d.getHours()) + ':' + pad(d.getMinutes()) + ':' + pad(d.getSeconds());
}

function loadMessageHistory() {
  fetch('/api/messages/history').then(function(r){ return r.json(); }).then(function(entries) {
    var container = document.getElementById('message-history-list');
    if (!entries || entries.length === 0) {
      container.innerHTML = '<div style="color:#8b949e;font-size:13px" data-i18n="message.historyEmpty">' + t('message.historyEmpty') + '</div>';
      return;
    }
    container.innerHTML = '';
    // Show newest first
    for (var i = entries.length - 1; i >= 0; i--) {
      var e = entries[i];
      var row = document.createElement('div');
      row.style.cssText = 'display:flex;gap:8px;align-items:baseline;border-bottom:1px solid #21262d;padding-bottom:4px;font-size:13px';
      var ts = document.createElement('span');
      ts.style.cssText = 'color:#8b949e;white-space:nowrap;font-family:monospace;flex-shrink:0';
      ts.textContent = formatMessageTimestamp(e.timestamp);
      var msg = document.createElement('span');
      msg.style.cssText = 'color:#c9d1d9;word-break:break-word';
      msg.textContent = e.message;
      row.appendChild(ts);
      row.appendChild(msg);
      container.appendChild(row);
    }
  }).catch(function() {});
}

// Load history on tab switch to Message
document.querySelectorAll('.tab').forEach(function(btn) {
  btn.addEventListener('click', function() {
    if (btn.getAttribute('data-tab') === 'message') loadMessageHistory();
  });
});

// ── Search city/postal code via Open-Meteo geocoding and fill lat/lon ───────
function runCitySearch() {
  var statusEl   = document.getElementById('city-search-status');
  var resultsEl  = document.getElementById('city-search-results');
  var query      = val('cfg-city-search').trim();
  resultsEl.innerHTML = '';
  if (!query) {
    statusEl.textContent = t('weather.searchEmpty');
    return;
  }
  statusEl.textContent = t('weather.searching');
  var lang = (document.getElementById('cfg-locale') ? val('cfg-locale') : 'en') || 'en';
  var url = 'https://geocoding-api.open-meteo.com/v1/search?name=' +
            encodeURIComponent(query) + '&count=5&language=' + encodeURIComponent(lang) + '&format=json';
  fetch(url).then(function(r) { return r.json(); }).then(function(data) {
    var results = data.results || [];
    if (results.length === 0) {
      statusEl.textContent = t('weather.searchNoResults');
      return;
    }
    statusEl.textContent = '';
    results.forEach(function(place) {
      var btn = document.createElement('button');
      btn.className = 'btn btn-secondary';
      btn.style.textAlign = 'left';
      var parts = [place.name];
      if (place.admin1) parts.push(place.admin1);
      if (place.country) parts.push(place.country);
      btn.textContent = parts.join(', ') + ' (' + place.latitude.toFixed(2) + ', ' + place.longitude.toFixed(2) + ')';
      btn.addEventListener('click', function() {
        document.getElementById('cfg-weather-lat').value = place.latitude.toFixed(4);
        document.getElementById('cfg-weather-lon').value = place.longitude.toFixed(4);
        statusEl.textContent = t('weather.citySelected');
        resultsEl.innerHTML = '';
      });
      resultsEl.appendChild(btn);
    });
  }).catch(function() {
    statusEl.textContent = t('weather.searchError');
  });
}
document.getElementById('btn-city-search').addEventListener('click', runCitySearch);
document.getElementById('cfg-city-search').addEventListener('keydown', function(e) {
  if (e.key === 'Enter') { e.preventDefault(); runCitySearch(); }
});

// ── Save weather config ──────────────────────────────────────────────────────
document.getElementById('btn-save-weather').addEventListener('click', function() {
  var schedStart = val('cfg-weather-sched-start'), schedEnd = val('cfg-weather-sched-end');
  var body = {
    weather_enabled: checked('cfg-weather-en'),
    weather_lat: parseFloat(val('cfg-weather-lat')) || 0,
    weather_lon: parseFloat(val('cfg-weather-lon')) || 0,
    weather_update_ms: parseInt(val('cfg-weather-update'), 10) * 60000,
    weather_display_ms: parseInt(val('cfg-weather-display'), 10) * 1000,
    temp_unit: val('cfg-temp-unit'),
    weather_sched_start: (schedStart && schedEnd) ? timeStrToMin(schedStart) : 0,
    weather_sched_end:   (schedStart && schedEnd) ? timeStrToMin(schedEnd) : 0,
    weather_sched_days:  getDaysMask('cfg-weather-days')
  };
  fetch('/api/config', {method:'POST', headers:{'Content-Type':'application/json'},
    body: JSON.stringify(body)}).then(function(r){ return r.json(); }).then(function(r) {
    showToast(r.ok ? t('toast.saved') : (t('toast.error') + (r.error || '?')));
  }).catch(function(){ showToast(t('toast.networkError')); });
});

// ── Preview weather/quotes on display (buttons duplicated on Display tab) ─────
function previewSlot(slot) {
  fetch('/api/preview', {method:'POST', headers:{'Content-Type':'application/json'},
    body: JSON.stringify({slot: slot})}).then(function(r){ return r.json(); }).then(function(r) {
    showToast(r.ok ? t('toast.previewSent') : (t('toast.error') + (r.error || '?')));
  }).catch(function(){ showToast(t('toast.networkError')); });
}
document.getElementById('btn-preview-weather').addEventListener('click', function() { previewSlot(2); });
document.getElementById('btn-preview-quotes').addEventListener('click', function() { previewSlot(3); });
document.getElementById('btn-preview-weather-2').addEventListener('click', function() { previewSlot(2); });
document.getElementById('btn-preview-quotes-2').addEventListener('click', function() { previewSlot(3); });

// ── Force-refresh weather/quotes data (bypasses the update interval) ──────────
// Only queues the fetch — the device's next fetcherTick() (within a few
// seconds) does the actual HTTP call and updates the cache; poll status
// shortly after so the "Last data" card picks up the new value.
function fetchSlot(slot) {
  fetch('/api/fetch', {method:'POST', headers:{'Content-Type':'application/json'},
    body: JSON.stringify({slot: slot})}).then(function(r){ return r.json(); }).then(function(r) {
    showToast(r.ok ? t('toast.fetchQueued') : (t('toast.error') + (r.error || '?')));
    if (r.ok) setTimeout(pollStatus, 4000);
  }).catch(function(){ showToast(t('toast.networkError')); });
}
document.getElementById('btn-fetch-weather').addEventListener('click', function() { fetchSlot(2); });
document.getElementById('btn-fetch-quotes').addEventListener('click', function() { fetchSlot(3); });

// ── Save quotes config ───────────────────────────────────────────────────────
document.getElementById('btn-save-quotes').addEventListener('click', function() {
  var schedStart = val('cfg-quotes-sched-start'), schedEnd = val('cfg-quotes-sched-end');
  var body = {
    quotes_enabled: checked('cfg-quotes-en'),
    quotes_tickers: val('cfg-quotes-tickers'),
    quotes_update_ms: parseInt(val('cfg-quotes-update'), 10) * 60000,
    quotes_display_ms: parseInt(val('cfg-quotes-display'), 10) * 1000,
    quotes_sched_start: (schedStart && schedEnd) ? timeStrToMin(schedStart) : 0,
    quotes_sched_end:   (schedStart && schedEnd) ? timeStrToMin(schedEnd) : 0,
    quotes_sched_days:  getDaysMask('cfg-quotes-days')
  };
  fetch('/api/config', {method:'POST', headers:{'Content-Type':'application/json'},
    body: JSON.stringify(body)}).then(function(r){ return r.json(); }).then(function(r) {
    showToast(r.ok ? t('toast.saved') : (t('toast.error') + (r.error || '?')));
  }).catch(function(){ showToast(t('toast.networkError')); });
});

// ── WiFi network scan ────────────────────────────────────────────────────────
// Calls GET /api/wifi/scan (blocking ~2 s on the device), then renders a list
// of visible networks. Clicking a network row fills the SSID field and focuses
// the password input so the user can immediately type the password.
// rssiToBar() is defined in the pollStatus() section above and reused here.
function runWifiScan() {
  var statusEl  = document.getElementById('wifi-scan-status');
  var resultsEl = document.getElementById('wifi-scan-results');
  var btn       = document.getElementById('btn-wifi-scan');

  resultsEl.innerHTML = '';
  statusEl.textContent = t('network.scanning');
  statusEl.style.display = '';
  btn.disabled = true;

  fetch('/api/wifi/scan').then(function(r) { return r.json(); }).then(function(nets) {
    btn.disabled = false;
    if (!nets || nets.length === 0) {
      statusEl.textContent = t('network.noNetworks');
      return;
    }
    statusEl.style.display = 'none';
    // Sort strongest first
    nets.sort(function(a, b) { return b.rssi - a.rssi; });
    nets.forEach(function(net) {
      var row = document.createElement('button');
      row.className = 'btn btn-secondary';
      row.style.cssText = 'display:flex;align-items:center;justify-content:space-between;text-align:left;width:100%;padding:6px 10px;gap:8px';
      var nameSpan = document.createElement('span');
      nameSpan.style.cssText = 'flex:1;overflow:hidden;text-overflow:ellipsis;white-space:nowrap';
      nameSpan.textContent = (net.secure ? '\uD83D\uDD12 ' : '\uD83D\uDD13 ') + net.ssid;
      var sigSpan = document.createElement('span');
      sigSpan.style.cssText = 'font-family:monospace;font-size:12px;color:#8b949e;flex-shrink:0';
      sigSpan.textContent = rssiToBar(net.rssi) + ' ' + net.rssi + ' dBm';
      row.appendChild(nameSpan);
      row.appendChild(sigSpan);
      row.addEventListener('click', function() {
        document.getElementById('wifi-ssid').value = net.ssid;
        resultsEl.innerHTML = '';
        statusEl.style.display = 'none';
        document.getElementById('wifi-pass').focus();
      });
      resultsEl.appendChild(row);
    });
  }).catch(function() {
    btn.disabled = false;
    statusEl.textContent = t('network.scanError');
    statusEl.style.display = '';
  });
}
document.getElementById('btn-wifi-scan').addEventListener('click', runWifiScan);

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
<footer style="margin-top:24px;padding:12px 0;border-top:1px solid #30363d;text-align:center;font-size:11px;color:#8b949e">
  Smart Matrix Clock &mdash; firmware <span id="fw-version">--</span>
</footer>
</body>
</html>
)=====";
