#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ========== WiFi & MQTT ==========
const char* ssid = "Xiaomi 13T";
const char* password = "";
const char* mqtt_server = "broker.emqx.io";
const char* mqtt_sub_topic = "Rectoverso/rpl2/sic6/stage4/classification/result";

WiFiClient espClient;
PubSubClient client(espClient);

// ========== SERVO ==========
#define SERVO3_PIN 21
Servo servo3;

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (message == "Organik") {
    Serial.println("🟢 Organik → Arah Tong 45°");
    servo3.write(45);
    delay(2000);  // Increased delay to 2 seconds
    servo3.write(90); // Reset ke tengah
    delay(1000);  // Added delay after reset
  } else if (message == "Anorganik") {
    Serial.println("🔴 Anorganik → Arah Tong 135°");
    servo3.write(135);
    delay(2000);  // Increased delay to 2 seconds
    servo3.write(90); // Reset ke tengah
    delay(1000);  // Added delay after reset
  }
}

void reconnectMQTT() {
  while (!client.connect("ESP32Servo3_Rectoverso")) {
    delay(1000);
    Serial.print(".");
  }
  client.subscribe(mqtt_sub_topic);
  Serial.println("\n✅ MQTT Connected");
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting setup...");

  // Setup Servo first
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  servo3.setPeriodHertz(50);  // Standard 50hz servo
  servo3.attach(SERVO3_PIN, 500, 2500);
  servo3.write(90);  // Neutral di tengah
  delay(2000);  // Increased initial delay
  Serial.println("Servo initialized");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");

  // Setup MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  reconnectMQTT();
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  delay(100);
}