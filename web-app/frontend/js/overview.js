// =========================
// CONFIG & STATE
// =========================
const BROKER = "b8ae4809915f4027b2d18c7fc219b204.s1.eu.hivemq.cloud";
const PORT = 8884;
const USER = "esp32-v1";
const PASS = "RajaSawit_2026";

const TIMEOUT = 10000; // ms → dianggap offline

// 1. Tentukan daftar gate yang dimiliki, hardcoded
const raw_nodes = ["gate1", "gate2", "gate3", "gate4", "gate5", "gate6"];
const INITIAL_NODES = raw_nodes.map(node => node.toUpperCase());
const nodes = {}; 


//DOM
const grid = document.getElementById("grid");


// =========================
// CREATE CARD (Living Card)
// =========================
function createCard(nodeId) {
  //mencegah duplikasi jika card sudah diinisialisasi
  if (nodes[nodeId]) return;

  const el = document.createElement("div");
  
  el.className = "card offline"; //kondisi default offline

  el.innerHTML = `
  <div class="card-header">
        <div class="node-badge">${nodeId.replace('gate', '').toUpperCase()}</div>
        <div class="status-indicator">
            <span class="status-text">OFFLINE</span>
            <div class="status-dot"></div>
        </div>
  </div>

  <div class="card-body">
        <div class="main-stat">
            <h2 class="value">--</h2>
            <span class="unit">dBA</span>
        </div>
        <div class="trend-indicator trend-neutral">
            <span class="trend-icon">--</span>
            <span class="trend-pct">--%</span>        
        </div>
  </div>

  <div class="card-footer">
        <span class="last-seen">Last updated: Never</span>
  </div>
  `;

  el.onclick = () => {
    window.location.href = `dashboard.html?node=${nodeId}`;
  };

  grid.appendChild(el);

  nodes[nodeId] = {
    value: null,
    lastUpdate: 0, //langsung dianggap Watchdog offline
    el: el,
    previousValue: null //dibutuhkan untuk menghitung tren
  };
}

//Fungsi untuk inisialisasi card saat startup, biar instan
function initDashboard() {
  INITIAL_NODES.forEach(nodeId => {
    createCard(nodeId);
  });
}

initDashboard();

function updateCard(nodeId, value) {
    if (!nodes[nodeId]) createCard(nodeId);
    const node = nodes[nodeId];
  
    // 1. Hitung Tren (%)
    let trendPct = 0;
    let trendIcon = "•";
    let trendClass = "trend-neutral";
  
    if (node.previousValue !== null && node.previousValue !== 0) {
        trendPct = ((value - node.previousValue) / node.previousValue) * 100;
        if (value > node.previousValue) {
            trendIcon = "▲";
            trendClass = "trend-up"; // Merah: Kebisingan naik itu buruk
        } else if (value < node.previousValue) {
            trendIcon = "▼";
            trendClass = "trend-down"; // Hijau: Kebisingan turun itu baik
        }
    }
  
    // 2. Update State
    node.previousValue = value;
    node.value = value;
    node.lastUpdate = Date.now();
  
    // 3. Update UI
    const el = node.el;
  
    //Angka Utama
    el.querySelector(".value").textContent = value.toFixed(1);
    
    // Update tren dengan class warna
    const trendContainer = el.querySelector(".trend-indicator");
    trendContainer.className = `trend-indicator ${trendClass}`;
    el.querySelector(".trend-pct").textContent = Math.abs(trendPct).toFixed(1) + "%";
    el.querySelector(".trend-icon").textContent = trendIcon;
  
    // Update Status & Time
    el.querySelector(".status-text").textContent = "ONLINE";
    el.querySelector(".last-seen").textContent = "Updated: " + new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
  
    // 4. Update Level Bahaya (Card Border/Glow)
    el.classList.remove("online", "warning", "danger", "offline");
    if (value > 80) el.classList.add("danger", "online");
    else if (value > 60) el.classList.add("warning", "online");
    else el.classList.add("online");
  }

// =========================
// OFFLINE WATCHDOG
// =========================
function checkOffline() {
  const now = Date.now();

  Object.keys(nodes).forEach(nodeId => {
    const node = nodes[nodeId];

// Jika lewat timeout, ubah UI ke state offline    
    if (now - node.lastUpdate > TIMEOUT) {
      const el = node.el;

     //Peraiki selector agar tidak null
     const statusText = el.querySelector(".status-text");
     if (statusText) statusText.textContent = "OFFLINE";

     el.classList.remove("online", "warning", "danger");
     el.classList.add("offline");
    }
  });
}

// Jalankan pengecekan setiap 2 detik
setInterval(checkOffline, 2000);

// =========================
// MQTT SETUP (PAHO)
// =========================
const client = new Paho.MQTT.Client(
  BROKER,
  PORT,
  "overview_" + Math.random()
);

// connection lost
client.onConnectionLost = function (responseObject) {
  console.warn("MQTT lost:", responseObject.errorMessage);
};

// message handler
client.onMessageArrived = function (message) {
  const topic = message.destinationName;
  const payload = message.payloadString;

  // Ekstrak nodeId dari topic monitoring/nodeId/db
  // contoh topic: monitoring/gate1/db
  const nodeId = topic.split("/")[1];

  let value;

  // fleksibel parsing
  try {
    const data = JSON.parse(payload);
    value = data.spl ?? data.value ?? parseFloat(payload);
  } catch {
    value = parseFloat(payload);
  }

  if (!isNaN(value)) {
  updateCard(nodeId, value);
  }
};

// connect
function onConnect() {
  console.log("MQTT connected (overview)");

  // wildcard → semua node
  client.subscribe("monitoring/+/db"); //subscribe ke semua node
}

client.connect({
  userName: USER,
  password: PASS,
  useSSL: true,
  onSuccess: onConnect,
//   reconnect: true
});

