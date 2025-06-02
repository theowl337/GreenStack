#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <time.h>
#include "DHT.h"
#include "FS.h"
#include "SPI.h"
#include "secrets.h"
#include "SPIFFS.h"

const char* hostname = "GreenStack";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

AsyncWebServer server(80);

#define AOUT_PIN 33 // soil moisture sensor

#define DHTPIN 13 // temperature and humidity sensor
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

const int pumpPin = 26;
String pumpState;
String lastWateringTime = "Never";

bool isAPMode = false;

void startAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.println("Access Point started");
  Serial.print("AP SSID: ");
  Serial.println(ap_ssid);
  Serial.print("AP IP Adresse: ");
  Serial.println(IP);
  
  isAPMode = true;
}

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  Serial.println("Versuche Verbindung zu WiFi herzustellen...");
  
  unsigned long startTime = millis();
  const unsigned long timeout = 30000;  // Timeout 30 Sec
  
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < timeout) {
    Serial.print('.');
    delay(1000);
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi verbunden!");
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("Hostname: ");
    Serial.println(hostname);
    isAPMode = false;
    
    // NTP Zeit synchronisieren
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  } else {
    Serial.println("\nNo Wifi connection. Starting AP mode...");
    startAccessPoint();
  }
}

float getTemperature() {
  return (dht.readTemperature());
}

float getHumidity() {
  return (dht.readHumidity());
}

float getSoilMoisture() {
  return (analogRead(AOUT_PIN));
}

void printLocalTime() {
  if (isAPMode) {
    Serial.println("!Zeit nicht verfügbar (AP Modus)");
    return;
  }
  
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d, %Y %H:%M:%S");
}

String localTime() {
  if (isAPMode) {
    return "!Zeit nicht verfügbar (AP modus)";
  }
  
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "Failed to obtain time";
  }

  char buffer[64];
  strftime(buffer, sizeof(buffer), "%A, %B %d, %Y %H:%M:%S", &timeinfo);
  return String(buffer);
}

void initSPIFFS(){
  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  Serial.println("SPIFFS mounted successfully");
  
  size_t totalBytes = SPIFFS.totalBytes();
  size_t usedBytes = SPIFFS.usedBytes();
  Serial.printf("SPIFFS Total: %u bytes, Used: %u bytes\n", totalBytes, usedBytes);
}

void setup() {
  Serial.begin(115200);
  initSPIFFS();
  dht.begin();
  initWiFi();  
  
  printLocalTime();
  Serial.println("Temperature: " + String(dht.readTemperature()) + "°C");
  Serial.println("Humidity: " + String(getHumidity()) + "%");
  Serial.print("Soil Moisture: " + String(getSoilMoisture()));

  pinMode(AOUT_PIN, INPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(2, OUTPUT); // REMOVE AFTER TESTING!
 
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });
 
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request){ // get style
    request->send(SPIFFS, "/styles.css", "text/css");
  });
 
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){ // get js
    request->send(SPIFFS, "/script.js", "application/javascript");
  });
 
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
      float temp = getTemperature();
      String json = "{\"temp\":" + String(temp, 1) + "}";
      request->send(200, "application/json", json);
      });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
      float humidity = getHumidity();
      String json = "{\"humidity\":" + String(humidity, 1) + "}";
      request->send(200, "application/json", json);
      });
  server.on("/soilmoisture", HTTP_GET, [](AsyncWebServerRequest *request){
      float soilMoisture = getSoilMoisture();
      String json = "{\"soilmoisture\":" + String(soilMoisture, 1) + "}";
      request->send(200, "application/json", json);
      });
  server.on("/pump_on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(pumpPin, HIGH);
    digitalWrite(2, HIGH); // REMOVE AFTER TESTING!
    pumpState = "on";
    lastWateringTime = localTime();
    Serial.println("request for /pump_on received - toggling pump at " + localTime());
    request->send(200, "text/plain", "pump toggled");

    static bool taskRunning = false;
    if (!taskRunning) {
      taskRunning = true;
      xTaskCreate([](void *){
        delay(5000);
        digitalWrite(pumpPin, LOW);
        digitalWrite(2, LOW); // REMOVE AFTER TESTING
        pumpState = "off";
        taskRunning = false;
        vTaskDelete(NULL);
      }, "Pump_Off_Task", 2048, NULL, 1, NULL);
    }
  });
  
  server.begin();
}

void loop() {
  delay(700); 
}
