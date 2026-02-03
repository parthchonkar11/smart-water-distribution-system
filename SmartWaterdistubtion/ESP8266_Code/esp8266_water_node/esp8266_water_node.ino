#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// ===== WIFI SETTINGS =====
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// ===== MQTT SETTINGS =====
const char* mqtt_server = "YOUR_LAPTOP_IP";   // e.g. 192.168.1.10

WiFiClient espClient;
PubSubClient client(espClient);

// ===== PIN DEFINITIONS =====
#define FLOW_PIN D5
#define RELAY_PIN D1
#define PRESSURE_PIN A0

volatile int pulseCount = 0;
float flowRate = 0;
unsigned long lastFlowCalc = 0;

// ===== FLOW INTERRUPT =====
void ICACHE_RAM_ATTR pulseCounter() {
  pulseCount++;
}

// ===== MQTT CALLBACK =====
void callback(char* topic, byte* payload, unsigned int length) {

  String command = "";
  for (int i = 0; i < length; i++) {
    command += (char)payload[i];
  }

  if (command == "CLOSE") {
    digitalWrite(RELAY_PIN, HIGH);
  }
}

// ===== WIFI CONNECT =====
void setup_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

// ===== MQTT RECONNECT =====
void reconnect() {
  while (!client.connected()) {
    client.connect(("ESP8266_" + String(ESP.getChipId())).c_str());
    client.subscribe("water/zone1/valve");
  }
}

void setup() {

  Serial.begin(115200);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  pinMode(FLOW_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), pulseCounter, FALLING);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) reconnect();
  client.loop();

  if (millis() - lastFlowCalc > 1000) {
    flowRate = pulseCount / 7.5;
    pulseCount = 0;
    lastFlowCalc = millis();
  }

  int raw = analogRead(PRESSURE_PIN);
  float voltage = (raw / 1023.0) * 1.0;     // NodeMCU safe
  float pressureBar = voltage * 10.0;

  String payload = "{";
  payload += "\"flow\":" + String(flowRate) + ",";
  payload += "\"pressure\":" + String(pressureBar);
  payload += "}";

  client.publish("water/zone1/data", payload.c_str());
  delay(2000);
}
