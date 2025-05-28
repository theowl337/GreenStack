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

const char* hostname = "Pflanze";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

AsyncWebServer server(80);

#define AOUT_PIN 36 // Bodenfeuchtigkeitssensor

#define DHTPIN 4 // Temperatur und Luftfeuchtigkeitssensor
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

const int pumpPin = 26;
String outputPumpState = "off";
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

  pinMode(AOUT_PIN, INPUT);
  pinMode(pumpPin, OUTPUT);
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SD, "/index.html", "text/html");
  });
  server.on("/pump_on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(pumpPin, HIGH);
    Serial.println("request for /pump_on received - toggling pump");
    request->send(200, "text/plain", "pump toggled");

    static bool taskRunning = false;
    if (!taskRunning) {
      taskRunning = true;
      xTaskCreate([](void *){
        delay(5000);
        digitalWrite(pumpPin, LOW);
        taskRunning = false;
        vTaskDelete(NULL);
      }, "Pump_Off_Task", 2048, NULL, 1, NULL);
    }
  });
  server.serveStatic("/", SD, "/");
  server.begin();
}

void loop() {}
