# AcousticLevel
**An IoT-Based Distributed Sound Pressure Level Monitoring System**
---
This project aims to address several challenges I found during my internship period at PT Angkasa Pura Indonesia (Injourney Airports). Currently, monitoring sound pressure levels at Juanda International Airport boarding gates requires manual, on-site device checks, which often fail to accurately reflect the noise perceived by visitors and passengers.

The quality and configuration of the Public Address (PA) system significantly influence sound and noise levels. Poor settings can lead to sound pollution, causing hearing damage, personal discomfort, and making announcements can hardly be heard or understood clearly.

To resolve these issues, I developed a low-cost, functional IoT Sound Pressure Level Monitoring and data acquisition system. Using the INMP441 MEMS I2S microphone and ESP32 microcontroller as the core hardware, the system connects to a web-based dashboard. This allows local technicians to remotely monitor devices in real time, eliminating the need for frequent physical site visits.

I will try to comprise everything in this following items:
- System Design/Architecture
- Firmware Logics/DSP Pipeline
- Pinout and Configuration
- 3D print and enclosure
- Probably Bill of Materials (BOMs)

### System Architecture
```mermaid
graph TD
    subgraph Sensing["Sensing Layer (Hardware)"]
        A[INMP441 MEMS Microphone]
    end

    subgraph Processing["Processing Layer (ESP32 Edge)"]
        B[I2S Data Acquisition]
        C[DSP Core: DC Removal + A-Weighting]
        D[RMS & dBA Calculation]
        E[EMA Smoothing]
    end

    subgraph Display["Local & Remote Output"]
        F[LCD 16x2 I2C Display]
        G[MQTT Client - Secure TLS]
    end

    subgraph Cloud["Cloud Infrastructure"]
        H[HiveMQ Cloud MQTT Broker]
    end

    subgraph Web["User Interface (Monitoring)"]
        I[Web Dashboard overview.html, dashboard.html]
        J[Client-Side CSV Export]
    end

    A -- "Digital I2S (24-bit)" --> B
    B --> C
    C --> D
    D --> E
    E -- "Every 500ms" --> F
    E -- "Every 3s" --> G
    G -- "Pub/Sub" --> H
    H --> I
    I --> J
```
### INMP441 I2S Mems Microphone Pinout Configuration
<p align="center">
    <img src="https://github.com/user-attachments/assets/72507554-423b-49eb-84ca-8a3a8c95f720" width="80%" alt="INMP441 Pinout" />
    <br>
    <em>INMP441 Pinout</em>
    <br>
    <em>source: https://docs.cirkitdesigner.com/component/f5a2b2c1-1830-47a0-bebd-4ccef6a7babd/inmp441-front-mic</em>
</p>

The system operates in **I2S Standard Mode (Philips)**, allowing for a direct 24-bit digital stream from the MEMS sensor to the ESP32's internal DMA buffer.

| INMP441 Pin | ESP32 Pin | Function | Description |
| :--- | :--- | :--- | :--- |
| **VDD** | 3V3 | Power | Power supply (1.62V - 3.63V) |
| **GND** | GND | Ground | Common system ground |
| **L/R** | GND | Channel Select | Pulled to GND for Left Channel acquisition |
| **WS** | GPIO 25 | Word Select | I2S Word/Slot select line |
| **SCK** | GPIO 26 | Bit Clock | I2S Serial Clock line |
| **SD** | GPIO 33 | Serial Data | I2S Digital Data output |
> **Note:** The **MCLK (Master Clock)** line is not required for this implementation, as the INMP441 generates its internal timing from the SCK line.



### Firmware Logics/DSP Pipeline
```mermaid
flowchart TD
    Start([Start]) --> Init[Initialize I2S, WiFi, MQTT, & LCD]
    Init --> ReadBuffer[Read 32-bit I2S Buffer]
    ReadBuffer --> BitShift[Extract 24-bit Valid Data]
    
    subgraph DSP["DSP Pipeline (Per Sample)"]
        BitShift --> DCRemoval[DC Offset Removal - IIR Filter]
        DCRemoval --> AWeight[A-Weighting - 2nd Order Biquad]
        AWeight --> RMS[RMS Calculation - 125ms Window]
        RMS --> dBFS[dBFS to dBA Scale Conversion]
        dBFS --> EMA[Smoothing - Exponential Moving Average]
    end

    EMA --> CheckLCD{500ms Interval?}
    CheckLCD -- Yes --> UpdateLCD[Update Local I2C LCD]
    CheckLCD -- No --> CheckMQTT{3s Interval?}
    
    UpdateLCD --> CheckMQTT
    
    CheckMQTT -- Yes --> Publish[Publish JSON Payload to HiveMQ Cloud]
    CheckMQTT -- No --> ReadBuffer
    
    Publish --> ReadBuffer
```
