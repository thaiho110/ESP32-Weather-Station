#include "DFRobot_EnvironmentalSensor.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#define MODESWITCH 1

#define ESP32_RX_PIN D3
#define ESP32_TX_PIN D2

const char* ssid = "WIFI Name";
const char* password = "WIFI Password";
const char* mqtt_server = "HiveMQ Server Address";
const int   mqtt_port = 8883; // Default by HiveMQ
const char* mqtt_user = "HiveMQ User";
const char* mqtt_pass = "HiveMQ User Password";
const char* mqtt_topic = "HiveMQ Topic";

WiFiClientSecure espClient;
PubSubClient client(espClient);
DFRobot_EnvironmentalSensor environment(/*addr =*/0x22, /*s =*/&Serial1);

unsigned long lastMsg = 0;
const int MSG_PUBLISH_INTERVAL = 30000;

void setup_wifi()
{
  delay(10);
  Serial.print("\nConnecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("XIAO-ESP32S3-Client", mqtt_user, mqtt_pass)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);

  Serial1.begin(9600, SERIAL_8N1, ESP32_RX_PIN, ESP32_TX_PIN);

  while(environment.begin() != 0)
  {
    Serial.println("Sensor initialization failed! Check wiring and switch position.");
    delay(1000);
  }
  Serial.println("Sensor initialized successfully!");
  setup_wifi();
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
}

void loop()
{
  if (!client.connected()) {
    reconnect_mqtt();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > MSG_PUBLISH_INTERVAL) {
    lastMsg = now;

    float temp = environment.getTemperature(TEMP_C);
    float humidity = environment.getHumidity();
    float uv = environment.getUltravioletIntensity();
    float light = environment.getLuminousIntensity();
    uint16_t pressure = environment.getAtmospherePressure(HPA);
    float altitude = environment.getElevation();

    
    // Format and publish data
    char json_payload[200]; 
    snprintf(json_payload, 200, 
      "{\"temperature\":%.2f, \"humidity\":%.2f, \"uv_intensity\":%.2f, \"light_intensity\":%.2f, \"pressure\":%u, \"altitude\":%.2f}", 
      temp, humidity, uv, light, pressure, altitude);
    
    Serial.print("Publishing message: ");
    Serial.println(json_payload);
    client.publish(mqtt_topic, json_payload);
  }
}
