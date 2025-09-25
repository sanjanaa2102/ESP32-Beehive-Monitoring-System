# ESP32-Based IoT Beehive Monitoring System

![Beehive Monitor](https://image2url.com/images/1758808961313-477d79a2-451f-4319-82a4-64b064974db5.jpeg) 
## Overview

This project is an IoT-based environmental monitoring system designed to protect honeybee colonies by tracking critical hive conditions in real-time. Using an ESP32 microcontroller, a suite of sensors, and cloud integration with ThingSpeak, the system provides beekeepers with instant alerts about potential threats such as extreme temperatures, humidity imbalances, unusual sounds, or the presence of harmful gases.

This system helps ensure hive health, prevent colony collapse, and improve apiary management through data-driven insights.

## Features

-   **Real-Time Monitoring:** Tracks temperature, humidity, ambient sound, and gas levels every 20 seconds.
-   **Cloud Data Logging:** Sends all sensor data to a **ThingSpeak** channel for historical analysis and visualization.
-   **Dual-Threshold Alerts:** Implements "Warning" and "Critical" alert levels for each sensor to distinguish between minor deviations and immediate dangers.
-   **Sustained Condition Logic:** Triggers alerts only after an abnormal condition has been sustained for 30 seconds to prevent false alarms.
-   **Instant Notifications:** Uses a **Make.com webhook** to send detailed notifications (including a live camera feed link during critical events) to a service like Pushbullet or Telegram.
-   **On-Site Alarms:** An onboard buzzer provides an immediate, audible alarm for critical-level threats.
-   **Live Status Display:** An OLED screen displays current sensor readings for at-a-glance monitoring.

## Hardware Components

-   **Microcontroller:** ESP32 Dev Kit
-   **Sensors:**
    -   DHT11 (Temperature & Humidity)
    -   Analog Sound Sensor (Microphone)
    -   MQ-2 Gas Sensor
-   **Actuator:** Piezo Buzzer
-   **Display:** 0.96" SSD1306 OLED Display
-   **Optional:** ESP32-CAM for live video feed.

## Software and Services

-   **Framework:** Arduino IDE with the ESP32 Core
-   **Cloud Platform:** [ThingSpeak](https://thingspeak.com/)
-   **Notification Service:** [Make.com](https://www.make.com/) (formerly Integromat)
-   **Libraries:**
    -   `WiFi.h`
    -   `DHT.h`
    -   `ThingSpeak.h`
    -   `Adafruit_GFX.h`
    -   `Adafruit_SSD1306.h`

## Setup and Configuration

1.  **Hardware Assembly:** Connect the sensors, OLED display, and buzzer to the ESP32 according to the pin definitions at the top of the `.ino` sketch.

2.  **Arduino IDE:**
    -   Install the ESP32 board manager in the Arduino IDE.
    -   Install all the required libraries listed above using the Library Manager.

3.  **Wi-Fi Credentials:**
    -   Update the `ssid` and `password` variables in the code with your Wi-Fi network details.

4.  **ThingSpeak:**
    -   Create a new channel on ThingSpeak with four fields (Temperature, Humidity, Mic, Gas).
    -   Update the `myChannelNumber` and `myWriteAPIKey` variables with your channel ID and Write API Key.

5.  **Make.com (for Notifications):**
    -   Create a new scenario in Make.com that starts with a "Custom webhook" trigger.
    -   Copy the webhook URL and update the `webhook_url` variable in the code.
    -   Add other modules to the scenario to handle the incoming data (e.g., send a Pushbullet notification or a Telegram message).

6.  **ESP32-CAM (Optional):**
    -   If using an ESP32-CAM, set it up on your network and replace the placeholder IP address in the `camLink` variable.

7.  **Upload the Code:** Compile and upload the sketch to your ESP32. Open the Serial Monitor at `115200` baud to see debug output.

## How It Works

The ESP32 continuously reads data from the DHT11, microphone, and MQ-2 sensors. These values are displayed on the OLED screen and sent to ThingSpeak at a regular interval.

The core logic checks each sensor reading against two predefined thresholds: a wider "Warning" range and a narrower "Critical" range. If a reading stays within an abnormal range for more than 30 seconds, an alert is triggered. A combined message is created, and if any alert is "Critical," the buzzer sounds and a notification is sent via the Make.com webhook, which can then forward the alert to a mobile device.
