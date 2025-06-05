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
#include <ArduinoJson.h>

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

// WiFi credential storage
String currentSSID = "";
String currentPassword = "";

// WiFi credential storage functions
bool saveWiFiCredentials(const String& ssid, const String& password) {
  DynamicJsonDocument doc(512);
  doc["ssid"] = ssid;
  doc["password"] = password;
  
  File file = SPIFFS.open("/wifi_config.json", "w");
  if (!file) {
    Serial.println("Failed to open wifi_config.json for writing");
    return false;
  }
  
  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write to wifi_config.json");
    file.close();
    return false;
  }
  
  file.close();
  Serial.println("WiFi credentials saved successfully");
  return true;
}

bool loadWiFiCredentials() {
  if (!SPIFFS.exists("/wifi_config.json")) {
    Serial.println("No stored WiFi credentials found, using defaults from secrets.h");
    currentSSID = WIFI_SSID;
    currentPassword = WIFI_PASSWORD;
    return false;
  }
  
  File file = SPIFFS.open("/wifi_config.json", "r");
  if (!file) {
    Serial.println("Failed to open wifi_config.json for reading");
    currentSSID = WIFI_SSID;
    currentPassword = WIFI_PASSWORD;
    return false;
  }
  
  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, file);
  file.close();
  
  if (error) {
    Serial.println("Failed to parse wifi_config.json");
    currentSSID = WIFI_SSID;
    currentPassword = WIFI_PASSWORD;
    return false;
  }
  
  currentSSID = doc["ssid"].as<String>();
  currentPassword = doc["password"].as<String>();
  
  Serial.println("Loaded WiFi credentials: " + currentSSID);
  return true;
}

bool clearWiFiCredentials() {
  if (SPIFFS.exists("/wifi_config.json")) {
    if (SPIFFS.remove("/wifi_config.json")) {
      Serial.println("WiFi credentials cleared");
      currentSSID = WIFI_SSID;
      currentPassword = WIFI_PASSWORD;
      return true;
    }
  }
  return false;
}

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
  // Load stored credentials or use defaults
  loadWiFiCredentials();
  
  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname);
  WiFi.begin(currentSSID.c_str(), currentPassword.c_str());
  
  Serial.println("Versuche Verbindung zu WiFi herzustellen...");
  Serial.println("SSID: " + currentSSID);
  
  unsigned long startTime = millis();
  const unsigned long timeout = 20000;  // Timeout 20 Sec
  
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
 
  // Serve static files
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
  });
 
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/styles.css", "text/css");
  });
 
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/script.js", "application/javascript");
  });
 
  // Sensor API endpoints
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
  
  // Pump control
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

  
  // WiFi Status
  server.on("/wifi/status", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{";
    json += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
    json += "\"ap_mode\":" + String(isAPMode ? "true" : "false") + ",";
    
    if (WiFi.status() == WL_CONNECTED) {
      json += "\"ssid\":\"" + WiFi.SSID() + "\",";
      json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    }
    
    if (isAPMode) {
      json += "\"ap_ssid\":\"" + String(ap_ssid) + "\",";
    }
    
    json += "\"ip\":\"" + (isAPMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString()) + "\",";
    json += "\"stored_ssid\":\"" + currentSSID + "\"";
    json += "}";
    
    request->send(200, "application/json", json);
  });

  // WiFi Connect
  server.on("/wifi/connect", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
      String body = String((char*)data).substring(0, len);
      Serial.println("WiFi connect request: " + body);
      
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, body);
      
      if (error) {
        Serial.println("JSON parse error");
        request->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
        return;
      }
      
      String newSSID = doc["ssid"];
      String newPassword = doc["password"];
      
      Serial.println("Attempting to connect to: " + newSSID);
      
      // Disconnect current WiFi
      WiFi.disconnect();
      delay(1000);
      
      WiFi.begin(newSSID.c_str(), newPassword.c_str());
      
      unsigned long startTime = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
        delay(500);
        Serial.print(".");
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        isAPMode = false;
        Serial.println("\nConnected to new WiFi!");
        Serial.println("IP: " + WiFi.localIP().toString());
        
        // Save new credentials to SPIFFS
        if (saveWiFiCredentials(newSSID, newPassword)) {
          currentSSID = newSSID;
          currentPassword = newPassword;
          Serial.println("New WiFi credentials saved successfully");
        } else {
          Serial.println("Warning: Failed to save WiFi credentials");
        }
        
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Connected and credentials saved\",\"ip\":\"" + WiFi.localIP().toString() + "\"}");
      } else {
        Serial.println("\nConnection failed!");
        initWiFi();
        request->send(200, "application/json", "{\"success\":false,\"message\":\"Connection failed\"}");
      }
    });

  server.on("/wifi/reset", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println("WiFi reset requested");
    
    if (clearWiFiCredentials()) {
      request->send(200, "application/json", "{\"success\":true,\"message\":\"WiFi credentials reset to defaults\"}");
      Serial.println("Restarting ESP32 to apply changes...");
      delay(1000);
      ESP.restart();
    } else {
      request->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to clear credentials\"}");
    }
  });

  server.on("/wifi/toggle_ap", HTTP_POST, [](AsyncWebServerRequest *request){
    Serial.println("AP mode toggle requested");
    
    if (isAPMode) {
      Serial.println("Switching from AP to STA mode");
      WiFi.softAPdisconnect(true);
      delay(1000);
      
      WiFi.mode(WIFI_STA);
      WiFi.begin(currentSSID.c_str(), currentPassword.c_str());
      
      unsigned long startTime = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
        delay(500);
        Serial.print(".");
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        isAPMode = false;
        Serial.println("\nSwitched to WiFi mode successfully");
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        request->send(200, "application/json", "{\"success\":true,\"message\":\"Switched to WiFi mode\",\"ip\":\"" + WiFi.localIP().toString() + "\"}");
      } else {
        Serial.println("\nWiFi connection failed, staying in AP mode");
        startAccessPoint();
        request->send(200, "application/json", "{\"success\":false,\"message\":\"WiFi connection failed, staying in AP mode\"}");
      }
      
    } else {
      Serial.println("Switching from STA to AP mode");
      WiFi.disconnect();
      delay(1000);
      startAccessPoint();
      request->send(200, "application/json", "{\"success\":true,\"message\":\"Switched to AP mode\",\"ap_ssid\":\"" + String(ap_ssid) + "\",\"ip\":\"" + WiFi.softAPIP().toString() + "\"}");
    }
  });

  // Handle 404
  server.onNotFound([](AsyncWebServerRequest *request){
    request->send(404, "text/plain", "Not found :(");
  });
  
  server.begin();
}

void loop() {
  delay(700); 
}
