# import serial
# import json
# import threading
# import time
# from flask import Flask, Response

# # =========================
# # CONFIG
# # =========================
# SERIAL_PORT = "COM11"     # ganti sesuai ESP32
# BAUDRATE = 115200

# # =========================
# # SHARED STATE
# # =========================
# latest_data = None

# # =========================
# # SERIAL READER THREAD
# # =========================
# def serial_reader():
#     global latest_data

#     ser = serial.Serial(
#         port=SERIAL_PORT,
#         baudrate=BAUDRATE,
#         timeout=1
#     )

#     while True:
#         try:
#             line = ser.readline().decode("utf-8").strip()

#             if not line or "dBFS" in line:
#                 continue

#             parts = line.split(",")
#             if len(parts) != 4:
#                 continue

#             latest_data = {
#                 "dbfs":  float(parts[0]),
#                 "noise": float(parts[1]),
#                 "snr":   float(parts[2]),
#                 "spl":   float(parts[3]),
#                 "ts":    time.time()
#             }

#         except Exception as e:
#             print("Serial error:", e)
#             time.sleep(1)

# # =========================
# # FLASK APP
# # =========================
# app = Flask(__name__)

# @app.route("/events")
# def sse_events():
#     def event_stream():
#         last_sent = None
#         while True:
#             if latest_data and latest_data != last_sent:
#                 yield f"data: {json.dumps(latest_data)}\n\n"
#                 last_sent = latest_data
#             time.sleep(0.1)  # ~10 Hz max
#     return Response(event_stream(), mimetype="text/event-stream")

# @app.route("/")
# def index():
#     return """
# <!DOCTYPE html>
# <html>
# <head>
#   <meta charset="utf-8">
#   <title>PA Monitoring</title>
# </head>
# <body>
#   <h1>Monitoring</h1>

#   <div>
#     <strong>SPL:</strong> <span id="spl">--</span> dB
#   </div>
#   <div>
#     <strong>SNR:</strong> <span id="snr">--</span> dB
#   </div>

#   <script>
#     const evtSource = new EventSource("/events");

#     evtSource.onmessage = function(event) {
#       const data = JSON.parse(event.data);
#       document.getElementById("spl").textContent = data.spl.toFixed(1);
#       document.getElementById("snr").textContent = data.snr.toFixed(1);
#     };
#   </script>
# </body>
# </html>
# """

# # =========================
# # MAIN
# # =========================
# if __name__ == "__main__":
#     t = threading.Thread(target=serial_reader, daemon=True)
#     t.start()

#     app.run(host="0.0.0.0", port=8000, debug=False)

import serial
import json
import threading
import time
from flask import Flask, Response, send_from_directory

# =========================
# CONFIG
# =========================
SERIAL_PORT = "COM11"     # sesuaikan
BAUDRATE = 115200

# =========================
# SHARED STATE
# =========================
latest_data = None
lock = threading.Lock()
# =========================
# SERIAL READER THREAD
# =========================
def serial_reader():
    global latest_data

    while True:
        try:
          with serial.Serial(
                port=SERIAL_PORT,
                baudrate=BAUDRATE,
                timeout=1) as ser:
            print("Serial connected")

            while True: 
                line = ser.readline().decode().strip()

                if "SPL" not in line:
                    continue

                try:
                    spl_str = line.split("SPL:")[1]. strip()
                    spl = float(spl_str)

                    with lock:
                        latest_data = {
                            "spl": spl,
                            "time": time.time()
                        }

                except:
                    continue  

        except Exception as e:
            print("Serial error:", e)
            time.sleep(2) #Jangan terlalu agresif

# =========================
# FLASK APP
# =========================
app = Flask(__name__, static_folder="frontend", static_url_path="")

@app.route("/")
def index():
    return send_from_directory("frontend", "index.html")

@app.route("/css/<path:filename>")
def css_files(filename):
    return send_from_directory("frontend/css", filename)

@app.route("/js/<path:filename>")
def js_files(filename):
    return send_from_directory("frontend/js", filename)

@app.route("/events")
def sse_events():
    def event_stream():
        last_sent = None

        while True:
            try:
                with lock:
                    data = latest_data

                if data and data != last_sent:
                    yield f"data: {json.dumps(data)}\n\n"
                    last_sent = data

                time.sleep(0.1)
                
            except GeneratorExit:
                print("Client disconnected")
                break

    
    return Response(event_stream(), 
                    mimetype="text/event-stream",
                    headers={
                        "Cache-Control": "no-cache",
                        "X-Accel-Buffering": "no"
                    }
                )

# =========================
# MAIN
# =========================
if __name__ == "__main__":
    t = threading.Thread(target=serial_reader, daemon=True)
    t.start()

    app.run(host="0.0.0.0", port=8000, debug=False)
