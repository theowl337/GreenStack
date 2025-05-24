#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include "DHT.h"

const char* hostname = "Pflanze";
const char* ssid = "FRITZ!Box 7580 YV";
const char* password = "08937923487059560914";

const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

#define AOUT_PIN 36 // Bodenfeuchtigkeitssensor

#define DHTPIN 4 // Temperatur und Luftfeuchtigkeitssensor
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

const int pumpPin = 26;
String outputPumpState = "off";

String lastWateringTime = "Never";

WebServer server(80);

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, password);
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

double mapValue(double value, double fromLow, double fromHigh, double toLow, double toHigh) {
    return (value - fromLow) / (fromHigh - fromLow) * (toHigh - toLow) + toLow;
}

void handleWater() {
  outputPumpState = "on";
  digitalWrite(pumpPin, HIGH);
  String html = "<!DOCTYPE html><html><head><title>Watering...</title></head><body><h1>Watering...</h1><p>You should be redirected once the watering is done.<br>If you are not redirected, press this button: <button><a href=\"../\">Redirect</a></button></p><script>setTimeout(function(){window.location.href=\"../\";},5000);</script></body></html>";
  server.send(200, "text/html", html);
  delay(5000);
  outputPumpState = "off";
  digitalWrite(pumpPin, LOW);
}

void handleRoot() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)){
    Serial.println("Failed to get time");
    return;
  }
  char timeMonth[10];
  strftime(timeMonth,10, "%B", &timeinfo);
  char timeWeekDay[10];
  strftime(timeWeekDay,10, "%A", &timeinfo);
  char timeDay[3];
  strftime(timeDay,3, "%d", &timeinfo);
  char timeHour[3];
  strftime(timeHour,3, "%H", &timeinfo);
  char timeMinute[3];
  strftime(timeMinute,3, "%M", &timeinfo);
  char timeSecond[3];
  strftime(timeSecond,3, "%S", &timeinfo);
  String html = "<!DOCTYPE html><html><head><title>SensorTest</title><style>body{width:50%;max-width:1000px;margin:auto;}h1{text-align:center;}.temp{display:inline-block;background-color:rgb("+String(mapValue(dht.readTemperature(),15.0,30.0,0.0,255.0))+",0,"+String(mapValue(dht.readTemperature(),30.0,15.0,0.0,255.0))+");height:12px;width:12px;border:2px solid black;}</style></head><body><h1>SensorTest</h1><p>Temperature: "+String(dht.readTemperature())+"<span>&#8451;</span> <span class=\"temp\"></span></p><p>Humidity: "+String(dht.readHumidity())+"%</p><p>Pflanzenfeuchtigkeit: "+String(analogRead(AOUT_PIN))+"</p><p>Time: "+String(timeWeekDay)+", "+String(timeMonth)+" "+String(timeDay)+" "+String(timeHour)+":"+String(timeMinute)+":"+String(timeSecond)+"</p><button><a href=\"./water\">Water Plant</a></button></body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  initWiFi();
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  pinMode(AOUT_PIN, INPUT);
  pinMode(pumpPin, OUTPUT);
  
  server.on("/", handleRoot);
  server.on("/water", handleWater);
  server.begin();
  Serial.println(mapValue(dht.readTemperature(), 15.0, 30.0, 0.0, 1.0));
}

void loop() {
 server.handleClient();
}