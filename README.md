# IoT Environmental Weather Station
This project is a complete, battery-powered IoT weather station that measures various environmental parameters and displays them on a real-time, web-based dashboard. It uses a Seeed Studio XIAO ESP32S3 microcontroller to read sensor data and publishes it via MQTT to a cloud broker. The data is then visualized using a Node-RED dashboard, accessible from any device on the local network like a PC, iPad, or tablet.

A walkthrough of my project development is in the [Dev Blog](Dev_Blog.md)

## Features

Real-time Monitoring: Measures Temperature, Humidity, Barometric Pressure, UV Intensity, and Ambient Light.

IoT Connectivity: Publishes data wirelessly over Wi-Fi using the lightweight MQTT protocol.

Web-Based Dashboard: A clean, graphical user interface built with Node-RED to visualize data with gauges and charts.

Scalable: The architecture allows for easy addition of new sensors and devices.

Power Efficient Design: Designed with components suitable for a future battery and solar-powered setup.

## System Architecture

The data flows through the system in a simple, robust sequence:

Sensors -> XIAO ESP32S3 -> Wi-Fi -> HiveMQ MQTT Broker -> Node-RED -> Web Dashboard

## Hardware Components

Microcontroller: Seeed Studio XIAO ESP32S3

Environmental Sensor: DFRobot SEN0501 Gravity: Multifunctional Environmental Sensor

Power Management (Planned):

DFRobot DFR0559 5V Solar Manager

4x 200mAh 3.7V LiPo Battery Cells

Dust Sensor (Planned): Sharp GP2Y1010AU0F

Misc: USB-C Cable, Jumper Wires

## Software and Services

Device Firmware: Arduino IDE

MQTT Broker: HiveMQ Cloud (Free Tier)

Dashboard & Logic: Node-RED running on a PC or Raspberry Pi

### Core Arduino Libraries:

WiFi.h

WiFiClientSecure.h

PubSubClient by Nick O'Leary

DFRobot_EnvironmentalSensor

## Setup Instructions

This project is set up in three main parts: The Device, The Broker, and The Dashboard.

Part 1: The Device (XIAO ESP32S3)

1.1. Hardware Wiring

Connect the DFRobot SEN0501 sensor to the XIAO ESP32S3. Ensure the physical switch on the sensor is set to UART mode.

DFRobot SEN0501	Connection	XIAO ESP32S3 Pin

VCC (Red)	Power	3V3

GND (Black)	Ground	GND

D/T (TX)	Sensor Transmit	D3 (RX)

C/R (RX)	Sensor Receive	D2 (TX)

1.2. Arduino IDE Setup

Install ESP32 Board Support: Add the ESP32 boards manager URL in File > Preferences.

Install Libraries: Use the Library Manager (Sketch > Include Library > Manage Libraries...) to install PubSubClient and DFRobot_EnvironmentalSensor.

Load the Code: Copy the final Arduino code into a new sketch.

1.3. Configure and Flash

Before uploading, you must configure the credentials in the code:

```
// --- CONFIGURE YOUR SETTINGS HERE ---
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const char* mqtt_server = "YOUR_CLUSTER_URL.s1.eu.hivemq.cloud";
const int   mqtt_port = 8883;
const char* mqtt_user = "YOUR_HIVEMQ_USERNAME";
const char* mqtt_pass = "YOUR_HIVEMQ_PASSWORD";

const char* mqtt_topic = "myhome/weatherstation/data";
// ---
```
Upload the configured code to your XIAO ESP32S3.

Part 2: The MQTT Broker (HiveMQ Cloud)

Create an Account: Sign up for a free HiveMQ Cloud account.

Create a Cluster: Create a new free-tier cluster.

Generate Credentials: Go to the "Access Management" tab and create a new username and password for your device. Use these credentials in your Arduino code and in Node-RED.

Part 3: The Dashboard (Node-RED)

3.1. Install and Run Node-RED

Install Node.js: Download and install the LTS version from nodejs.org.

Install Node-RED: Open a command prompt or terminal and run npm install -g --unsafe-perm node-red.

Run Node-RED: In the same terminal, type node-red.

Access Editor: Open a web browser and go to http://localhost:1880.

3.2. Configure the Node-RED Flow

Install Dashboard: In the editor, go to Menu > Manage Palette > Install and install node-red-dashboard.

Create the Flow: Recreate the following flow on your canvas:
```
[mqtt in] ---> [json] ---> [dashboard nodes (gauge, text, etc.)]
```
Configure mqtt in Node:

Server: Your HiveMQ Cluster URL (e.g., xxxxxxxx.s1.eu.hivemq.cloud)

Port: 8883

Security Tab: Enter your HiveMQ device Username and Password.

TLS Tab: Check the box for "Enable secure (TLS) connection".

Topic: myhome/weatherstation/data

Configure json Node:

Leave the default settings. It will automatically parse msg.payload.

Configure Dashboard Nodes (e.g., a Temperature Gauge):

Group: Assign it to a group on a UI tab (e.g., [Home] Default).

Label: Temperature

Value format: {{payload.temperature}}

Units: °C
Range: Set a sensible range, like 0 to 50.

Repeat for all other data points ({{payload.humidity}}, {{payload.pressure}}, etc.).

Deploy: Click the red Deploy button.

View Dashboard: Open a new browser tab and navigate to http://localhost:1880/ui.

## Future Improvements

Integrate Dust Sensor: Wire and add the code for the GP2Y1010AU0F dust sensor.

Power System: Implement the DFR0559 solar manager and LiPo batteries for a fully off-grid setup.

Deep Sleep: Add code to the ESP32S3 to put it into a deep sleep mode between readings, drastically extending battery life.

Data Logging: Add nodes in Node-RED to log data over time to a file or database like InfluxDB for historical analysis.
