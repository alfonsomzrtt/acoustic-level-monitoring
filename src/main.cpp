// #include <Arduino.h>
// #include <driver/i2s.h>
// #include <math.h>
// #include <LiquidCrystal_I2C.h>
// #include <Wire.h>
// #include <WiFi.h>
// // #include <PubSubClient.h>

// /*===================================
// WiFi Configuration
// ===================================*/

// const char* ssid = "Juanda_airport"; 
// const char* password = "juandahebat";


// // #define BASELINE_WINDOW 300

// // float splHistory[BASELINE_WINDOW];
// // int historyIndex = 0;
// // bool historyFilled = false;
// // bool baselineReady = false;

// // float noise_baseline = -120.0f;
// // unsigned long lastBaselineUpdate = 0;

// /* ===============================
//   A-WEIGHTING CONFIG
// ================================ */
// typedef struct
// {
//   float b0, b1, b2;
//   float a1, a2;
//   float z1, z2;
// } Biquad;

// // Berikut koefisien yang sudah diturunkan untuk 16 kHz (
// // Ini pendekatan 2nd-order A-weight approximation.)):
// Biquad aweight = {
//     0.2557411f, -0.5114822f, 0.2557411f,
//     -0.6476948f, 0.1428700f,
//     0.0f, 0.0f};

// // Fungsi Proses Biquad
// float processBiquad(Biquad *f, float x)
// {

//   float y = f->b0 * x + f->z1;
//   f->z1 = f->b1 * x - f->a1 * y + f->z2;
//   f->z2 = f->b2 * x - f->a2 * y;

//   return y;
// }

// /* ===============================
//    LCD CONFIG
// ================================ */
// LiquidCrystal_I2C lcd(0x27, 16, 2); // Ganti 0x27 jika perlu
// unsigned long lastLcdUpdate = 0;
// #define LCD_UPDATE_INTERVAL 500 // 500 ms

// /* ===============================
//    I2S CONFIG
// ================================ */
// #define I2S_PORT I2S_NUM_0

// #define I2S_WS 25
// #define I2S_SCK 26
// #define I2S_SD 33

// #define SAMPLE_RATE 16000
// #define DMA_BUF_LEN 256
// #define DMA_BUF_CNT 6

// int32_t i2sBuffer[DMA_BUF_LEN];

// /* ===============================
//    DSP CONFIG
// ================================ */
// #define RMS_WINDOW_SAMPLES 2000 // 125 ms @16kHz

// // DEFINE K_cal
// #define SPL_CAL_OFFSET 119.0f

// // int64_t sumsq = 0;
// /*Karena sekarang kita sudah di domain float (weighted adalah float),
// lebih bersih kalau kita ubah sumsq menjadi double. */
// double sumsq = 0.0;
// int rmsCount = 0;

// float rmsValue = 0;
// float level_dBFS = -120.0f;;

// // smoothing spl
// float spl_smooth = 0.0f;
// bool spl_initialized = false;

// // DC removal
// int32_t dc_estimate = 0;
// #define DC_ALPHA_SHIFT 10 // ~1–2 Hz HPF

// // bool noiseCaptured = false;

// /* ===============================
//    I2S SETUP
// ================================ */
// void i2s_install()
// {
//   const i2s_config_t i2s_config = {
//       .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
//       .sample_rate = SAMPLE_RATE,
//       .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
//       .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
//       .communication_format = I2S_COMM_FORMAT_I2S_MSB,
//       .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
//       .dma_buf_count = DMA_BUF_CNT,
//       .dma_buf_len = DMA_BUF_LEN,
//       .use_apll = false};

//   i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
// }

// void i2s_setpin()
// {
//   const i2s_pin_config_t pin_config = {
//       .bck_io_num = I2S_SCK,
//       .ws_io_num = I2S_WS,
//       .data_out_num = I2S_PIN_NO_CHANGE,
//       .data_in_num = I2S_SD};

//   i2s_set_pin(I2S_PORT, &pin_config);
// }

// /* ===============================
//    DSP CORE (FINAL + SPL SMOOTHING)
// ================================ */
// void processSample(int32_t sample32)
// {

//   // 1️. Ambil 24-bit valid
//   int32_t sample24 = sample32 >> 8;

//   // 2️. DC removal (fixed-point IIR)
//   dc_estimate += (sample24 - dc_estimate) >> DC_ALPHA_SHIFT;
//   int32_t sample = sample24 - dc_estimate;

//   // diubah/convert ke float untuk filter A-weighting, sekaligus dinormalisasi
//   float sample_f = (float)sample / 8388608.0f;
//   // Apply A-Weighting
//   float weighted = processBiquad(&aweight, sample_f);

//   // 3️. Accumulate RMS energy
//   // sumsq += (int64_t)sample * sample;
//   // diubah ke
//   sumsq += (double)(weighted * weighted); // Sekarang RMS dihitung dari sinyal yang sudah di-A-weight.
//   /*Tanpa cast ke int64 lagi.
// Karena:

// 1. Sudah di floating domain
// 2. Filter IIR bekerja di float
// 3. Lebih konsisten dan presisi
// 4. ESP32 punya FPU harusnya aman.*/

//   rmsCount++;

//   if (rmsCount >= RMS_WINDOW_SAMPLES)
//   {

//     // 4️. RMS
//     rmsValue = sqrt((float)sumsq / rmsCount);
//     if (rmsValue < 1e-9f)
//       rmsValue = 1e-9f; /*Tujuannya hanya mencegah
// log10(0), bukan meng-clamp RMS ke 1.*/

//     // 5️. RMS → dBFS
//     level_dBFS = 20.0f * log10(rmsValue);

//     // 6. SPL (raw, hasil kalibrasi)
//     float spl_raw = level_dBFS + SPL_CAL_OFFSET;

//     // 7. SPL smoothing (EMA)
//     if (!spl_initialized)
//     {
//       spl_smooth = spl_raw;
//       spl_initialized = true;
//     }
//     else
//     {
//       spl_smooth = 0.8f * spl_smooth + 0.2f * spl_raw;
//     }

//     //  9. Rolling Window untuk Baseline Update
//     //   ===============================
//     //   Rolling Baseline Update (1 sec)
//     //   ===============================
//     // if (millis() - lastBaselineUpdate > 1000)
//     // {

//     //   lastBaselineUpdate = millis();

//     //   splHistory[historyIndex] = spl_smooth;
//     //   historyIndex++;

//     //   if (historyIndex >= BASELINE_WINDOW)
//     //   {
//     //     historyIndex = 0;
//     //     historyFilled = true;
//     //   }

//     //   if (historyFilled)
//     //   {
//     //     baselineReady = true;
//     //   }

//     //   float minVal = 999.0f;
//     //   int count = historyFilled ? BASELINE_WINDOW : historyIndex;

//     //   for (int i = 0; i < count; i++)
//     //   {
//     //     if (splHistory[i] < minVal)
//     //     {
//     //       minVal = splHistory[i];
//     //     }
//     //   }

//     //   noise_baseline = minVal;
//     // }

//     // // 10. Hitung SNR SETELAH baseline update
//     // // if (historyIndex > 0 || historyFilled) {
//     // if (baselineReady)
//     // {
//     //   snr_dB = spl_smooth - noise_baseline;
//     // }
//     // else
//     // {
//     //   snr_dB = 0.0f;
//     // }



//     // ==========================
//     // LCD UPDATE
//     // ==========================
//     if (millis() - lastLcdUpdate > LCD_UPDATE_INTERVAL)
//     {

//       lastLcdUpdate = millis();

//       lcd.setCursor(0, 0);
//       lcd.print("SPL:");
//       lcd.setCursor(5, 0);
//       lcd.print("        "); // clear area, 8 spasi, just in case kalau SPL naik ke 100+ dBA, which is unlikely to happen
//       lcd.setCursor(5, 0);
//       lcd.print(spl_smooth, 1);
//       lcd.print("dBA");

//       // lcd.setCursor(0, 1);
//       // lcd.print("SNR:");
//       // lcd.setCursor(5, 1);
//       // lcd.print("        "); // clear area

//       // if (!baselineReady)
//       // {
//       //   lcd.setCursor(5, 1);
//       //   lcd.print("WARMUP");
//       // }
//       // else
//       // {
//       //   lcd.setCursor(5, 1);
//       //   lcd.print(snr_dB, 1);
//       //   lcd.print("dB");
//       // }
//     }

//     // 10. Reset window
//     sumsq = 0;
//     rmsCount = 0;

//     // 1️1. Output CSV ke Serial Plotter
//     Serial.print("dBFS:");
//     Serial.print(level_dBFS,1);
//     Serial.print(" SPL:");
//     Serial.println(spl_smooth,1);
//   }
// }

// /* ===============================
//    SETUP & LOOP
// ================================ */
// void setup()
// {
//   Serial.begin(115200);
//   Wire.begin(21, 22); // SDA, SCL
//   lcd.init();
//   lcd.backlight();
//   //  lcd.clear();

//   lcd.setCursor(0, 0);
//   lcd.print("Connecting to ");
//   lcd.setCursor(0, 1);
//   lcd.print(ssid);

//   delay(2000);
//   WiFi.mode(WIFI_STA);
//   // delay(100);
//   WiFi.setSleep(false);
//   WiFi.begin(ssid, password);

//   unsigned long startAttempt = millis();

// //Menunggu hingga terhubung  
//   while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) { 
//     delay(500);

//     lcd.setCursor(15,1); 
//     lcd.print(".");
//   }

//   lcd.clear();

//   if (WiFi.status() == WL_CONNECTED) {
//   lcd.setCursor(0,0);
//   lcd.print("WiFi terhubung.");

//   lcd.setCursor(0,1);
//   lcd.print("Alamat IP: ");
  
//   delay(2000);
//   lcd.clear();
//   lcd.setCursor(0,0);
//   lcd.print(WiFi.localIP());

//   delay(2000);
//   } else {
//     lcd.setCursor(0,0);
//     lcd.println("WiFi gagal tersambung");
//     delay(2000);
//   }

//   lcd.clear();
//   lcd.setCursor(0, 0);
//   lcd.print("SPL monitoring..");
//   lcd.setCursor(0, 1);
//   lcd.print("Initializing...");
//   delay(1500);
//   lcd.clear();


//   i2s_install();
//   i2s_setpin();
//   i2s_start(I2S_PORT);
// }

// void loop()
// {
//   size_t bytesIn = 0;

//   i2s_read(
//       I2S_PORT,
//       i2sBuffer,
//       DMA_BUF_LEN * sizeof(int32_t),
//       &bytesIn,
//       portMAX_DELAY);

//   int samples = bytesIn / sizeof(int32_t);

//   for (int i = 0; i < samples; i++)
//   {
//     processSample(i2sBuffer[i]);
//   }
// }


//yang diatas itu kode lama, untuk jaga-jaga, karena kodenya stabil


//di bawah ini kode refactor dari AI + revisi dari saya sendiri
// === ONLY MQTT ADDED, NO OTHER LOGIC TOUCHED ===

#include <Arduino.h>
#include <driver/i2s.h>
#include <math.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <WiFi.h>

#include <WiFiClientSecure.h>
#include <PubSubClient.h>

/*===================================
WiFi Configuration
===================================*/

const char* ssid = "Juanda_airport"; 
const char* password = "juandahebat";

const char* mqtt_server = "b8ae4809915f4027b2d18c7fc219b204.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
// const char* mqtt_topic = "spl/data";

WiFiClientSecure espClient;
PubSubClient client(espClient);

// ===============================
// NODE CONFIG
// ===============================
#define NODE_ID "GATE2"   // <-- GANTI tiap device
String topic_pub = "monitoring/" + String(NODE_ID) + "/db";
unsigned long lastReconnectAttempt = 0;

unsigned long lastPublish = 0;
#define PUBLISH_INTERVAL 5000 // ubah ke berapa detik rate stream data yang diinginkan

void reconnectMQTT() {
  if (client.connected()) return;

  if (millis() - lastReconnectAttempt < 3000) return;
  lastReconnectAttempt = millis();

  Serial.print("MQTT connecting...");
  String clientId = "ESP32_" + String(NODE_ID) + "_" + String(random(1000));

  if (client.connect(clientId.c_str(), "esp32-v1", "RajaSawit_2026")) {
    Serial.println("connected");
  } else {
    Serial.print("failed rc=");
    Serial.println(client.state());
  }
}

/* ===============================
  A-WEIGHTING CONFIG
================================ */
typedef struct
{
  float b0, b1, b2;
  float a1, a2;
  float z1, z2;
} Biquad;

// Berikut koefisien yang sudah diturunkan untuk 16 kHz (
// Ini pendekatan 2nd-order A-weight approximation.)):
Biquad aweight = {
    0.2557411f, -0.5114822f, 0.2557411f,
    -0.6476948f, 0.1428700f,
    0.0f, 0.0f};

// Fungsi Proses Biquad
float processBiquad(Biquad *f, float x)
{
  float y = f->b0 * x + f->z1;
  f->z1 = f->b1 * x - f->a1 * y + f->z2;
  f->z2 = f->b2 * x - f->a2 * y;
  return y;
}

/* ===============================
   LCD CONFIG
================================ */
LiquidCrystal_I2C lcd(0x27, 16, 2); // Ganti 0x27 jika perlu
unsigned long lastLcdUpdate = 0;
#define LCD_UPDATE_INTERVAL 500 // 500 ms

/* ===============================
   I2S CONFIG
================================ */
#define I2S_PORT I2S_NUM_0

#define I2S_WS 25
#define I2S_SCK 26
#define I2S_SD 33

#define SAMPLE_RATE 16000
#define DMA_BUF_LEN 256
#define DMA_BUF_CNT 6

int32_t i2sBuffer[DMA_BUF_LEN];

/* ===============================
   DSP CONFIG
================================ */
#define RMS_WINDOW_SAMPLES 2000 // 125 ms @16kHz

// DEFINE K_cal
#define SPL_CAL_OFFSET 119.0f

// int64_t sumsq = 0;
/*Karena sekarang kita sudah di domain float (weighted adalah float),
lebih bersih kalau kita ubah sumsq menjadi double. */
double sumsq = 0.0;
int rmsCount = 0;

float rmsValue = 0;
float level_dBFS = -120.0f;

// smoothing spl
float spl_smooth = 0.0f;
bool spl_initialized = false;

// DC removal
int32_t dc_estimate = 0;
#define DC_ALPHA_SHIFT 10

/* ===============================
   I2S SETUP
================================ */
void i2s_install()
{
  const i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = I2S_COMM_FORMAT_I2S_MSB,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = DMA_BUF_CNT,
      .dma_buf_len = DMA_BUF_LEN,
      .use_apll = false};

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin()
{
  const i2s_pin_config_t pin_config = {
      .bck_io_num = I2S_SCK,
      .ws_io_num = I2S_WS,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = I2S_SD};

  i2s_set_pin(I2S_PORT, &pin_config);
}

/* ===============================
   DSP CORE
================================ */
void processSample(int32_t sample32)
{
  // 1️. Ambil 24-bit valid
  int32_t sample24 = sample32 >> 8;

  // 2️. DC removal (fixed-point IIR)
  dc_estimate += (sample24 - dc_estimate) >> DC_ALPHA_SHIFT;
  int32_t sample = sample24 - dc_estimate;

  // diubah/convert ke float untuk filter A-weighting, sekaligus dinormalisasi
  float sample_f = (float)sample / 8388608.0f;
  // Apply A-Weighting
  float weighted = processBiquad(&aweight, sample_f);

  // 3️. Accumulate RMS energy
  // sumsq += (int64_t)sample * sample;
  // diubah ke
  sumsq += (double)(weighted * weighted); // Sekarang RMS dihitung dari sinyal yang sudah di-A-weight.
  /*Tanpa cast ke int64 lagi.
  Karena:

  1. Sudah di floating domain
  2. Filter IIR bekerja di float
  3. Lebih konsisten dan presisi
  4. ESP32 punya FPU harusnya aman.*/  
  rmsCount++;

  if (rmsCount >= RMS_WINDOW_SAMPLES)
  {

    // 4️. RMS   
    rmsValue = sqrt((float)sumsq / rmsCount);
    if (rmsValue < 1e-9f) 
     rmsValue = 1e-9f; /*Tujuannya hanya mencegah
  log10(0), bukan meng-clamp RMS ke 1.*/

    // 5️. RMS → dBFS
    level_dBFS = 20.0f * log10(rmsValue);

    // 6. SPL (raw, hasil kalibrasi)
    float spl_raw = level_dBFS + SPL_CAL_OFFSET;

    // 7. SPL smoothing (EMA)
    if (!spl_initialized)
    {
      spl_smooth = spl_raw;
      spl_initialized = true;
    }
    else
    {
      spl_smooth = 0.8f * spl_smooth + 0.2f * spl_raw;
    }

    // === MQTT PUBLISH (SAFE, NON BLOCKING) ===
    if (client.connected() && millis() - lastPublish > PUBLISH_INTERVAL) {
      lastPublish = millis();


      char payload[64];
      snprintf(payload, sizeof(payload), "{\"spl\": %.1f}", spl_smooth);
      client.publish(topic_pub.c_str(), payload);
    }

    // ==========================
    // LCD UPDATE
    // ==========================
    if (millis() - lastLcdUpdate > LCD_UPDATE_INTERVAL)
    {

      lastLcdUpdate = millis();

      lcd.setCursor(0, 0);
      lcd.print("SPL:");
      lcd.setCursor(5, 0);
      lcd.print("        ");  // clear area, 8 spasi, just in case kalau SPL naik ke 100+ dBA, which is unlikely to happen
      lcd.setCursor(5, 0);
      lcd.print(spl_smooth, 1);
      lcd.print("dBA");
    }

    // 10. Reset window
    sumsq = 0;
    rmsCount = 0;

    //11. Output CSV ke Serial Plotter
    Serial.print("dBFS:");
    Serial.print(level_dBFS,1);
    Serial.print(" SPL:");
    Serial.println(spl_smooth,1);
  }
}

/* ===============================
   SETUP & LOOP
================================ */
void setup()
{
  Serial.begin(115200);
  Wire.begin(21, 22); //SDA, SCL
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Connecting to ");
  lcd.setCursor(0, 1);
  lcd.print(ssid);

  delay(2000);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);

  unsigned long startAttempt = millis();

  //Menunggu hingga terhubung
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
    delay(500);
    lcd.setCursor(15,1);
    lcd.print(".");
  }

  lcd.clear();

  if (WiFi.status() == WL_CONNECTED) {
    lcd.setCursor(0,0);
    lcd.print("WiFi terhubung.");
    
    lcd.setCursor(0,1);
    lcd.print("Alamat IP: ");

    delay(2000);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(WiFi.localIP());

    delay(2000);
  } else {
    lcd.setCursor(0,0);
    lcd.print("WiFi gagal tersambung");
    delay(2000);
  }

  //bypass TLS cert
    espClient.setInsecure();

    // MQTT init (NON BLOCKING)
    client.setServer(mqtt_server, mqtt_port);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SPL monitoring..");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");
    delay(1500);
    lcd.clear();

    i2s_install();
    i2s_setpin();
    i2s_start(I2S_PORT);
}

void loop()
{
    // //WIFI reconnect ringan 
    // if (WiFi.status() != WL_CONNECTED) {
    //   WiFi.reconnect();
    //   return;
    // }    

    // MQTT loop ringan (TIDAK BLOCKING)
    if (WiFi.status() == WL_CONNECTED) {
        reconnectMQTT();
        client.loop();
    }    

    size_t bytesIn = 0;

    i2s_read(
        I2S_PORT,
        i2sBuffer,
        DMA_BUF_LEN * sizeof(int32_t),
        &bytesIn,
        portMAX_DELAY);

    int samples = bytesIn / sizeof(int32_t);

    for (int i = 0; i < samples; i++)
    {
        processSample(i2sBuffer[i]);
  }
}
