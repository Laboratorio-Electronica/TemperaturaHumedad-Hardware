#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "HTTPClient.h"

// Variables de la red WiFi 
const char* ssid = "Krloz Medina";
const char* password =  "F@mili@571112";

// Variables del servidor Network Time Protocol
const char* ntpServer = "co.pool.ntp.org";
const long  gmtOffset_sec = -21600;
const int   daylightOffset_sec = 3600;

#define DHT_PIN 13
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);
struct tm timeinfo;
char fecha[25];
char id[30];

char week[10];

void setup() {
  Serial.begin(9600);
  Serial.println(F("DHTxx test!"));
  WiFi.begin(ssid, password);
  dht.begin();

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  //Servido NTP (Network Time Protocole)
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
 
  Serial.println("Connected to the WiFi network");
}

void loop() {
  // Obtener la fecha
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  strftime(fecha, 25, "%d-%B-%Y %H:%M", &timeinfo);
  strftime(id, 30, "%y%j%H%M", &timeinfo);

  strftime(week, 10, "%W", &timeinfo);

  // Serial.println(week);
  // delay(2000);

  float humidity = dht.readHumidity();
  float temperatureC = dht.readTemperature();
  float temperatureF = dht.readTemperature(true); // Read temperature as Fahrenheit (isFahrenheit = true)

  if (isnan(humidity) || isnan(temperatureC) || isnan(temperatureF)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  float heatIndexC = dht.computeHeatIndex(temperatureC, humidity, false);
  float heatIndexF = dht.computeHeatIndex(temperatureF, humidity);

  Serial.print(F("Humidity: "));
  Serial.print(humidity);
  Serial.print(F("%  Temperature: "));
  Serial.print(temperatureC);
  Serial.print(F("°C "));
  Serial.print(temperatureF);
  Serial.print(F("°F  Heat index: "));
  Serial.print(heatIndexC);
  Serial.print(F("°C "));
  Serial.print(heatIndexF);
  Serial.println(F("°F"));

  if(WiFi.status()== WL_CONNECTED){
    HTTPClient http;   
    http.begin("https://2a6jbzx4xd.execute-api.us-east-1.amazonaws.com/items");
    http.addHeader("Content-Type", "text/plain");            
    
    StaticJsonBuffer <300> JSONbuffer;
    JsonObject& JSONencoder = JSONbuffer.createObject();
    JSONencoder["id"] = id;
    JSONencoder["temperature"] = temperatureC;
    JSONencoder["humidity"] = humidity;
    JSONencoder["time"] = fecha;
    JSONencoder["week"] = week;
    JSONencoder["ubication"] = "warehouse";
    char JSONmessageBuffer[300];
    JSONencoder.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

    int httpResponseCode = http.PUT(JSONmessageBuffer);

    if(httpResponseCode>0){
      String response = http.getString();   
      Serial.println(httpResponseCode);
      Serial.println(response);          
    }else{
      Serial.print("Error on sending PUT Request: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }else{
    Serial.println("Error in WiFi connection");
  }
  delay(1000000);
  // break;
}