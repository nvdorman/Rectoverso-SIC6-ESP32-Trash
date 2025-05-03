#include "esp_camera.h"
#include <WiFi.h>
#include <PubSubClient.h>

// ========== MODEL PINOUT ==========
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// ========== WiFi Credentials ==========
const char *ssid = "Xiaomi 13T";         // Ganti dengan SSID kamu
const char *password = "";  // Ganti dengan password WiFi

// ========== MQTT Broker ==========
const char* mqtt_server = "broker.emqx.io";
const char* mqtt_sub_topic = "Rectoverso/rpl2/sic6/stage4/trigger/capture";

WiFiClient espClient;
PubSubClient client(espClient);

bool captureNow = false;

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (message == "true") {
    Serial.println("📸 Menerima Trigger MQTT → Siap ambil gambar");
    captureNow = true;
  }
}

void connectMQTT() {
  client.setServer(mqtt_server, 1883);
  while (!client.connect("ESP32Cam_Rectoverso")) {
    delay(1000);
    Serial.print(".");
  }
  client.subscribe(mqtt_sub_topic);
  client.setCallback(callback);
  Serial.println("\n✅ MQTT Connected");
}

void startCameraServer();
void setupLedFlash(int pin);

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 10;
  config.fb_count = 1;

  // Inisialisasi kamera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  s->set_brightness(s, -2);
  s->set_contrast(s, -2);
  s->set_saturation(s, -2);
  s->set_special_effect(s, 0);
  s->set_whitebal(s, 1);
  s->set_awb_gain(s, 1);
  s->set_wb_mode(s, 0);
  s->set_exposure_ctrl(s, 1);
  s->set_aec2(s, 0);
  s->set_ae_level(s, -2);
  s->set_aec_value(s, 300);
  s->set_gain_ctrl(s, 1);
  s->set_agc_gain(s, 0);
  s->set_gainceiling(s, GAINCEILING_2X);
  s->set_bpc(s, 0);
  s->set_wpc(s, 1);
  s->set_raw_gma(s, 1);
  s->set_lenc(s, 1);
  s->set_hmirror(s, 0);
  s->set_vflip(s, 0);
  s->set_dcw(s, 1);
  s->set_colorbar(s, 0);

  // Connect ke WiFi
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  Serial.print("🔌 Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");

  // Setup MQTT
  client.setServer(mqtt_server, 1883);
  connectMQTT();

  // Mulai Web Server
  startCameraServer();
  Serial.print("🌐 Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("/capture' to access the image");
}

void loop() {
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();
  delay(10);
}