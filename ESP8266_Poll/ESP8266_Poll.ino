#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <cstdint>

//Def
#define myPeriodic 15 //in sec | Thingspeak pub is 15sec

// https://api.openweathermap.org/data/2.5/weather?q=Xi%E2%80%99an,CN&APPID=4da28fda20eca6cb0e9a8a6b5da9002d

/** Global constants */

const char* server = "api.openweathermap.org";
const String apiKey ="4da28fda20eca6cb0e9a8a6b5da9002d";
const char* MY_SSID = "aerofugia"; 
const char* MY_PWD = "Aero#33990";

/** Global flags */
uint8_t 

void setup()
{
    Serial.begin(115200);
    connectWifi();
}

void loop()
{
    ///< Check Wifi condition
    if (WiFi.status() != WL_CONNECTED)
    {
      WiFi.begin(MY_SSID, MY_PWD);
      Serial.print("Wi-Fi disconnected, reconnecting...");
    }
}

void connectWifi()
{
    Serial.print("Connecting to " + *MY_SSID);
    WiFi.begin(MY_SSID, MY_PWD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.print(".");
    }
    
    Serial.println("");
    Serial.println("Connected");
    Serial.println("");  
}

void sendTeperatureTS(float temp)
{  
    WiFiClient client;
    
    if (client.connect(server, 80))
    { // use ip 184.106.153.149 or api.thingspeak.com
        Serial.println("WiFi Client connected ");
        
        String postStr = apiKey;
        postStr += "&field1=";
        postStr += String(temp);
        postStr += "\r\n\r\n";
        
        client.print("POST /update HTTP/1.1\n");
        client.print("Host: api.thingspeak.com\n");
        client.print("Connection: close\n");
        client.print("X-THINGSPEAKAPIKEY: " + apiKey + "\n");
        client.print("Content-Type: application/x-www-form-urlencoded\n");
        client.print("Content-Length: ");
        client.print(postStr.length());
        client.print("\n\n");
        client.print(postStr);
        delay(1000);
    }

    sent++;
    client.stop();
}