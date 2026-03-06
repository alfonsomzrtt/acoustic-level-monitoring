import serial

# =========================
# CONFIG
# =========================
SERIAL_PORT = "COM11"      # ganti sesuai port ESP32 kamu
BAUDRATE = 115200

# =========================
# OPEN SERIAL
# =========================
ser = serial.Serial(
    port=SERIAL_PORT,
    baudrate=BAUDRATE,
    timeout=1
)

print("Serial bridge started...")
print("Waiting for data...\n")

# =========================
# MAIN LOOP
# =========================
while True:
    try:
        line = ser.readline().decode("utf-8").strip()

        # Skip empty lines / header
        if not line or "dBFS" in line:
            continue

        parts = line.split(",")
        if len(parts) != 4:
            continue

        data = {
            "dbfs":  float(parts[0]),
            "noise": float(parts[1]),
            "snr":   float(parts[2]),
            "spl":   float(parts[3]),
        }

        print(data)

    except KeyboardInterrupt:
        print("\nStopped by user")
        break

    except Exception as e:
        print("Error:", e)
