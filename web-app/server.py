# # import serial
# # import json
# # import threading
# # import time
# # from flask import Flask, Response

# # # =========================
# # # CONFIG
# # # =========================
# # SERIAL_PORT = "COM11"     # ganti sesuai ESP32
# # BAUDRATE = 115200

# # # =========================
# # # SHARED STATE
# # # =========================
# # latest_data = None

# # # =========================
# # # SERIAL READER THREAD
# # # =========================
# # def serial_reader():
# #     global latest_data

# #     ser = serial.Serial(
# #         port=SERIAL_PORT,
# #         baudrate=BAUDRATE,
# #         timeout=1
# #     )

# #     while True:
# #         try:
# #             line = ser.readline().decode("utf-8").strip()

# #             if not line or "dBFS" in line:
# #                 continue

# #             parts = line.split(",")
# #             if len(parts) != 4:
# #                 continue

# #             latest_data = {
# #                 "dbfs":  float(parts[0]),
# #                 "noise": float(parts[1]),
# #                 "snr":   float(parts[2]),
# #                 "spl":   float(parts[3]),
# #                 "ts":    time.time()
# #             }

# #         except Exception as e:
# #             print("Serial error:", e)
# #             time.sleep(1)

# # # =========================
# # # FLASK APP
# # # =========================
# # app = Flask(__name__)

# # @app.route("/events")
# # def sse_events():
# #     def event_stream():
# #         last_sent = None
# #         while True:
# #             if latest_data and latest_data != last_sent:
# #                 yield f"data: {json.dumps(latest_data)}\n\n"
# #                 last_sent = latest_data
# #             time.sleep(0.1)  # ~10 Hz max
# #     return Response(event_stream(), mimetype="text/event-stream")

# # @app.route("/")
# # def index():
# #     return """
# # <!DOCTYPE html>
# # <html>
# # <head>
# #   <meta charset="utf-8">
# #   <title>PA Monitoring</title>
# # </head>
# # <body>
# #   <h1>Monitoring</h1>

# #   <div>
# #     <strong>SPL:</strong> <span id="spl">--</span> dB
# #   </div>
# #   <div>
# #     <strong>SNR:</strong> <span id="snr">--</span> dB
# #   </div>

# #   <script>
# #     const evtSource = new EventSource("/events");

# #     evtSource.onmessage = function(event) {
# #       const data = JSON.parse(event.data);
# #       document.getElementById("spl").textContent = data.spl.toFixed(1);
# #       document.getElementById("snr").textContent = data.snr.toFixed(1);
# #     };
# #   </script>
# # </body>
# # </html>
# # """

# # # =========================
# # # MAIN
# # # =========================
# # if __name__ == "__main__":
# #     t = threading.Thread(target=serial_reader, daemon=True)
# #     t.start()

# #     app.run(host="0.0.0.0", port=8000, debug=False)

# import serial
# import json
# import threading
# import time
# from flask import Flask, Response, send_from_directory
# from zeroconf import ServiceInfo, Zeroconf
# import socket
# from collections import deque
# from waitress import serve




# # =========================
# # multicast DNS//mDNS configuration, often called zero-configuration.
# # =========================

# def get_local_ip():
#     s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
#     try:
#         s.connect(("8.8.8.8", 80))
#         ip = s.getsockname()[0]
#     finally:
#         s.close()
#     return ip


# def start_mdns():
#     zeroconf = Zeroconf()
#     ip = get_local_ip()

#     info = ServiceInfo(
#         "_http._tcp.local.",
#         "spl-monitor._http._tcp.local.",
#         addresses=[socket.inet_aton(ip)],
#         port=8000,
#         properties={},
#         server="spl-monitor.local."
#     )

#     zeroconf.register_service(info)
#     print(f"mDNS aktif: http://spl-monitor.local:8000")
#     print(f"IP fallback: http://{ip}:8000")

#     return zeroconf


# # =========================
# # CONFIG
# # =========================
# SERIAL_PORT = "COM11"     # sesuaikan
# BAUDRATE = 115200

# # =========================
# # SHARED STATE
# # =========================
# latest_data = None
# lock = threading.Lock()
# # =========================
# # SERIAL READER THREAD
# # =========================
# def serial_reader():
#     global latest_data

#     while True:
#         try:
#           with serial.Serial(
#                 port=SERIAL_PORT,
#                 baudrate=BAUDRATE,
#                 timeout=1) as ser:
#             print("Serial connected")

#             while True: 
#                 line = ser.readline().decode(errors="ignore").strip()

#                 if "SPL:" not in line or "dBFS:" not in line:
#                     continue

#                 try:
#                     parts = line.split()

#                     dbfs = None
#                     spl = None
                    
#                     for p in parts:
#                         if p.startswith("dBFS:"):
#                             dbfs = float(p.replace("dBFS:", ""))
#                         elif p.startswith("SPL:"):
#                             spl = float(p.replace("SPL:", ""))

#                     if spl is None: 
#                         continue

#                     with lock:
#                         latest_data = {
#                             "spl": spl,
#                             "dbfs": dbfs,
#                             "ts": time.time()
#                         }
#                         history.append(spl)

#                 except:
#                     continue  

#         except Exception as e:
#             print("Serial error:", e)
#             time.sleep(2) #Jangan terlalu agresif

# # =========================
# # FLASK APP
# # =========================
# app = Flask(__name__, static_folder="frontend", static_url_path="")

# @app.route("/")
# def index():
#     return send_from_directory("frontend", "index.html")

# @app.route("/css/<path:filename>")
# def css_files(filename):
#     return send_from_directory("frontend/css", filename)

# @app.route("/js/<path:filename>")
# def js_files(filename):
#     return send_from_directory("frontend/js", filename)

# @app.route("/events")
# def sse_events():
#     def event_stream():
#         last_sent = None

#         while True:
#             try:
#                 with lock:
#                     data = latest_data.copy() if latest_data else None

#                 if data and data != last_sent:
#                     yield f"data: {json.dumps(data)}\n\n"
#                     last_sent = data

#                 time.sleep(0.5)
                
#             except GeneratorExit:
#                 print("Client disconnected")
#                 break

    
#     return Response(event_stream(), 
#                     mimetype="text/event-stream",
#                     headers={
#                         "Cache-Control": "no-cache",
#                         "X-Accel-Buffering": "no"
#                     }
#                 )

# @app.route("/api/spl")
# def get_spl():
#     with lock:
#         data = latest_data.copy() if latest_data else None
        
#         return { 
#             "status": "ok",
#             "data": data
#         }
    
# @app.route("/api/health")
# def health():
#     now = time.time()

#     with lock:
#         data = latest_data

#     return {
#         "status": "ok",
#         "data": {
#             "serial": data is not None,
#             "fresh": (now - data["ts"] < 2) if data else False
#             }
#     }

# # History
# history = deque(maxlen=300)
# @app.route("/api/history")
# def get_history():
#     with lock:
#         return {
#             "status": "ok",
#             "data": list(history)
#         }


# # =========================
# # MAIN
# # =========================
# if __name__ == "__main__":
#     zeroconf = start_mdns()

#     t = threading.Thread(target=serial_reader, daemon=True)
#     t.start()

#     try: 
#         serve(app, host="0.0.0.0", port=8000)
#     finally:
#         zeroconf.close()

import serial
import json
import threading
import time
from flask import Flask, Response, send_from_directory
from collections import deque
from waitress import serve
import paho.mqtt.client as mqtt

# =========================
# CONFIG
# =========================
SERIAL_PORT = "COM11"
BAUDRATE = 115200
MQTT_BROKER = "192.168.88.221"  # bisa diganti IP broker
MQTT_PORT = 1883
MQTT_TOPIC = "spl/data"

# =========================
# SHARED STATE
# =========================
latest_data = None
lock = threading.Lock()
history = deque(maxlen=300)

# =========================
# MQTT SETUP
# =========================
def on_connect(client, userdata, flags, rc):
    print("MQTT connected with result code", rc)

mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.loop_start()

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
                    line = ser.readline().decode(errors="ignore").strip()

                    if "SPL:" not in line or "dBFS:" not in line:
                        continue

                    try:
                        parts = line.split()
                        dbfs = None
                        spl = None

                        for p in parts:
                            if p.startswith("dBFS:"):
                                dbfs = float(p.replace("dBFS:", ""))
                            elif p.startswith("SPL:"):
                                spl = float(p.replace("SPL:", ""))

                        if spl is None:
                            continue

                        data = {
                            "spl": spl,
                            "dbfs": dbfs,
                            "ts": time.time()
                        }

                        # print("Publishing:", data) #kalau ingin debug kecil
                        # mqtt_client.publish(MQTT_TOPIC, json.dumps(data))

                        # update local state
                        with lock:
                            latest_data = data
                            history.append(spl)

                        # publish ke MQTT
                        mqtt_client.publish(MQTT_TOPIC, json.dumps(data))

                    except:
                        continue

        except Exception as e:
            print("Serial error:", e)
            time.sleep(2)

# =========================
# FLASK APP
# =========================
app = Flask(__name__, static_folder="frontend", static_url_path="")

@app.route("/")
def index():
    return send_from_directory("frontend", "index.html")

@app.route("/events")
def sse_events():
    def event_stream():
        last_sent = None

        while True:
            try:
                with lock:
                    data = latest_data.copy() if latest_data else None

                if data and data != last_sent:
                    yield f"data: {json.dumps(data)}\n\n"
                    last_sent = data

                time.sleep(0.1)

            except GeneratorExit:
                print("Client disconnected")
                break

    return Response(event_stream(), mimetype="text/event-stream")

@app.route("/api/spl")
def get_spl():
    with lock:
        data = latest_data.copy() if latest_data else None

    return {
        "status": "ok",
        "data": data
    }

@app.route("/api/history")
def get_history():
    with lock:
        return {
            "status": "ok",
            "data": list(history)
        }

@app.route("/api/health")
def health():
    now = time.time()

    with lock:
        data = latest_data

    return {
        "status": "ok",
        "data": {
            "serial": data is not None,
            "fresh": (now - data["ts"] < 2) if data else False
        }
    }

# =========================
# MAIN
# =========================
if __name__ == "__main__":
    t = threading.Thread(target=serial_reader, daemon=True)
    t.start()

    serve(app, host="0.0.0.0", port=8000)
