#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <time.h>
#include "DHT.h"
#include "FS.h"
#include "SD.h"
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

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("Connected!");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Hostname: ");
  Serial.println(hostname);
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
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d, %Y %H:%M:%S");
}

String localTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return "Failed to obtain time";
  }

  char buffer[64];
  strftime(buffer, sizeof(buffer), "%A, %B %d, %Y %H:%M:%S", &timeinfo);
  return String(buffer);
}

void initSDCard(){
  if(!SD.begin()){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
}

void setup() {
  Serial.begin(115200);
  initSDCard();
  dht.begin();
  initWiFi();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  Serial.println("Temperature: " + String(dht.readTemperature()) + "°C");
  Serial.println("Humidity: " + String(getHumidity()) + "%");
  Serial.print("Soil Moisture: " + String(getSoilMoisture()));

  pinMode(AOUT_PIN, INPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(2, OUTPUT); // REMOVE AFTER TESTING!
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SD, "/index.html", "text/html");
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
  server.serveStatic("/", SD, "/");
  server.begin();
}

void loop() {}
