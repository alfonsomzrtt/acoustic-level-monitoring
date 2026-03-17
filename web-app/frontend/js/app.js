const splEl = document.getElementById("spl");
const minEl = document.getElementById("min");
const maxEl = document.getElementById("max");
const avgEl = document.getElementById("avg");

const statusEl = document.getElementById("status");


let min = Infinity;
let max = -Infinity;
let sum = 0;
let count = 0;

// =========================
// STATUS LOGIC
// =========================
function updateStatus(spl) {
  

  if (spl < 60 ) {
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

function updateStats(spl){

  const data = splData.datasets[0].data;

  if (data.length === 0) return;

  const min = Math.min(...data);
  const max = Math.max(...data);

  const sum = data.reduce((a,b)=> a+b,0)
  const avg = sum / data.length;

  minEl.textContent = min.toFixed(1);
  maxEl.textContent = max.toFixed(1);
  avgEl.textContent = avg.toFixed(1);

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

  // Update numbers
  splEl.textContent = spl.toFixed(1);

  updateStatus(spl);
  updateStats();


  // Rolling window update
  const now = new Date().toLocaleTimeString();

  splData.labels.push(now);
  splData.datasets[0].data.push(spl);

  if (splData.labels.length > MAX_POINTS) {

    splData.labels.shift();
    splData.datasets[0].data.shift();

    //reset statistics window
    min = Infinity;
    max = -Infinity;
    sum = 0; 
    count = 0;
  }

  splChart.update();
};

evtSource.onerror = () => {
  statusEl.textContent = "STATUS: DISCONNECTED";
};
