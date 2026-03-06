const splEl = document.getElementById("spl");
const snrEl = document.getElementById("snr");
const statusEl = document.getElementById("status");
const splMeter = document.getElementById("splMeter");

// =========================
// STATUS LOGIC
// =========================
function updateStatus(spl) {
  splMeter.classList.remove("normal", "warning", "danger");

  if (spl < 60) {
    splMeter.classList.add("normal");
    statusEl.textContent = "STATUS: NORMAL";
  } else if (spl < 80) {
    splMeter.classList.add("warning");
    statusEl.textContent = "STATUS: WARNING";
  } else {
    splMeter.classList.add("danger");
    statusEl.textContent = "STATUS: DANGER";
  }
}

// =========================
// CHART SETUP
// =========================
const ctx = document.getElementById("splChart").getContext("2d");

const MAX_POINTS = 30; // 30 detik @ ~1 Hz

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
      x: {
        display: false
      },
      y: {
        min: 0,
        max: 100,
        ticks: {
          color: "#94A3B8"
        }
      }
    },
    plugins: {
      legend: {
        display: false
      }
    }
  }
});

// =========================
// DATA STREAM (SSE)
// =========================
const evtSource = new EventSource("/events");

evtSource.onmessage = (event) => {
  const data = JSON.parse(event.data);
  const spl = data.spl;
  const snr = data.snr;

  // Update numbers
  splEl.textContent = spl.toFixed(1);
  snrEl.textContent = snr.toFixed(1);

  updateStatus(spl);

  // Rolling window update
  const now = new Date().toLocaleTimeString();

  splData.labels.push(now);
  splData.datasets[0].data.push(spl);

  if (splData.labels.length > MAX_POINTS) {
    splData.labels.shift();
    splData.datasets[0].data.shift();
  }

  splChart.update();
};

evtSource.onerror = () => {
  statusEl.textContent = "STATUS: DISCONNECTED";
};
