#include <ESP32Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ========== WiFi ==========
const char* ssid = "Xiaomi 13T";
const char* password = "";

// ========== MQTT ==========
const char* mqtt_server = "broker.emqx.io";
const char* mqtt_sub_topic = "Rectoverso/rpl2/sic6/stage4/classification/result";
const char* mqtt_pub_topic = "Rectoverso/rpl2/sic6/stage4/trigger/capture";

WiFiClient espClient;
PubSubClient client(espClient);

// ========== ULTRASONIK ==========
#define TRIG_PIN 5
#define ECHO_PIN 4

// ========== SERVO ==========
#define SERVO2_PIN 21
Servo servo2;

bool bendaTerdeteksi = false;

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("📩 Pesan MQTT diterima: ");
  Serial.println(message);

  if (message == "Organik") {
    Serial.println("🟢 Organik → Gerakkan ke kiri (0°)");
    servo2.write(90); delay(1000);
    servo2.write(0);  delay(500);
  } else if (message == "Anorganik") {
    Serial.println("🔴 Anorganik → Gerakkan ke kanan (180°)");
    servo2.write(90); delay(1000);
    servo2.write(0);  delay(500);
  }
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("🔌 Reconnecting to MQTT...");
    if (client.connect("ESP32_NoServo1_Rectoverso")) {
      Serial.println("connected");
      client.subscribe(mqtt_sub_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup_wifi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected");
}

long getDistance() {
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duration = pulseIn(ECHO_PIN, HIGH, 1000000); // timeout 1s
  return duration * 0.034 / 2;
}

void setup() {
  Serial.begin(115200);
  Serial.println("🚀 Mulai setup...");

  // Ultrasonik
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  // Servo
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);

  servo2.setPeriodHertz(50);
  servo2.attach(SERVO2_PIN, 500, 2500);
  servo2.write(0);   // Awal tutup
  delay(1000);

  // WiFi + MQTT
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  reconnectMQTT();
}

void loop() {
  long distance = getDistance();

  if (distance > 0 && distance < 10 && !bendaTerdeteksi) {
    Serial.println("📦 Benda Terdeteksi → Kirim trigger ke CAM");
    bendaTerdeteksi = true;
    client.publish(mqtt_pub_topic, "true");
    delay(1000);
  } else if (distance > 10) {
    bendaTerdeteksi = false;
  }

  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  delay(100);
}
