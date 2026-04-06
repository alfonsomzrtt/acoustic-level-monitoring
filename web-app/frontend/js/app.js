// // =========================
// // DOM ELEMENTS
// // =========================
// const splEl = document.getElementById("spl");
// const minEl = document.getElementById("min");
// const maxEl = document.getElementById("max");
// const avgEl = document.getElementById("avg");
// const statusEl = document.getElementById("status");

// // =========================
// // STATE
// // =========================
// const MAX_POINTS = 30;

// // =========================
// // STATUS LOGIC
// // =========================
// function updateStatus(spl) {
//   if (spl < 60) {
//     statusEl.textContent = "STATUS: NORMAL";
//     splEl.style.color = "#22C55E";
//   } else if (spl < 80) {
//     statusEl.textContent = "STATUS: WARNING";
//     splEl.style.color = "#FACC15";
//   } else {
//     statusEl.textContent = "STATUS: DANGER";
//     splEl.style.color = "#EF4444";
//   }
// }

// function updateStats() {
//   const data = splData.datasets[0].data;

//   if (data.length === 0) return;

//   const min = Math.min(...data);
//   const max = Math.max(...data);
//   const sum = data.reduce((a, b) => a + b, 0);
//   const avg = sum / data.length;

//   minEl.textContent = min.toFixed(1);
//   maxEl.textContent = max.toFixed(1);
//   avgEl.textContent = avg.toFixed(1);
// }

// // =========================
// // CHART SETUP
// // =========================
// const ctx = document.getElementById("splChart").getContext("2d");

// const splData = {
//   labels: [],
//   datasets: [{
//     label: "SPL (dB)",
//     data: [],
//     borderColor: "#22C55E",
//     backgroundColor: "rgba(34,197,94,0.15)",
//     borderWidth: 2,
//     tension: 0.3,
//     fill: true,
//     pointRadius: 1
//   }]
// };

// const splChart = new Chart(ctx, {
//   type: "line",
//   data: splData,
//   options: {
//     responsive: true,
//     maintainAspectRatio: false,
//     animation: false,
//     scales: {
//       x: {
//         display: false
//       },
//       y: {
//         min: 0,
//         max: 100,
//         ticks: {
//           color: "#94A3B8"
//         }
//       }
//     },
//     plugins: {
//       legend: {
//         display: false
//       }
//     }
//   }
// });

// // =========================
// // MQTT CONNECTION
// // =========================
// const client = new Paho.MQTT.Client(
//   "b8ae4809915f4027b2d18c7fc219b204.s1.eu.hivemq.cloud",
//   8884,
//   "webclient_" + Math.random()
// );

// client.onConnectionLost = function () {
//   console.warn("MQTT disconnected");
//   statusEl.textContent = "STATUS: DISCONNECTED";
// };

// client.onMessageArrived = function (message) {
//   const data = JSON.parse(message.payloadString);
//   const spl = data.spl;


//   // Update main value
//   splEl.textContent = spl.toFixed(1);

//   // Update status
//   updateStatus(spl);
//   // Update chart
//   const now = new Date().toLocaleTimeString();

//   splData.labels.push(now);
//   splData.datasets[0].data.push(spl);

//   if (splData.labels.length > MAX_POINTS) {
//     splData.labels.shift();
//     splData.datasets[0].data.shift();
//   }

//   // Update stats (based on current window)
//   updateStats();
//   splChart.update();
// };

// function onConnect() {
//   console.log("MQTT connected");
//   statusEl.textContent = "STATUS: CONNECTED";
//   client.subscribe("spl/data");
// }

// client.connect({
//   userName: "esp32-v1",
//   password: "RajaSawit_2026",
//   useSSL: true,
//   onSuccess: onConnect
// });


//tes kode baru
// =========================
// CONFIG
// =========================
const BROKER = "b8ae4809915f4027b2d18c7fc219b204.s1.eu.hivemq.cloud";
const PORT = 8884;
const USER = "esp32-v1";
const PASS = "RajaSawit_2026";

const MAX_POINTS = 30;
const TIMEOUT = 5000; // ms

// =========================
// GET NODE FROM URL
// =========================
const params = new URLSearchParams(window.location.search);
const nodeId = params.get("node") || "unknown";

// update title (optional)
document.querySelector("h1").textContent = "Node: " + nodeId;

// =========================
// DOM ELEMENTS
// =========================
const splEl = document.getElementById("spl");
const minEl = document.getElementById("min");
const maxEl = document.getElementById("max");
const avgEl = document.getElementById("avg");
const statusEl = document.getElementById("status");

// =========================
// STATE
// =========================
let lastUpdate = 0;

// =========================
// STATUS LOGIC
// =========================
function updateStatus(spl) {
  if (spl < 60) {
    statusEl.textContent = "STATUS: NORMAL";
    splEl.style.color = "#22C55E";
  } else if (spl < 80) {
    statusEl.textContent = "STATUS: WARNING";
    splEl.style.color = "#FACC15";
  } else {
    statusEl.textContent = "STATUS: DANGER";
    splEl.style.color = "#EF4444";
  }
}

// =========================
// STATS
// =========================
function updateStats() {
  const data = splData.datasets[0].data;
  if (data.length === 0) return;

  const min = Math.min(...data);
  const max = Math.max(...data);
  const sum = data.reduce((a, b) => a + b, 0);
  const avg = sum / data.length;

  if (minEl) minEl.textContent = min.toFixed(1);
  if (maxEl) maxEl.textContent = max.toFixed(1);
  if (avgEl) avgEl.textContent = avg.toFixed(1);
}

// =========================
// CHART SETUP
// =========================
const ctx = document.getElementById("splChart").getContext("2d");

const splData = {
  labels: [],
  datasets: [{
    label: "SPL (dB)",
    data: [],
    borderColor: "#22C55E",
    backgroundColor: "rgba(34,197,94,0.15)",
    borderWidth: 2,
    tension: 0.3,
    fill: true,
    pointRadius: 1
  }]
};

const splChart = new Chart(ctx, {
  type: "line",
  data: splData,
  options: {
    responsive: true,
    maintainAspectRatio: false,
    animation: false,
    scales: {
      x: { display: false },
      y: {
        min: 0,
        max: 100,
        ticks: { color: "#94A3B8" }
      }
    },
    plugins: {
      legend: { display: false }
    }
  }
});

// =========================
// MQTT SETUP (PAHO)
// =========================
const client = new Paho.MQTT.Client(
  BROKER,
  PORT,
  "detail_" + nodeId + "_" + Math.random()
);

// connection lost
client.onConnectionLost = function (responseObject) {
  console.warn("MQTT lost:", responseObject.errorMessage);
  statusEl.textContent = "STATUS: DISCONNECTED";
};

// message handler
client.onMessageArrived = function (message) {
  const payload = message.payloadString;

  let spl;

  // fleksibel parsing
  try {
    const data = JSON.parse(payload);
    spl = data.spl ?? data.value ?? parseFloat(payload);
  } catch {
    spl = parseFloat(payload);
  }

  if (isNaN(spl)) return;

  lastUpdate = Date.now();

  // update UI
  splEl.textContent = spl.toFixed(1);
  updateStatus(spl);

  // chart update
  const now = new Date().toLocaleTimeString();

  splData.labels.push(now);
  splData.datasets[0].data.push(spl);

  if (splData.labels.length > MAX_POINTS) {
    splData.labels.shift();
    splData.datasets[0].data.shift();
  }

  updateStats();
  splChart.update();
};

// connect
function onConnect() {
  console.log("MQTT connected (detail)");

  statusEl.textContent = "STATUS: CONNECTED";

  const topic = `monitoring/${nodeId}/db`;
  client.subscribe(topic);
}

// connect options
client.connect({
  userName: USER,
  password: PASS,
  useSSL: true,
  onSuccess: onConnect,
  // reconnect: true
});

// =========================
// OFFLINE WATCHDOG
// =========================
setInterval(() => {
  if (Date.now() - lastUpdate > TIMEOUT) {
    statusEl.textContent = "STATUS: OFFLINE";
  }
}, 2000);