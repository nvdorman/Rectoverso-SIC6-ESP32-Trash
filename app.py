import streamlit as st
import cv2
import numpy as np
import paho.mqtt.client as mqtt
from PIL import Image
import requests
import time
from tensorflow.keras.models import load_model

# ========== MQTT ==========
MQTT_BROKER = "broker.emqx.io"
MQTT_TOPIC_TRIGGER = "Rectoverso/rpl2/sic6/stage4/trigger/capture"
MQTT_TOPIC_RESULT = "Rectoverso/rpl2/sic6/stage4/classification/result"

# ========== IP CAM ==========
CAMERA_URL = "http://192.168.76.234/capture"  # Ganti dengan IP ESP32-CAM saat terhubung ke WiFi

# ========== MODEL KLASSIFIKASI ==========
@st.cache_resource
def load_classifier():
    return load_model('model_Rectoverso_best.h5')

clf_model = load_classifier()

def preprocess(img_cv):
    img_resized = cv2.resize(img_cv, (150, 150))
    # Convert to RGB instead of grayscale since the model expects 3 channels
    return cv2.cvtColor(img_resized, cv2.COLOR_BGR2RGB) / 255.0

def classify(img_cv):
    pred = clf_model.predict(np.expand_dims(preprocess(img_cv), axis=0), verbose=0)
    return 'Organik' if pred[0][0] < 0.47 else 'Anorganik'

# ========== MQTT CLIENT ==========
mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION1, "streamlit_ai")
mqtt_client.connect(MQTT_BROKER, 1883)
mqtt_client.loop_start()

# ========== STREAMLIT UI ==========
st.title("📷 Rectoverso - RPL2 | Kelompok Sic6 Stage4")

frame_placeholder = st.empty()
result_placeholder = st.empty()
label_placeholder = st.empty()

latest_frame = None

def on_message(client, userdata, msg):
    global latest_frame
    if msg.topic == MQTT_TOPIC_TRIGGER and msg.payload.decode() == "true":
        try:
            response = requests.get(CAMERA_URL, timeout=10)
            if response.status_code == 200:
                img_array = np.asarray(bytearray(response.content), dtype=np.uint8)
                frame = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
                latest_frame = frame
        except Exception as e:
            print(f"❌ Gagal ambil gambar dari kamera: {e}")

mqtt_client.subscribe(MQTT_TOPIC_TRIGGER)
mqtt_client.on_message = on_message

while True:
    if latest_frame is not None:
        frame_rgb = cv2.cvtColor(latest_frame, cv2.COLOR_BGR2RGB)
        pil_img = Image.fromarray(frame_rgb)
        frame_placeholder.image(pil_img, caption="📸 Gambar Masuk", use_container_width=True)

        label = classify(latest_frame)
        result_placeholder.success(f"🏷 Hasil: {label}")
        label_placeholder.info(f"🔄 Mengirim hasil ke MQTT...")

        # Kirim hasil ke MQTT
        mqtt_client.publish(MQTT_TOPIC_RESULT, label)

        latest_frame = None

    time.sleep(0.1)