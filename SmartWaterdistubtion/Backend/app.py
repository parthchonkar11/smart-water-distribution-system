from flask import Flask, jsonify, render_template
from flask_cors import CORS
import paho.mqtt.client as mqtt
import json, time

app = Flask(__name__)
CORS(app)

latest_data = {
    "flow": 0,
    "pressure": 0,
    "status": "Normal",
    "zone": "Zone 1",
    "timestamp": ""
}

MQTT_BROKER = "localhost"
MQTT_PORT = 1883
MQTT_TOPIC_DATA = "water/zone1/data"
MQTT_TOPIC_VALVE = "water/zone1/valve"
PRESSURE_THRESHOLD = 1.2

def on_message(client, userdata, msg):
    global latest_data
    payload = json.loads(msg.payload.decode())
    pressure = float(payload["pressure"])
    flow = float(payload["flow"])

    latest_data["flow"] = flow
    latest_data["pressure"] = pressure
    latest_data["timestamp"] = time.strftime("%H:%M:%S")

    if pressure < PRESSURE_THRESHOLD:
        latest_data["status"] = "Leak Detected"
        client.publish(MQTT_TOPIC_VALVE, "CLOSE")
    else:
        latest_data["status"] = "Normal"

mqtt_client = mqtt.Client()
mqtt_client.on_message = on_message
mqtt_client.connect(MQTT_BROKER, MQTT_PORT)
mqtt_client.subscribe(MQTT_TOPIC_DATA)
mqtt_client.loop_start()

@app.route("/")
def home():
    return render_template("index.html")

@app.route("/data")
def data():
    return jsonify(latest_data)

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)
