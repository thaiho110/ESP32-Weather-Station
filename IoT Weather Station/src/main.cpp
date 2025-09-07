#include <Arduino.h>
#include "DFRobot_EnvironmentalSensor.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <Wire.h>

// --- DEEP SLEEP CONFIGURATION ---
// Set the time in seconds for the device to sleep between readings.
// 300 seconds = 5 minutes.
const int SLEEP_INTERVAL_SECONDS = 300;

const char* mqtt_server = MQTT_SERVER;
const int   mqtt_port = 8883;
const char* mqtt_user = MQTT_USER;
const char* mqtt_pass = MQTT_PASS;
const char* mqtt_topic = "myhome/weatherstation/data";

WiFiClientSecure espClient;
PubSubClient client(espClient);
DFRobot_EnvironmentalSensor environment(/*addr =*/0x22);


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- Waking up! ---");

  Wire.begin(D4, D5);
  
  WiFiManager wm;

  wm.setConfigPortalTimeout(180);


  if (!wm.autoConnect("WeatherStation-Setup", WIFI_PASS)) {
    Serial.println("Failed to connect to WiFi and hit timeout. Rebooting to restart the portal...");
    delay(3000);
    ESP.restart();
  }
  
  Serial.println("WiFi connected successfully!");


  if(environment.begin() != 0) {
    Serial.println("Sensor initialization failed! Going to sleep.");
    ESP.deepSleep(SLEEP_INTERVAL_SECONDS * 1000000);
  }
  Serial.println("Sensor initialized successfully!");

  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);

  Serial.print("Attempting MQTT connection...");
  if (client.connect("XIAO-ESP32S3-Client", mqtt_user, mqtt_pass)) {
    Serial.println("connected.");

    // Read Sensor Data
    float temp = environment.getTemperature(TEMP_C);
    float humidity = environment.getHumidity();
    float uv = environment.getUltravioletIntensity();
    float light = environment.getLuminousIntensity();
    uint16_t pressure = environment.getAtmospherePressure(HPA);
    float altitude = environment.getElevation();

    // Format and Publish Data
    char json_payload[200]; 
    snprintf(json_payload, 200, 
      "{\"temperature\":%.2f, \"humidity\":%.2f, \"uv_intensity\":%.2f, \"light_intensity\":%.2f, \"pressure\":%u, \"altitude\":%.2f}", 
      temp, humidity, uv, light, pressure, altitude);
    
    Serial.print("Publishing message: ");
    Serial.println(json_payload);
    client.publish(mqtt_topic, json_payload);


    delay(500);
    client.disconnect();
    Serial.println("MQTT message sent and disconnected.");

  } else {
    Serial.print("failed, rc=");
    Serial.println(client.state());

  }

  Serial.printf("All tasks complete. Going to sleep for %d seconds.\n\n", SLEEP_INTERVAL_SECONDS);
  ESP.deepSleep(SLEEP_INTERVAL_SECONDS * 1000000);
}


void loop() {

}