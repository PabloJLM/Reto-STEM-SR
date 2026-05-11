#pragma once

// webpage.h  —  Interfaz web para Arduino Opta
// Pagina de monitoreo y control: 4 relays D0-D3 + 2 entradas analogicas

const char PAGE_CSS[] PROGMEM = R"css(

:root {
  --bg:       #0f1117;
  --bg-card:  #1a1d27;
  --bg-bar:   #2a2d3a;
  --border:   #2e3247;
  --accent:   #3b82f6;
  --ok:       #22c55e;
  --warn:     #f59e0b;
  --err:      #ef4444;
  --text:     #e2e8f0;
  --muted:    #64748b;
  --label:    #94a3b8;
  --radius:   10px;
  --font:     'Segoe UI', 'Helvetica Neue', Arial, sans-serif;
}

*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
body { font-family: var(--font); background: var(--bg); color: var(--text); min-height: 100vh; line-height: 1.5; }

header { background: var(--bg-card); border-bottom: 1px solid var(--border); padding: 16px 24px; display: flex; align-items: center; gap: 16px; }
header h1 { font-size: 1.25rem; font-weight: 700; color: var(--accent); }
.ip-badge { font-size: 0.75rem; color: var(--muted); background: var(--bg-bar); padding: 3px 10px; border-radius: 20px; border: 1px solid var(--border); }

.container { max-width: 820px; margin: 24px auto; padding: 0 16px; display: flex; flex-direction: column; gap: 18px; }

.card { background: var(--bg-card); border: 1px solid var(--border); border-radius: var(--radius); padding: 20px 24px; }
.card h2 { font-size: 0.72rem; font-weight: 600; text-transform: uppercase; letter-spacing: 1.2px; color: var(--muted); margin-bottom: 16px; }

/* Relays */
.relay-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 12px; }
.relay-btn { display: flex; flex-direction: column; align-items: center; gap: 8px; padding: 14px 8px; border-radius: 8px; border: 1px solid var(--border); background: var(--bg-bar); cursor: pointer; transition: all 0.15s; user-select: none; }
.relay-btn:hover { border-color: var(--accent); }
.relay-btn.on { background: #14532d33; border-color: var(--ok); }
.relay-label { font-size: 0.68rem; text-transform: uppercase; letter-spacing: 0.8px; color: var(--label); }
.relay-name  { font-size: 0.82rem; font-weight: 600; }
.relay-badge { font-size: 0.72rem; padding: 2px 10px; border-radius: 20px; font-weight: 600; background: var(--bg); color: var(--muted); border: 1px solid var(--border); }
.relay-btn.on .relay-badge { background: #14532d33; color: var(--ok); border-color: var(--ok); }

/* Entradas analogicas */
.ain-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; }
.ain-item label { display: block; font-size: 0.72rem; color: var(--label); margin-bottom: 6px; }
.ain-bar-outer { background: var(--bg-bar); border-radius: 6px; height: 8px; overflow: hidden; margin-bottom: 6px; }
.ain-bar-inner { height: 100%; background: linear-gradient(90deg, var(--accent), var(--ok)); border-radius: 6px; transition: width 0.4s ease; }
.ain-value { font-size: 1.4rem; font-weight: 700; font-variant-numeric: tabular-nums; }
.ain-unit  { font-size: 0.8rem; color: var(--muted); }

/* Status dot */
.status-row { display: flex; align-items: center; gap: 10px; }
.dot { width: 10px; height: 10px; border-radius: 50%; background: var(--err); }
.dot.ok { background: var(--ok); box-shadow: 0 0 6px var(--ok); }
.dot-label { font-size: 0.82rem; color: var(--muted); }

footer { text-align: center; padding: 20px; font-size: 0.72rem; color: var(--muted); }

@media (max-width: 520px) {
  .relay-grid { grid-template-columns: repeat(2, 1fr); }
  .ain-grid   { grid-template-columns: 1fr; }
}

)css";


const char PAGE_HTML[] PROGMEM = R"html(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Arduino Opta — Control</title>
  <style id="injected-css"><!-- CSS_PLACEHOLDER --></style>
</head>
<body>

  <header>
    <h1>Arduino Opta</h1>
    <span class="ip-badge" id="ip-lbl">conectando...</span>
  </header>

  <main class="container">

    <!-- Estado de conexion -->
    <section class="card">
      <h2>Estado</h2>
      <div class="status-row">
        <div class="dot" id="conn-dot"></div>
        <span class="dot-label" id="conn-lbl">Sin respuesta</span>
      </div>
    </section>

    <!-- Control de relays -->
    <section class="card">
      <h2>Relays de salida (D0 – D3 / 10 A NO)</h2>
      <div class="relay-grid">
        <div class="relay-btn" id="rb0" onclick="toggleRelay(0)">
          <span class="relay-label">D0 / PI_6</span>
          <span class="relay-name">Relay 1</span>
          <span class="relay-badge" id="rs0">OFF</span>
        </div>
        <div class="relay-btn" id="rb1" onclick="toggleRelay(1)">
          <span class="relay-label">D1 / PI_5</span>
          <span class="relay-name">Relay 2</span>
          <span class="relay-badge" id="rs1">OFF</span>
        </div>
        <div class="relay-btn" id="rb2" onclick="toggleRelay(2)">
          <span class="relay-label">D2 / PI_7</span>
          <span class="relay-name">Relay 3</span>
          <span class="relay-badge" id="rs2">OFF</span>
        </div>
        <div class="relay-btn" id="rb3" onclick="toggleRelay(3)">
          <span class="relay-label">D3 / PI_4</span>
          <span class="relay-name">Relay 4</span>
          <span class="relay-badge" id="rs3">OFF</span>
        </div>
      </div>
    </section>

    <!-- Entradas analogicas -->
    <section class="card">
      <h2>Entradas analogicas (I1 / I2 — 0…10 V)</h2>
      <div class="ain-grid">
        <div class="ain-item">
          <label>I1 — A0 (PA0_C)</label>
          <div class="ain-bar-outer"><div class="ain-bar-inner" id="bar0" style="width:0%"></div></div>
          <span class="ain-value" id="ain0">--</span><span class="ain-unit"> V</span>
        </div>
        <div class="ain-item">
          <label>I2 — A1 (PC2_C)</label>
          <div class="ain-bar-outer"><div class="ain-bar-inner" id="bar1" style="width:0%"></div></div>
          <span class="ain-value" id="ain1">--</span><span class="ain-unit"> V</span>
        </div>
      </div>
    </section>

  </main>

  <footer> &mdash; Arduino Opta Demo V1.0</footer>

  <script>
    const relayState = [false, false, false, false];

    async function poll() {
      try {
        const d = await (await fetch('/status')).json();

        document.getElementById('conn-dot').className = 'dot ok';
        document.getElementById('conn-lbl').textContent = 'Conectado';
        document.getElementById('ip-lbl').textContent = location.hostname;

        // Relays
        d.relays.forEach((on, i) => {
          relayState[i] = on;
          document.getElementById('rb' + i).className = 'relay-btn' + (on ? ' on' : '');
          document.getElementById('rs' + i).textContent = on ? 'ON' : 'OFF';
        });

        // Entradas analogicas (0..10V)
        const ain = [d.ain0, d.ain1];
        ain.forEach((v, i) => {
          document.getElementById('ain' + i).textContent = v.toFixed(2);
          document.getElementById('bar' + i).style.width = (v / 10 * 100).toFixed(1) + '%';
        });

      } catch (e) {
        document.getElementById('conn-dot').className = 'dot';
        document.getElementById('conn-lbl').textContent = 'Sin respuesta';
      }
    }

    async function toggleRelay(n) {
      const newState = !relayState[n];
      try {
        await fetch('/relay', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: 'relay=' + n + '&val=' + (newState ? 1 : 0)
        });
        poll();
      } catch (e) {}
    }

    poll();
    setInterval(poll, 1500);
  </script>

</body>
</html>
)html";
