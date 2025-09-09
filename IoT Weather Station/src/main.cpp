#include <Arduino.h>
#include "DFRobot_EnvironmentalSensor.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFiManager.h>
#include <Wire.h>

// --- CONFIGURATION ---
const int SLEEP_INTERVAL_SECONDS = 300;

// Time Zone Configuration for Ho Chi Minh (GMT+7)
const long gmtOffset_sec = 7 * 3600;
const int daylightOffset_sec = 0;

// --- MQTT Configuration ---
const char* mqtt_server = MQTT_SERVER;
const int   mqtt_port = 8883;
const char* mqtt_user = MQTT_USER;
const char* mqtt_pass = MQTT_PASS;
const char* mqtt_topic = "myhome/weatherstation/data";

WiFiClientSecure espClient;
PubSubClient client(espClient);
DFRobot_EnvironmentalSensor environment(/*addr =*/0x22);

void tokenStatusCallback(TokenInfo info);

String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) { return "N/A"; }
  char timeStringBuff[50];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(timeStringBuff);
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.print("Day of week: ");
  Serial.println(&timeinfo, "%A");
  Serial.print("Month: ");
  Serial.println(&timeinfo, "%B");
  Serial.print("Day of Month: ");
  Serial.println(&timeinfo, "%d");
  Serial.print("Year: ");
  Serial.println(&timeinfo, "%Y");
  Serial.print("Hour: ");
  Serial.println(&timeinfo, "%H");
  Serial.print("Hour (12 hour format): ");
  Serial.println(&timeinfo, "%I");
  Serial.print("Minute: ");
  Serial.println(&timeinfo, "%M");
  Serial.print("Second: ");
  Serial.println(&timeinfo, "%S");

  Serial.println("Time variables");
  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  Serial.println(timeHour);
  char timeWeekDay[10];
  strftime(timeWeekDay,10, "%A", &timeinfo);
  Serial.println(timeWeekDay);
  Serial.println();
}


void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- Waking up! ---");

  Wire.begin(D4, D5);

  WiFiManager wm;

  wm.setConfigPortalTimeout(180);

  if (!wm.autoConnect("WeatherStation-Setup", WIFI_PASS)) {
    Serial.println("Failed to connect to WiFi and hit timeout.");
    Serial.println("Rebooting to restart the portal...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("\nWiFi connected successfully!");

  // --- UPDATED: Use multiple NTP servers for better reliability ---
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "1.ntp.vnix.vn");

  Serial.print("Waiting for time sync");
  struct tm timeinfo;
  unsigned long startMillis = millis();
  bool timeSynced = false;

  while (millis() - startMillis < 30000) {
    if (getLocalTime(&timeinfo)) {
      Serial.println("\nTime synced successfully!");
      timeSynced = true;
      break;
    }
    delay(500);
    Serial.print(".");
  }

  if (!timeSynced) {
    Serial.println("\nTime sync FAILED after 30 seconds. Going to sleep.");
    ESP.deepSleep(SLEEP_INTERVAL_SECONDS * 1000000); // Go to sleep if time fails
  }

  if(environment.begin() != 0)
  {
    Serial.println("Sensor initialization failed! Going to sleep.");
    ESP.deepSleep(SLEEP_INTERVAL_SECONDS * 1000000);
  }

  float temp = environment.getTemperature(TEMP_C);
  float humidity = environment.getHumidity();
  float uv = environment.getUltravioletIntensity();
  float light = environment.getLuminousIntensity();
  uint16_t pressure = environment.getAtmospherePressure(HPA);
  float altitude = environment.getElevation();

  Serial.println("Sensor initialized successfully!");
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);

  Serial.print("Attempting MQTT connection...");
  if (client.connect("XIAO-ESP32S3-Client", mqtt_user, mqtt_pass)) {
    Serial.println("connected.");

  // Format and publish data
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
  }
  else 
  {
    Serial.print("failed, rc=");
    Serial.println(client.state());
  }

  GSheet.setTokenCallback(tokenStatusCallback);
  GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);

  if (timeSynced)
  {
    if (GSheet.ready()) {
      FirebaseJson valueRange;
      String formattedTime = getFormattedTime();

      // Prepare the data payload in the format the library needs
      valueRange.add("majorDimension", "ROWS");
      valueRange.set("values/[0]/[0]", formattedTime);
      valueRange.set("values/[0]/[1]", temp);
      valueRange.set("values/[0]/[2]", humidity);
      valueRange.set("values/[0]/[3]", uv < 0 ? 0 : uv);
      valueRange.set("values/[0]/[4]", light);
      valueRange.set("values/[0]/[5]", pressure);
      valueRange.set("values/[0]/[6]", altitude);
      
      Serial.println("Appending values to spreadsheet...");
      FirebaseJson response;
      if (GSheet.values.append(&response, SPREADSHEET_ID, "Sheet1!A1", &valueRange)) {
        Serial.println("Data logged successfully to Google Sheets.");
      } else {
        Serial.println("Failed to log data to Google Sheets. Reason: " + GSheet.errorReason());
      }
    } else {
      Serial.println("Google Sheets authentication failed after 30 seconds. Skipping log.");
    }
  }

  Serial.printf("All tasks complete. Going to sleep for %d seconds.\n\n", SLEEP_INTERVAL_SECONDS);
  delay(3000);
  ESP.deepSleep(SLEEP_INTERVAL_SECONDS * 1000000);
}

void loop() {}

void tokenStatusCallback(TokenInfo info) {
  if (info.status == token_status_error) {
    GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    GSheet.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
  } else {
    GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
  }
}