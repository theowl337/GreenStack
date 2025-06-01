#include <WiFi.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SD.h>

AsyncWebServer server(80);

const char* wifiCredsPath = "/wifi.json";

bool readWiFiCredentials(String &ssid, String &pass) {
  if (!SD.exists(wifiCredsPath)) {
    Serial.println("[WiFiConfig] WiFiCredentials file not found");
    return false;
  }

  File file = SD.open(wifiCredsPath);
  if (!file) {
    Serial.println("[WiFiConfig] Failed to open wifi.json");
    return false;
  }

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.println("[WiFIConfig] Failed to parse wifi.json");
    return false;
  }

  ssid = doc["ssid"].as<String>();
  pass = doc["pass"].as<String>();
  return true;
}

void saveWifiCredentials(const String &ssid, consit String &pass) {
  StaticJsonDocument<256> doc;
  doc["ssid"] = ssid;
  doc["pass"] = pass;

  File file = SD.open(wifiCredsPath, FILE_WRITE);
  if (!file) {
    Serial.println("[WiFiConfig] Failed to write wifi.json");
    return;
  }

  serializeJson(doc, file);
  file.close();
  Serial.println("[WiFIConfig] WiFiCredentials saved to SD");
}

void startConfigPortal() {
  const char* apSSID = "GreenStack";

  Serial.println("[WiFiConfig] Starting AP mode");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID);

  IPAddress IP = WiFI.softAPIP();
  Serial.println("[WiFiConfig] Access Point IP: " + IP.toString());

  String ssid, pass;

  // CONTINUE
}

void setupWiFi() {
  WiFi.mode(WIFI_STA);
  string ssid, pass;


}
