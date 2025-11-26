#include "global.h"
#include "TaskMainserver.h"
#include <WebServer.h>
#include "TaskDHT20.h" 

bool led1_state = false;
bool led2_state = false;
bool isAPMode   = true;

WebServer server(80);

String ssid = "Hacker_Binh";
String password = "chianlo2352080";
String wifi_ssid = "";
String wifi_password = "";

unsigned long connect_start_ms = 0;
bool connecting = false;

bool ap_enabled = false;
bool sta_enabled = false;
IPAddress apIP(192, 168, 10, 1);

String mainPage() {
  float temperature = 0, humidity = 0;
  if (DHT20_Mutex && xSemaphoreTake(DHT20_Mutex, pdMS_TO_TICKS(5))) {
      temperature = dht20.temp;
      humidity    = dht20.humidity;
      xSemaphoreGive(DHT20_Mutex);
  }

  String ip = isAPMode ? WiFi.softAPIP().toString()
                       : WiFi.localIP().toString();
  String wifiName = isAPMode ? ssid : wifi_ssid;

  String led1Class = led1_state ? "btn on"  : "btn off";
  String led2Class = led2_state ? "btn on"  : "btn off";

  return R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>HackerBinh</title>
  <style>
    :root{
      --bg1:#111827; --bg2:#1f2937; --primary:#6366f1; --accent:#22d3ee;
      --ok:#22c55e; --bad:#ef4444; --text:#e5e7eb; --muted:#94a3b8;
      --card:#0b1020aa; --glass:rgba(255,255,255,.06);
      --radius:18px; --shadow:0 10px 30px rgba(0,0,0,.35);
    }
    @media (prefers-color-scheme: light){
      :root{ --bg1:#eef2ff; --bg2:#e0e7ff; --text:#0f172a; --muted:#334155; --card:#ffffffcc; --glass:rgba(0,0,0,.05); }
    }
    *{box-sizing:border-box}
    body{
      margin:0; min-height:100dvh; color:var(--text);
      font:15px/1.45 Poppins,system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;
      background:radial-gradient(1200px 800px at 20% 10%, #0ea5e9 0%, transparent 60%),
                 radial-gradient(1000px 700px at 80% 0%, #6366f1 0%, transparent 55%),
                 linear-gradient(140deg,var(--bg1),var(--bg2));
      overflow-x:hidden;
    }
    .wrap{ display:grid; place-items:center; padding:28px; }
    .card{
      width:min(92vw,460px); background:var(--card); backdrop-filter:blur(12px);
      border-radius:var(--radius); box-shadow:var(--shadow); border:1px solid var(--glass);
      overflow:hidden; animation:pop .35s ease-out;
    }
    @keyframes pop{ from{ transform:translateY(8px) scale(.98); opacity:0 } to{ transform:none; opacity:1 } }
    .header{ padding:20px 22px 6px; }
    .title{ display:flex; align-items:center; gap:10px; font-weight:700; font-size:20px; letter-spacing:.2px; }
    .pulse{ width:10px; height:10px; border-radius:50%; background:var(--ok); box-shadow:0 0 0 0 rgba(34,197,94,.6); animation:pulse 2s infinite; }
    @keyframes pulse{ to{ box-shadow:0 0 0 14px rgba(34,197,94,0)} }
    .meta{ display:grid; grid-template-columns:1fr 1fr; gap:10px; margin:12px 0 8px; }
    .chip{ display:flex; gap:8px; align-items:center; padding:10px 12px; border-radius:12px;
           background:linear-gradient(180deg,rgba(255,255,255,.06),rgba(255,255,255,.02));
           border:1px solid var(--glass); font-weight:600; color:var(--text); }
    .chip small{ color:var(--muted); font-weight:500; display:block; margin-top:2px }
    .grid{ padding:10px 18px 18px; display:grid; gap:14px }
    .sensor{ display:grid; grid-template-columns:auto 1fr auto; align-items:center; gap:12px;
             padding:14px; border-radius:14px; border:1px solid var(--glass);
             background:linear-gradient(180deg,rgba(255,255,255,.05),rgba(255,255,255,.01)); }
    .sensor .label{ font-size:13px; color:var(--muted) }
    .sensor .value{ font-size:28px; font-weight:800; letter-spacing:.3px }
    .sensor .unit{ font-size:14px; color:var(--muted); margin-left:4px }
    .leds{ display:flex; gap:12px }
    .btn{ flex:1; border:0; cursor:pointer; color:#fff; font-weight:700; letter-spacing:.2px;
          padding:14px 12px; border-radius:14px; transition:transform .15s, box-shadow .2s, opacity .2s;
          display:flex; align-items:center; justify-content:center; gap:10px; }
    .btn:active{ transform:translateY(1px) }
    .btn.on{  background:linear-gradient(180deg,#22c55e,#16a34a); box-shadow:0 8px 20px rgba(34,197,94,.35) }
    .btn.off{ background:linear-gradient(180deg,#ef4444,#dc2626); box-shadow:0 8px 20px rgba(239,68,68,.35) }
    .toolbar{ display:flex; gap:10px; padding:0 18px 18px }
    .ghost{ flex:1; border:1px dashed var(--glass); color:var(--text); background:transparent; border-radius:12px; padding:12px; cursor:pointer }
    .footer{ padding:10px 18px 18px; display:flex; justify-content:space-between; align-items:center; color:var(--muted); font-size:12px }
    .badge{ padding:6px 10px; border-radius:999px; background:linear-gradient(180deg,rgba(255,255,255,.06),rgba(255,255,255,.02)); border:1px solid var(--glass) }
    .toast{ position:fixed; left:50%; bottom:22px; transform:translateX(-50%) translateY(20px); opacity:0; transition:.25s; padding:10px 14px; border-radius:12px; background:#000c; color:#fff; box-shadow:0 6px 18px rgba(0,0,0,.35); font-weight:600 }
    .toast.show{ opacity:1; transform:translateX(-50%) }
    .i{ width:18px; height:18px; display:inline-block; vertical-align:middle } .i svg{ width:18px; height:18px }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card" role="region" aria-label="ESP32 Dashboard">
      <div class="header">
        <div class="title"><span class="pulse" aria-hidden="true"></span> ESP32 Dashboard</div>
        <div class="meta">
          <div class="chip">
            <span class="i" aria-hidden="true">
              <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M2 8h20"/><path d="M5 12h14"/><path d="M8 16h8"/>
              </svg>
            </span>
            <div>
              Wi-Fi<br><small id="wifiName">)rawliteral" + wifiName + R"rawliteral(</small>
            </div>
          </div>
          <div class="chip">
            <span class="i" aria-hidden="true">
              <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M3 11l9-8 9 8v8a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>
              </svg>
            </span>
            <div>
              IP<br><small id="ipAddr">)rawliteral" + ip + R"rawliteral(</small>
            </div>
          </div>
        </div>
      </div>

      <div class="grid">
        <!-- Temperature -->
        <div class="sensor" aria-live="polite">
          <div class="i" aria-hidden="true">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <path d="M10 14.5A4.5 4.5 0 1 0 19 14.5a4.5 4.5 0 0 0-9 0z" opacity=".25"/>
              <path d="M12 3v8.5a2.5 2.5 0 1 0 2 0V3"/>
            </svg>
          </div>
          <div class="label">Temperature</div>
          <div class="value"><span id="temp">)rawliteral" + String(temperature,1) + R"rawliteral(</span><span class="unit">°C</span></div>
        </div>

        <!-- Humidity -->
        <div class="sensor" aria-live="polite">
          <div class="i" aria-hidden="true">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <path d="M12 2s7 7.3 7 12a7 7 0 0 1-14 0c0-4.7 7-12 7-12z"/>
            </svg>
          </div>
          <div class="label">Humidity</div>
          <div class="value"><span id="hum">)rawliteral" + String(humidity,0) + R"rawliteral(</span><span class="unit">%</span></div>
        </div>

        <!-- LED controls -->
        <div class="leds" role="group" aria-label="LED controls">
          <button id="btn1" class=")rawliteral" + led1Class + R"rawliteral(" onclick="toggleLED(1)">
            <span class="i" aria-hidden="true">
              <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M12 2v6"/><path d="M7 22h10"/><circle cx="12" cy="14" r="6"/>
              </svg>
            </span>
            LED 1
          </button>
          <button id="btn2" class=")rawliteral" + led2Class + R"rawliteral(" onclick="toggleLED(2)">
            <span class="i" aria-hidden="true">
              <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M12 2v6"/><path d="M7 22h10"/><circle cx="12" cy="14" r="6"/>
              </svg>
            </span>
            LED 2
          </button>
        </div>
      </div>

      <div class="toolbar">
        <button class="ghost" onclick="goSettings()">⚙ Settings</button>
        <button class="ghost" onclick="refreshSensors()">↻ Refresh</button>
      </div>

      <div class="footer">
        <span class="badge">Auto refresh every 3 seconds</span>
        <span class="badge" id="status">● online</span>
      </div>
    </div>
  </div>

  <div id="toast" class="toast" role="status" aria-live="polite">Updated</div>

  <script>
    function toast(msg){
      const el = document.getElementById('toast');
      el.textContent = msg; el.classList.add('show');
      setTimeout(()=>el.classList.remove('show'), 1600);
    }
    function goSettings(){ window.location='/settings'; }

    function updateLEDs(json){
      const b1=document.getElementById('btn1');
      const b2=document.getElementById('btn2');
      b1.classList.toggle('on',  json.led1==='ON');
      b1.classList.toggle('off', json.led1!=='ON');
      b2.classList.toggle('on',  json.led2==='ON');
      b2.classList.toggle('off', json.led2!=='ON');
    }
    function toggleLED(id){
      fetch('/toggle?led='+id)
        .then(r=>r.json())
        .then(j=>{ updateLEDs(j); toast('LED '+id+': '+(id===1?j.led1:j.led2)); })
        .catch(()=>toast('Failed to toggle LED'));
    }

    function refreshSensors(){
      fetch('/sensors')
        .then(r=>r.json())
        .then(d=>{
          document.getElementById('temp').textContent = Number(d.temp).toFixed(1);
          document.getElementById('hum').textContent  = Math.round(Number(d.hum));
          toast('Sensor data updated');
        })
        .catch(()=>toast('Failed to load sensor data'));
    }

    // auto refresh every 3s
    setInterval(refreshSensors, 3000);
  </script>
</body>
</html>
)rawliteral";
}



String settingsPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>ESP32 – Wi-Fi Settings</title>
  <style>
    :root{
      --bg1:#111827; --bg2:#1f2937; --text:#e5e7eb; --muted:#94a3b8;
      --card:#0b1020aa; --glass:rgba(255,255,255,.06);
      --accent:#22d3ee; --primary:#6366f1; --ok:#22c55e; --bad:#ef4444;
      --radius:18px; --shadow:0 10px 30px rgba(0,0,0,.35);
    }
    @media (prefers-color-scheme: light){
      :root{ --bg1:#eef2ff; --bg2:#e0e7ff; --text:#0f172a; --muted:#334155; --card:#ffffffcc; --glass:rgba(0,0,0,.05); }
    }
    *{box-sizing:border-box}
    body{
      margin:0; min-height:100dvh; color:var(--text);
      font:15px/1.45 Poppins,system-ui,-apple-system,Segoe UI,Roboto,Arial,sans-serif;
      background:radial-gradient(1200px 800px at 20% 10%, #0ea5e9 0%, transparent 60%),
                 radial-gradient(1000px 700px at 80% 0%, #6366f1 0%, transparent 55%),
                 linear-gradient(140deg,var(--bg1),var(--bg2));
      display:grid; place-items:center; padding:28px;
    }
    .card{
      width:min(92vw,460px); background:var(--card); backdrop-filter:blur(12px);
      border-radius:var(--radius); box-shadow:var(--shadow); border:1px solid var(--glass);
      overflow:hidden; animation:pop .35s ease-out; padding:22px 20px;
    }
    @keyframes pop{ from{ transform:translateY(8px) scale(.98); opacity:0 } to{ transform:none; opacity:1 } }
    h2{ margin:0 0 10px; font-size:22px; font-weight:800; letter-spacing:.2px }
    .sub{ color:var(--muted); font-size:13px; margin-bottom:18px }

    .field{ margin-bottom:14px }
    .label{ display:flex; justify-content:space-between; align-items:center; margin-bottom:6px; font-size:13px; color:var(--muted); font-weight:600 }
    .input{
      width:100%; padding:12px 14px; border-radius:14px; border:1px solid var(--glass);
      background:rgba(255,255,255,.06); color:var(--text); outline:none; transition:.2s;
      font-size:15px;
    }
    .input:focus{ border-color:var(--accent); background:rgba(255,255,255,.12) }
    .pw-row{ display:flex; gap:10px; align-items:center }
    .pw-row .input{ flex:1 }
    .toggle{
      user-select:none; cursor:pointer; font-size:12px; color:var(--muted);
      border:1px dashed var(--glass); padding:8px 10px; border-radius:10px;
    }

    .btn{
      width:100%; padding:14px; margin-top:8px; border:0; border-radius:14px; cursor:pointer;
      color:#fff; font-weight:800; letter-spacing:.2px; transition:.15s;
      display:flex; align-items:center; justify-content:center; gap:8px;
    }
    .btn:active{ transform:translateY(1px) }
    .primary{ background:linear-gradient(180deg,var(--primary),#4f46e5); box-shadow:0 8px 20px rgba(99,102,241,.35) }
    .ghost{ background:transparent; color:var(--text); border:1px dashed var(--glass) }

    .msg{ margin-top:12px; font-size:14px; font-weight:700 }
    .ok{ color:var(--ok) } .err{ color:var(--bad) }

    .toast{ position:fixed; left:50%; bottom:22px; transform:translateX(-50%) translateY(20px);
            opacity:0; transition:.25s; padding:10px 14px; border-radius:12px; background:#000c;
            color:#fff; font-weight:700; box-shadow:0 6px 18px rgba(0,0,0,.35) }
    .toast.show{ opacity:1; transform:translateX(-50%) }

    .row{ display:flex; gap:10px; }
  </style>
</head>
<body>
  <div class="card">
    <h2>Wi-Fi Settings</h2>
    <div class="sub">Enter SSID and password to connect your ESP32 to Wi-Fi.</div>

    <form id="wifiForm">
      <div class="field">
        <div class="label"><span>SSID</span></div>
        <input class="input" id="ssid" name="ssid" placeholder="Wi-Fi name (SSID)" required>
      </div>

      <div class="field">
        <div class="label"><span>Password</span></div>
        <div class="pw-row">
          <input class="input" id="pass" name="password" type="password" placeholder="Password" required>
          <span class="toggle" id="showpw">Show</span>
        </div>
      </div>

      <div class="row">
        <button class="btn primary" type="submit">⟶ Connect</button>
        <button class="btn ghost" type="button" onclick="window.location='/'">← Back</button>
      </div>
    </form>

    <div id="msg" class="msg"></div>
  </div>

  <div id="toast" class="toast">Sending request…</div>

  <script>
    const form  = document.getElementById('wifiForm');
    const toast = document.getElementById('toast');
    const msgEl = document.getElementById('msg');
    const pw    = document.getElementById('pass');
    const show  = document.getElementById('showpw');

    function showToast(text){
      toast.textContent = text;
      toast.classList.add('show');
      setTimeout(()=>toast.classList.remove('show'), 1600);
    }

    show.addEventListener('click', ()=>{
      const isPw = pw.type === 'password';
      pw.type = isPw ? 'text' : 'password';
      show.textContent = isPw ? 'Hide' : 'Show';
    });

    form.addEventListener('submit', (e)=>{
      e.preventDefault();
      const ssid = document.getElementById('ssid').value.trim();
      const pass = pw.value;

      if(!ssid){
        msgEl.textContent='SSID must not be empty';
        msgEl.className='msg err';
        return;
      }

      showToast('Sending request…');
      msgEl.textContent = '';
      msgEl.className = 'msg';

      fetch('/connect?ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass))
        .then(r=>r.text())
        .then(text=>{
          const ok = /success|connected|ok/i.test(text);
          msgEl.textContent = text;
          msgEl.className = 'msg ' + (ok ? 'ok' : 'err');
          showToast(ok ? 'Request sent – please wait for ESP32 to connect' : 'Connection failed');
        })
        .catch(()=>{
          msgEl.textContent = 'Failed to send request!';
          msgEl.className = 'msg err';
          showToast('Network error');
        });
    });
  </script>
</body>
</html>
)rawliteral";
}



// ========== Handlers ==========
void handleRoot() { server.send(200, "text/html", mainPage()); }

void handleToggle() {
  int led = server.arg("led").toInt();

  if (led == 1) {
    led1_state = !led1_state;
    digitalWrite(LED1_PIN, led1_state ? HIGH : LOW); 
  } else if (led == 2) {
    led2_state = !led2_state;
    digitalWrite(LED2_PIN, led2_state ? HIGH : LOW); 
  }

  server.send(200, "application/json",
    "{\"led1\":\"" + String(led1_state ? "ON":"OFF") +
    "\",\"led2\":\"" + String(led2_state ? "ON":"OFF") + "\"}");
}

void handleSensors() {
  float t = 0, h = 0;
  if (xSemaphoreTake(DHT20_Mutex, 5)) {
    t = dht20.temp;
    h = dht20.humidity;
    xSemaphoreGive(DHT20_Mutex);
  }
  String json = "{\"temp\":"+String(t,1)+",\"hum\":"+String(h,0)+"}";
  server.send(200, "application/json", json);
}


void handleSettings() { server.send(200, "text/html", settingsPage()); }

void handleConnect() {
  wifi_ssid     = server.arg("ssid");
  wifi_password = server.arg("pass");
  server.send(200, "text/plain", "Connecting....");
  isAPMode   = false;
  connecting = true;
  connect_start_ms = millis();
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());
  Serial.print("Connecting to STA: ");
  Serial.println(wifi_ssid);
  sta_enabled = true;
}

// ========== WiFi ==========
void setupServer() {
  server.on("/",        HTTP_GET, handleRoot);
  server.on("/toggle",  HTTP_GET, handleToggle);
  server.on("/sensors", HTTP_GET, handleSensors);
  server.on("/settings",HTTP_GET, handleSettings);
  server.on("/connect", HTTP_GET, handleConnect);
  server.begin();
}

void startAP() {
  if (!ap_enabled) {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0));
    WiFi.softAP(ssid.c_str(), password.c_str());
    Serial.print("AP started. AP IP address: ");
    Serial.println(WiFi.softAPIP());
    ap_enabled = true;
  }
  isAPMode = true;
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("✅ WiFi Connected!");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print("📡 IP Address assigned: ");
      Serial.println(WiFi.localIP());
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("⚠️ WiFi disconnected. Trying to reconnect...");
      WiFi.reconnect();
      break;
    default: break;
  }
}

void TaskMainserver(void *pvParameters){
  pinMode(BOOT_PIN, INPUT_PULLUP);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);

  startAP();
  setupServer();

  while(1){
    server.handleClient();

    if (digitalRead(BOOT_PIN) == LOW) {
      vTaskDelay(pdMS_TO_TICKS(100));
      if (digitalRead(BOOT_PIN) == LOW) {
        if (!isAPMode) {
          startAP();
          setupServer();
        }
      }
    }

    if (connecting) {
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("STA IP address: ");
        Serial.println(WiFi.localIP());
        isAPMode = false;
        connecting = false;
      } else if (millis() - connect_start_ms > 10000) { 
        Serial.println("WiFi connect failed! Back to AP.");
        startAP();
        setupServer();
        connecting = false;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(20)); 
  }
}
