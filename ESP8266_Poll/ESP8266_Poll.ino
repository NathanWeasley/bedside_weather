#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#include <cstdint>
#include <unordered_map>

ESP8266WiFiMulti WiFiMulti;

#define WIFI_SSID     "ghq"
#define WIFI_PASSWORD "gghhqq1963"
#define API_FORCAST   "forecast"
#define API_INSTANT   "weather"
#define API_URL       "http://api.openweathermap.org/data/2.5/weather?id="
#define API_CITY_ID   "1790630"
#define API_COUNTRY   "CN"
#define API_APPID     "4da28fda20eca6cb0e9a8a6b5da9002d"

#define FORECAST_MAX_DATA 40

struct TimeDate
{
  uint16_t _year;
  uint16_t _month;
  uint16_t _day;
  uint16_t _hour;
  uint16_t _minute;
  uint16_t _second;

  TimeDate()
  : _year(1970)
  , _month(0)
  , _day(0)
  , _hour(0)
  , _minute(0)
  , _second(0)
  {}

  TimeDate(uint64_t utc, int8_t zone = 8)
  {
    // Adjust timezone
    utc += (int64_t)zone * 3600;

    // Total days
    _day = (uint32_t)(utc / __utc_sec_per_day);

    // Extract year
    _year = 1970;
    while (_day > __utc_days_in_normal_year)
    {
        _day -= days_in_year(_year);
        ++_year;
    }
    bool leap = is_leap_year(_year);
    
    // Extract month
    static const uint8_t days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    _month = 0;
    for (_month = 0; _month < 12; ++_month)
    {
        uint8_t monthday = days_in_month[_month] + ((leap && _month == 1) ? 1 : 0);
        if (_day >= monthday)
        {
            _day -= monthday;
        }
        else
        {
            break;
        }
    }

    // Extract day(no need to do anything)

    // Extract hour
    uint32_t seconds = (uint32_t)(utc % __utc_sec_per_day);
    _hour = seconds / 3600;
    _minute = seconds % 3600 / 60;
    _second = seconds % 60;
  }

private:
  static constexpr uint32_t __utc_sec_per_day = 86400;
  static constexpr uint32_t __utc_days_in_normal_year = 365;
  static constexpr uint32_t __utc_days_in_leap_year = 366;

  static inline bool is_leap_year(uint32_t year)
  {
    return (year % 4 == 0) && ((year % 100 != 0) || ((year % 100 == 0) && (year % 400 == 0)));
  }

  static inline uint32_t days_in_year(uint32_t year)
  {
    return is_leap_year(year) ? __utc_days_in_leap_year : __utc_days_in_normal_year;
  }
};



// struct WeatherData
// {
//   using WeatherType = enum
//   {
//     THUNDERSTORM = 2,
//     DRIZZLE = 3,
//     RAIN = 5,
//     SNOW = 6,
//     MIST = 7,
//     CLEAR = 8,
//     CLOUDS = 8,
//   };

//   // hash map from weather code to weather icon index
//   static inline constexpr std::unordered_map<uint32_t, uint32_t> __weather_type =
//   {
//     { 200, 0 },   // Thunderstorm with light rain
//     { 201, 0 },   // Thunderstorm with rain
//     { 202, 0 },   // Thunderstorm with heavy rain
//     { 210, 0 },   // Light thunderstorm
//     { 211, 0 },   // Thunderstorm
//     { 212, 0 },   // Heavy thunderstorm
//     { 221, 0 },   // Ragged thunderstorm
//     { 230, 0 },   // Thunderstorm with light drizzle
//     { 231, 0 },   // Thunderstorm with drizzle
//     { 232, 0 },   // Thunderstorm with heavy drizzle

//     { 300, 1 },	// light intensity drizzle
//     { 301, 1 },	// drizzle
//     { 302, 1 },	// heavy intensity drizzle
//     { 310, 1 },	// light intensity drizzle rain
//     { 311, 1 },	// drizzle rain
//     { 312, 1 },	// heavy intensity drizzle rain
//     { 313, 1 },	// shower rain and drizzle
//     { 314, 1 },	// heavy shower rain and drizzle
//     { 321, 1 },	// shower drizzle
//   };


//   TimeDate timedate;
//   float temp_max;
//   float temp_min;
//   ;
// };

String url;

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 4; t > 0; t--)
  {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  url = API_URL;
  url += API_CITY_ID;
  url += "&appid=";
  url += API_APPID;
}

void loop() {
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED))
  {
    WiFiClient client;
    HTTPClient http;

    Serial.print("[HTTP] begin... ");
    Serial.println(url.c_str());
    if (http.begin(client, url))
    {

      Serial.print("[HTTP] GET...\n");
      int httpCode = http.GET();

      if (httpCode > 0)
      {
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {
          String payload = http.getString();
          Serial.println("Response:");
          Serial.println(payload);

          // Parse JSON
          DynamicJsonDocument doc(2000);
          DeserializationError error = deserializeJson(doc, payload);

          if (error)
          {
            Serial.print("Error parsing JSON: ");
            Serial.println(error.c_str());
          }
          else
          {
            // Access individual items
            Serial.println("Parsed Data:");
            Serial.println("City: " + doc["name"].as<String>());
            Serial.println("Temperature 1: " + String(doc["main"]["temp"].as<float>() - 273.15f));
            Serial.println("Weather Description 1: " + doc["weather"][0]["description"].as<String>());
            // Serial.println("Temperature 2: " + String(doc["list"][1]["main"]["temp"].as<float>()));
            // Serial.println("Weather ID 1: " + String(doc["list"][0]["weather"][0]["id"].as<int>()));
            // Serial.println("Weather ID 2: " + String(doc["list"][1]["weather"][0]["id"].as<int>()));

            // Serial.println("Weather Description 2: " + doc["list"][1]["weather"][0]["description"].as<String>());
          }
        }
      }
      else
      {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    }
    else
    {
      Serial.println("[HTTP] Unable to connect");
    }
  }

  delay(600000);
}
