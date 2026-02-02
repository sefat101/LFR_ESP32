#include "config.h"
#include "wifi_tuner.h"
#include <WiFi.h>
#include <WebServer.h>

static WebServer server(80);

static float clampf(float v, float lo, float hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void WifiTuner::begin(PIDGains *gains, LiveStatus *status,
                      volatile bool *reqCalib, volatile bool *reqStop, volatile bool *reqRun) {
  g_ = gains;
  st_ = status;
  reqCalib_ = reqCalib;
  reqStop_  = reqStop;
  reqRun_   = reqRun;

  WiFi.mode(WIFI_TRY_STA ? WIFI_AP_STA : WIFI_AP);
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);

  if (WIFI_TRY_STA) {
    WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASS);
  }

  setupRoutes_();
  server.begin();
}

void WifiTuner::loop() {
  server.handleClient();
}

void WifiTuner::setupRoutes_() {
  server.on("/", HTTP_GET, [this]() {
    server.send(200, "text/html", page_());
  });

  server.on("/api/status", HTTP_GET, [this]() {
    char buf[256];
    snprintf(buf, sizeof(buf),
      "{\"kp\":%.6f,\"ki\":%.6f,\"kd\":%.6f,"
      "\"pos\":%d,\"sum\":%d,\"lost\":%d,\"err\":%.2f,"
      "\"outL\":%d,\"outR\":%d}",
      g_->kp, g_->ki, g_->kd,
      st_->pos, st_->sum, st_->lost, st_->err,
      st_->outL, st_->outR
    );
    server.send(200, "application/json", buf);
  });

  // /api/nudge?p=kp|ki|kd&d=+1|-1
  server.on("/api/nudge", HTTP_GET, [this]() {
    if (!server.hasArg("p") || !server.hasArg("d")) {
      server.send(400, "text/plain", "missing args");
      return;
    }
    String p = server.arg("p");
    int dir = server.arg("d").toInt();

    if (p == "kp") g_->kp = clampf(g_->kp + dir * KP_STEP, KP_MIN, KP_MAX);
    if (p == "ki") g_->ki = clampf(g_->ki + dir * KI_STEP, KI_MIN, KI_MAX);
    if (p == "kd") g_->kd = clampf(g_->kd + dir * KD_STEP, KD_MIN, KD_MAX);

    server.send(200, "text/plain", "ok");
  });

  // /api/action?a=calib|stop|run
  server.on("/api/action", HTTP_GET, [this]() {
    if (!server.hasArg("a")) {
      server.send(400, "text/plain", "missing a");
      return;
    }
    String a = server.arg("a");
    if (a == "calib") *reqCalib_ = true;
    if (a == "stop")  *reqStop_  = true;
    if (a == "run")   *reqRun_   = true;
    server.send(200, "text/plain", "ok");
  });
}

String WifiTuner::page_() const {
  return R"HTML(
<!doctype html><html>
<head>
<meta name="viewport" content="width=device-width,initial-scale=1"/>
<title>LFR PID Tuner</title>
<style>
  :root{--bg:#0b1220;--card:#121b2f;--txt:#e9f0ff;--mut:#98a6c7;--acc:#5eead4;--warn:#fb7185;}
  body{margin:0;font-family:system-ui,Segoe UI,Roboto,Arial;background:radial-gradient(1200px 600px at 20% 0%,#182449 0%,var(--bg) 55%);color:var(--txt);}
  .wrap{max-width:860px;margin:0 auto;padding:18px;}
  .title{display:flex;align-items:center;justify-content:space-between;gap:12px;margin-bottom:14px;}
  .title h1{font-size:20px;margin:0;letter-spacing:.3px}
  .pill{font-size:12px;color:var(--bg);background:var(--acc);padding:6px 10px;border-radius:999px;font-weight:700}
  .grid{display:grid;grid-template-columns:1fr;gap:12px}
  @media(min-width:820px){.grid{grid-template-columns:1.1fr .9fr}}
  .card{background:linear-gradient(180deg,var(--card),#0f172a);border:1px solid rgba(255,255,255,.06);border-radius:16px;padding:14px;box-shadow:0 10px 25px rgba(0,0,0,.35)}
  .row{display:flex;align-items:center;justify-content:space-between;gap:10px}
  .k{font-size:13px;color:var(--mut)}
  .v{font-size:18px;font-weight:800}
  .btnpair{display:grid;grid-template-columns:repeat(2,1fr);gap:10px;width:220px}
  button{appearance:none;border:0;border-radius:14px;padding:12px 12px;font-weight:800;font-size:16px;color:var(--txt);
         background:rgba(255,255,255,.07);cursor:pointer;transition:.12s transform,.12s background}
  button:active{transform:scale(.98)}
  button:hover{background:rgba(255,255,255,.10)}
  .primary{background:rgba(94,234,212,.16);border:1px solid rgba(94,234,212,.25)}
  .danger{background:rgba(251,113,133,.14);border:1px solid rgba(251,113,133,.25)}
  .mini{font-size:12px;color:var(--mut);margin-top:6px}
  .statgrid{display:grid;grid-template-columns:repeat(2,1fr);gap:10px;margin-top:10px}
  .stat{background:rgba(255,255,255,.05);border-radius:12px;padding:10px;border:1px solid rgba(255,255,255,.06)}
  .footer{margin-top:10px;color:var(--mut);font-size:12px}
</style>
</head>
<body>
<div class="wrap">
  <div class="title">
    <h1>⚡ LFR PID Tuner</h1>
    <div class="pill">192.168.4.1</div>
  </div>

  <div class="grid">
    <div class="card">
      <div class="row">
        <div>
          <div class="k">Kp (step ±0.5)</div>
          <div class="v" id="kp">-</div>
        </div>
        <div class="btnpair">
          <button onclick="nudge('kp',-1)">−</button>
          <button class="primary" onclick="nudge('kp',+1)">+</button>
        </div>
      </div>

      <div style="height:10px"></div>

      <div class="row">
        <div>
          <div class="k">Ki (step ±0.01)</div>
          <div class="v" id="ki">-</div>
        </div>
        <div class="btnpair">
          <button onclick="nudge('ki',-1)">−</button>
          <button class="primary" onclick="nudge('ki',+1)">+</button>
        </div>
      </div>

      <div style="height:10px"></div>

      <div class="row">
        <div>
          <div class="k">Kd (step ±0.01)</div>
          <div class="v" id="kd">-</div>
        </div>
        <div class="btnpair">
          <button onclick="nudge('kd',-1)">−</button>
          <button class="primary" onclick="nudge('kd',+1)">+</button>
        </div>
      </div>

      <div class="mini">Tip: If it oscillates, reduce Kd. If it's lazy, increase Kp.</div>

      <div class="btnpair" style="margin-top:12px;width:100%;grid-template-columns:repeat(3,1fr)">
        <button class="primary" onclick="actionBtn('calib')">Recalibrate</button>
        <button class="danger" onclick="actionBtn('stop')">STOP</button>
        <button class="primary" onclick="actionBtn('run')">RUN</button>
      </div>
    </div>

    <div class="card">
      <div class="k">Live Status</div>
      <div class="statgrid">
        <div class="stat"><div class="k">pos</div><div class="v" id="pos">-</div></div>
        <div class="stat"><div class="k">sum</div><div class="v" id="sum">-</div></div>
        <div class="stat"><div class="k">err</div><div class="v" id="err">-</div></div>
        <div class="stat"><div class="k">lost</div><div class="v" id="lost">-</div></div>
        <div class="stat"><div class="k">outL</div><div class="v" id="outL">-</div></div>
        <div class="stat"><div class="k">outR</div><div class="v" id="outR">-</div></div>
      </div>
      <div class="footer">Connect to <b>LFR-Tuner</b> Wi-Fi and open <b>192.168.4.1</b></div>
    </div>
  </div>
</div>

<script>
const $ = (id)=>document.getElementById(id);

async function getStatus(){
  try{
    const r = await fetch('/api/status',{cache:'no-store'});
    const j = await r.json();
    $('kp').textContent = j.kp.toFixed(6);
    $('ki').textContent = j.ki.toFixed(6);
    $('kd').textContent = j.kd.toFixed(6);
    $('pos').textContent = j.pos;
    $('sum').textContent = j.sum;
    $('err').textContent = j.err.toFixed(1);
    $('lost').textContent = j.lost;
    $('outL').textContent = j.outL;
    $('outR').textContent = j.outR;
  }catch(e){}
}

async function nudge(p,d){
  await fetch(`/api/nudge?p=${p}&d=${d}`);
  await getStatus();
}

async function actionBtn(a){
  await fetch(`/api/action?a=${a}`);
}

setInterval(getStatus, 300);
getStatus();
</script>
</body></html>
)HTML";
}
