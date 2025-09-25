#include <WiFi.h>
#include "DHT.h"
#include "ThingSpeak.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HTTPClient.h>

// ---------- Wi-Fi Setup ----------
const char* ssid = "sarisha"; 
const char* password = "shiashia";

// ---------- ThingSpeak Setup ----------
WiFiClient client;
unsigned long myChannelNumber = 3047344;
const char * myWriteAPIKey = "KFEJKYLF0ZV9R850";

// ---------- Make webhook ----------
String webhook_url = "https://hook.eu2.make.com/qp4uq7exingel85i5b5lxv9onbkvenr9";

// ---------- Pins ----------
#define DHTPIN 27
#define DHTTYPE DHT11
#define MIC_PIN 34
#define MQ2_PIN 35
#define BUZZER_PIN 25

DHT dht(DHTPIN, DHTTYPE);

// ---------- OLED Setup ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------- Timers ----------
unsigned long lastUpdate = 0;
const unsigned long postingInterval = 20000; // ThingSpeak ~20s

unsigned long tempTimer = 0;
unsigned long humTimer  = 0;
unsigned long micTimer  = 0;
unsigned long gasTimer  = 0;

const unsigned long sustainTime = 30000; // 30s sustained before alert

// ---------- Helpers ----------
String urlEncode(const String &str) {
  String encoded = "";
  char c;
  char buf[4];
  for (size_t i=0; i<str.length(); i++){
    c = str[i];
    if ( (c >= '0' && c <= '9') ||
         (c >= 'A' && c <= 'Z') ||
         (c >= 'a' && c <= 'z') ||
         c == '-' || c == '_' || c == '.' || c == '~' ) {
      encoded += c;
    } else {
      sprintf(buf, "%%%02X", (unsigned char)c);
      encoded += String(buf);
    }
  }
  return encoded;
}

String escapeJsonString(const String &s) {
  String result = "";
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '\"') result += "\\\"";      // escape quotes
    else if (c == '\n') result += "\\n";  // escape newlines
    else if (c == '\r') continue;         // remove carriage returns
    else result += c;
  }
  return result;
}


void sendToMake(String status, String message, float temp, float hum, int mic, int gas) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Not connected to WiFi — cannot call webhook");
    return;
  }

  HTTPClient http;
  http.begin(webhook_url);
  http.addHeader("Content-Type", "application/json");

  // Build JSON payload
  String postData = "{";
  postData += "\"status\":\"" + status + "\",";
  postData += "\"message\":\"" + message + "\",";
  postData += "\"temp\":" + String(temp, 2) + ",";
  postData += "\"hum\":" + String(hum, 2) + ",";
  postData += "\"mic\":" + String(mic) + ",";
  postData += "\"gas\":" + String(gas);
  postData += "}";

  Serial.print("Sending JSON to webhook: ");
  Serial.println(postData);

  int httpResponseCode = http.POST(postData);

  Serial.print("Webhook response: ");
  Serial.println(httpResponseCode);

  http.end();
}


void sendBuzzerPattern(int times = 3) {
  for (int i = 0; i < times; i++) {
    tone(BUZZER_PIN, 2000);  // 2000 Hz tone
    delay(200);
    noTone(BUZZER_PIN);      // stop the tone
    delay(200);
  }
}


// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Beehive Monitor Init...");
  display.display();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  ThingSpeak.begin(client);
}

// ---------- Main loop ----------
void loop() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int micValue = analogRead(MIC_PIN);
  int gasValue = analogRead(MQ2_PIN);

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.printf("Temp: %.2f C  Hum: %.2f %%\n", temperature, humidity);
  }
  Serial.printf("Mic:%d  Gas:%d\n", micValue, gasValue);

  // ---------------- OLED Display ----------------
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("Temp: "); display.print(temperature); display.println(" C");
  display.print("Hum: "); display.print(humidity); display.println(" %");
  display.print("Mic: "); display.print(micValue); display.println();
  display.print("Gas: "); display.print(gasValue); display.println();
  display.display();

  // ---- Temperature, Humidity, Mic, Gas ----
String combinedStatus = ""; // Will be "Critical" if any critical exists, else "Warning" if any warning exists
String combinedMessage = "";

// ---------- Temperature ----------
if (temperature < 28 || temperature > 39) {
    if (tempTimer == 0) tempTimer = millis();
    if (millis() - tempTimer >= sustainTime) {
        String status = "Critical";
        String message = "Hive temp danger!" + String(temperature) + "°C";
        Serial.println(message);
        sendBuzzerPattern(4);
        combinedStatus = "Critical";
        combinedMessage += message + "\n";
        tempTimer = millis();
    }
} else if (temperature < 31 || temperature > 37) {
    if (tempTimer == 0) tempTimer = millis();
    if (millis() - tempTimer >= sustainTime) {
        String status = "Warning";
        String message = "Hive temp outside optimal range: " + String(temperature) + " °C";
        Serial.println(message);
        combinedStatus = (combinedStatus != "Critical") ? "Warning" : combinedStatus;
        combinedMessage += message + "\n";
        tempTimer = millis();
    }
} else {
    tempTimer = 0;
}

// ---------- Humidity ----------
if (humidity < 40 || humidity > 80) {
    if (humTimer == 0) humTimer = millis();
    if (millis() - humTimer >= sustainTime) {
        String status = "Critical";
        String message = "Hive humidity unsafe " + String(humidity) + " %";
        Serial.println(message);
        sendBuzzerPattern(4);
        combinedStatus = "Critical";
        combinedMessage += message + "\n";
        humTimer = millis();
    }
} else if (humidity < 45 || humidity > 75) {
    if (humTimer == 0) humTimer = millis();
    if (millis() - humTimer >= sustainTime) {
        String status = "Warning";
        String message = "Hive humidity drifting " + String(humidity) + " %";
        Serial.println(message);
        combinedStatus = (combinedStatus != "Critical") ? "Warning" : combinedStatus;
        combinedMessage += message + "\n";
        humTimer = millis();
    }
} else {
    humTimer = 0;
}

// ---------- Microphone ----------
if (micValue > 400 || micValue < 50) {
    if (micTimer == 0) micTimer = millis();
    if (millis() - micTimer >= sustainTime) {
        String status = "Critical";
        String message = "Abnormal hive sound detected, Mic " + String(micValue);
        Serial.println(message);
        sendBuzzerPattern(4);
        combinedStatus = "Critical";
        combinedMessage += message + "\n";
        micTimer = millis();
    }
} else if (micValue > 300 && micValue < 400) {
    if (micTimer == 0) micTimer = millis();
    if (millis() - micTimer >= sustainTime) {
        String status = "Warning";
        String message = "Unusual bee activity sound. Mic " + String(micValue);
        Serial.println(message);
        combinedStatus = (combinedStatus != "Critical") ? "Warning" : combinedStatus;
        combinedMessage += message + "\n";
        micTimer = millis();
    }
} else {
    micTimer = 0;
}

// ---------- Gas ----------
if (gasValue > 1000) {
    if (gasTimer == 0) gasTimer = millis();
    if (millis() - gasTimer >= sustainTime) {
        String status = "Critical";
        String message = "GAS ALERT!Gas: " + String(gasValue);
        Serial.println(message);
        sendBuzzerPattern(4);
        combinedStatus = "Critical";
        combinedMessage += message + "\n";
        gasTimer = millis();
    }
} else {
    gasTimer = 0;
}

// ---------- Send combined alert if any ----------
if (combinedMessage.length() > 0) {
    Serial.println("Sending combined alert:");
    Serial.println(combinedMessage);
    
    // Add camera link ONLY if Critical
    String camLink = "";
    if (combinedStatus == "Critical") {
        camLink = "http://192.168.0.27/";  // <--- replace with your ESP32-CAM IP
        combinedMessage += "\nCamera Live Stream: " + camLink;
    }
    String escapedMessage = escapeJsonString(combinedMessage);
    sendToMake(combinedStatus, escapedMessage, temperature, humidity, micValue, gasValue);
}

// ---------------- Send data to ThingSpeak ----------------
  unsigned long now = millis();
  if (now - lastUpdate >= postingInterval) {
    ThingSpeak.setField(1, temperature);
    ThingSpeak.setField(2, humidity);
    ThingSpeak.setField(3, micValue);
    ThingSpeak.setField(4, gasValue);

    int statusCode = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (statusCode == 200) {
      Serial.println("ThingSpeak update successful.");
    } else {
      Serial.println("Problem updating channel. HTTP error code " + String(statusCode));
    }
    lastUpdate = now;
  }

  delay(500);
}

