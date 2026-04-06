import paho.mqtt.client as mqtt
import json
import time
import random

# =========================
# CONFIG (Sesuai overview.js)
# =========================
BROKER = "b8ae4809915f4027b2d18c7fc219b204.s1.eu.hivemq.cloud"
PORT = 8883 # Port SSL untuk Paho Python
USER = "esp32-v1"
PASS = "RajaSawit_2026"

# List node yang ingin disimulasikan
NODES = ["GATE1", "GATE3","GATE4", "GATE5",
         "GATE6"]

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Simulator Connected to HiveMQ Cloud!")
    else:
        print(f"Failed to connect, return code {rc}")

client = mqtt.Client()
client.username_pw_set(USER, PASS)
client.tls_set() # Wajib untuk HiveMQ Cloud (SSL)
client.on_connect = on_connect

client.connect(BROKER, PORT)
client.loop_start()

print("Simulasi berjalan... Tekan Ctrl+C untuk berhenti.")

try:
    while True:
        for node in NODES:
            # Simulasi nilai dBA acak antara 40 - 90
            spl_value = round(random.uniform(45.0, 85.0), 1)
            
            # Buat payload JSON sesuai format overview.js
            payload = json.dumps({"spl": spl_value})
            
            # Topic: monitoring/{nodeId}/db
            topic = f"monitoring/{node}/db"
            
            client.publish(topic, payload)
            print(f"Sent to {topic}: {spl_value} dBA")
            
        time.sleep(5) # Kirim data setiap n detik
except KeyboardInterrupt:
    print("\nSimulasi dihentikan.")
    client.loop_stop()
    client.disconnect()
