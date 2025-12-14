#include "task.h"

#include <unordered_map>

#define WIFI_SSID     "NathansHome"
#define WIFI_PASSWORD "doge2048"
// #define API_FORCAST   "forecast"
// #define API_INSTANT   "weather"
#define API_URL       "http://api.seniverse.com/v3/weather/daily.json?key="
// #define API_CITY_ID   "1790630"
// #define API_COUNTRY   "CN"
#define API_APPID     "S20jWM82E5Odj8wf4"
#define API_SUFFIX    "&location=xian&language=en&unit=c"

// #ifdef DEBUG
// #undef DEBUG
// #endif

#ifdef DEBUG
#define SERIAL_PRINT(...)   Serial.print(__VA_ARGS__)
#define SERIAL_PRINTF(...)  Serial.printf(__VA_ARGS__)
#define SERIAL_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define SERIAL_PRINT(...)
#define SERIAL_PRINTF(...)
#define SERIAL_PRINTLN(...)
#endif

// hash map from weather code to weather icon index
static inline const std::unordered_map<uint32_t, WeatherType> __weather_type =
{
  { 200, WeatherType::THUNDERSTORM },   // Thunderstorm with light rain
  { 201, WeatherType::THUNDERSTORM },   // Thunderstorm with rain
  { 202, WeatherType::THUNDERSTORM },   // Thunderstorm with heavy rain
  { 210, WeatherType::THUNDERSTORM },   // Light thunderstorm
  { 211, WeatherType::THUNDERSTORM },   // Thunderstorm
  { 212, WeatherType::THUNDERSTORM },   // Heavy thunderstorm
  { 221, WeatherType::THUNDERSTORM },   // Ragged thunderstorm
  { 230, WeatherType::THUNDERSTORM },   // Thunderstorm with light drizzle
  { 231, WeatherType::THUNDERSTORM },   // Thunderstorm with drizzle
  { 232, WeatherType::THUNDERSTORM },   // Thunderstorm with heavy drizzle

  { 300, WeatherType::DRIZZLE },	  // light intensity drizzle
  { 301, WeatherType::DRIZZLE },	  // drizzle
  { 302, WeatherType::DRIZZLE },	  // heavy intensity drizzle
  { 310, WeatherType::DRIZZLE },	  // light intensity drizzle rain
  { 311, WeatherType::DRIZZLE },	  // drizzle rain
  { 312, WeatherType::DRIZZLE },	  // heavy intensity drizzle rain
  { 313, WeatherType::DRIZZLE },	  // shower rain and drizzle
  { 314, WeatherType::DRIZZLE },	  // heavy shower rain and drizzle
  { 321, WeatherType::DRIZZLE },	  // shower drizzle

  { 500, WeatherType::RAIN },   // Light rain
  { 501, WeatherType::RAIN },   // Moderate rain
  { 502, WeatherType::RAIN },   // Heavy intensity rain
  { 503, WeatherType::RAIN },   // Very heavy rain
  { 504, WeatherType::RAIN },   // Extreme rain
  { 511, WeatherType::SNOW },   // Freezing rain
  { 520, WeatherType::RAIN },   // Light intensity shower rain
  { 521, WeatherType::RAIN },   // Shower rain
  { 522, WeatherType::RAIN },   // Heavy intensity shower rain
  { 531, WeatherType::RAIN },   // Ragged shower rain

  { 600, WeatherType::SNOW },   // Light snow
  { 601, WeatherType::SNOW },   // Snow
  { 602, WeatherType::SNOW },   // Heavy snow
  { 611, WeatherType::SNOW },   // Sleet
  { 612, WeatherType::SNOW },   // Light shower sleet
  { 613, WeatherType::SNOW },   // Shower sleet
  { 615, WeatherType::SNOW },   // Light rain and snow
  { 616, WeatherType::SNOW },   // Rain and snow
  { 620, WeatherType::SNOW },   // Light shower snow
  { 621, WeatherType::SNOW },   // Shower snow
  { 622, WeatherType::SNOW },   // Heavy shower snow

  { 701, WeatherType::MIST },   // Mist
  { 711, WeatherType::MIST },   // Smoke
  { 721, WeatherType::MIST },   // Haze
  { 731, WeatherType::MIST },   // Sand/Dust whirls
  { 741, WeatherType::MIST },   // Fog
  { 751, WeatherType::MIST },   // Sand
  { 761, WeatherType::MIST },   // Dust
  { 762, WeatherType::MIST },   // Volcanic ash
  { 771, WeatherType::MIST },   // Squalls
  { 781, WeatherType::MIST },   // Tornado

  { 800, WeatherType::CLEAR },   // Clear

  { 801, WeatherType::CLOUDS },   // Few clouds
  { 802, WeatherType::CLOUDS },   // Scattered clouds
  { 803, WeatherType::CLOUDS },   // Broken clouds
  { 804, WeatherType::CLOUDS },   // Overcast
};



void Task::init()
{
    Serial.begin(921600);
    SERIAL_PRINTLN();
    SERIAL_PRINTF("<Task> Initializing...");
    SERIAL_PRINTLN();

    // Delay for the WiFi stack to initialize
    for (uint8_t t = 4; t > 0; t--)
    {
        SERIAL_PRINTF("<Task> Wait for WiFi %ds...\n", t);
        Serial.flush();
        delay(1000);
    }

    WiFi.mode(WIFI_STA);
    _wifi_multi.addAP(WIFI_SSID, WIFI_PASSWORD);

    _weather_url = API_URL;
    // _weather_url += API_CITY_ID;
    // _weather_url += "&appid=";
    _weather_url += API_APPID;
    _weather_url += API_SUFFIX;

    _time_url = "http://api.uuni.cn//api/time";

    reset_read_state();
}


void Task::tick()
{
    // Fetch UART RX data
    while (Serial.available())
    {
        _cbuf.writeByte(Serial.read());
    }

    // Process packet (MCU request decrypt)
    decrypt_packet();

    // Process 
    switch (_req.type)
    {
    case McuRequest::NETWORK_STATUS:
    {
        _status_resp.status = WiFi.status() == WL_CONNECTED     \
                            ? NetworkCondition::CONNECTED       \
                            : NetworkCondition::DISCONNECTED;
        resp_ready();
    } break;

    case McuRequest::WEATHER:
    {
        fetch_weather();
        resp_weather();
    } break;

    case McuRequest::DATETIME:
    {
        fetch_time();
        resp_datetime();
    } break;

    default:
      break;
    }

    delay(1000);
}

void Task::fetch_time()
{
    if (_wifi_multi.run() == WL_CONNECTED)
    {
        WiFiClient client;
        HTTPClient http;

        SERIAL_PRINT("[HTTP] begin... ");
        SERIAL_PRINTLN(_time_url.c_str());
        if (http.begin(client, _time_url))
        {
            SERIAL_PRINT("[HTTP] GET...\n");
            int httpCode = http.GET();

            if (httpCode > 0)
            {
                SERIAL_PRINTF("[HTTP] GET... code: %d\n", httpCode);

                if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
                {
                    String payload = http.getString();
                    SERIAL_PRINTLN("Response:");
                    SERIAL_PRINTLN(payload);

                    // Decrypt data into resp_time
                    decrypt_time(payload);
                }
            }
            else
            {
                SERIAL_PRINTF("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            }

            http.end();
        }
        else
        {
            SERIAL_PRINTLN("[HTTP] Unable to connect");
        }
    }
}

void Task::fetch_weather()
{
    if (_wifi_multi.run() == WL_CONNECTED)
    {
        WiFiClient client;
        HTTPClient http;

        SERIAL_PRINT("[HTTP] begin... ");
        SERIAL_PRINTLN(_weather_url.c_str());
        if (http.begin(client, _weather_url))
        {
            SERIAL_PRINT("[HTTP] GET...\n");
            int httpCode = http.GET();

            if (httpCode > 0)
            {
                SERIAL_PRINTF("[HTTP] GET... code: %d\n", httpCode);

                if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
                {
                    String payload = http.getString();
                    SERIAL_PRINTLN("Response:");
                    SERIAL_PRINTLN(payload);

                    // Decrypt data into resp_today and resp_tomorrow
                    decrypt_weather(payload);
                }
            }
            else
            {
                SERIAL_PRINTF("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            }

            http.end();
        }
        else
        {
            SERIAL_PRINTLN("[HTTP] Unable to connect");
        }
    }
}

void Task::reset_read_state()
{
    _read_state = ReadState::EXPECT_HEADER1;
    _bytes_left = 0;
    _next_ptr = _rx_packet.ptr();
}

void Task::decrypt_packet()
{
    uint8_t byte;
    uint16_t size = _cbuf.available();
    for (uint16_t i = 0; i < size; ++i)
    {
        if (_cbuf.readByte(byte))
        {
            switch (_read_state)
            {
            case ReadState::EXPECT_HEADER1:
            {
                // Expect header 0xAA
                if (byte == 0xAA)
                {
                    *_next_ptr = byte;
                    ++_next_ptr;
                    _read_state = ReadState::EXPECT_HEADER2;
                }
            } break;
            case ReadState::EXPECT_HEADER2:
            {
                // Expect header 0x55
                if (byte == 0x55)
                {
                    *_next_ptr = byte;
                    ++_next_ptr;
                    _read_state = ReadState::EXPECT_ADDR;
                }
                else
                {
                    reset_read_state();
                }
            } break;
            case ReadState::EXPECT_ADDR:
            {
                // Expect RX address
                *_next_ptr = byte;
                ++_next_ptr;
                _read_state = ReadState::EXPECT_LENGTH1;
            } break;
            case ReadState::EXPECT_LENGTH1:
            {
                // Expect 1st byte of length
                *_next_ptr = byte;
                ++_next_ptr;
                _read_state = ReadState::EXPECT_LENGTH2;
            } break;
            case ReadState::EXPECT_LENGTH2:
            {
                // Expect 2nd byte of length
                *_next_ptr = byte;
                ++_next_ptr;

                // Check for size exceeding
                if (_rx_packet.payload_len > v1::Packet::__max_payload_len)
                {
                    // Size exceeds maximum limit, must be something wrong
                    reset_read_state();
                }
                else
                {
                    _bytes_left = _rx_packet.payload_len + 1; // +1 for reading checksum
                    _read_state = ReadState::EXPECT_DATA;
                }
            } break;
            case ReadState::EXPECT_DATA:
            {
                // Expect data (or checksum)
                if (_bytes_left > 0)
                {
                    // Continue push packet data
                    *_next_ptr = byte;
                    ++_next_ptr;
                    --_bytes_left;

                    if (_bytes_left == 0)
                    {
                        // A complete packet is received, decode
                        if (!_rx_packet.unpack(_req))
                        {
                            _req.type = McuRequest::NO_REQUEST;
                            // switch(_req.type)
                            // {
                            // case McuRequest::NETWORK_STATUS:
                            //     resp_ready();
                            //     break;
                            // case McuRequest::DATETIME:
                            //     resp_datetime();
                            //     break;
                            // case McuRequest::WEATHER:
                            //     resp_weather();
                            //     break;
                            // }
                        }

                        // reset read state to default
                        reset_read_state();
                    }
                }
            } break;

            default:
                // Unknown error, force to default
                reset_read_state();
            }
        }
    }
}

void Task::decrypt_weather(String& payload)
{
    // Parse JSON
    DeserializationError error = deserializeJson(_json, payload);

    if (error)
    {
        SERIAL_PRINT("Error parsing JSON: ");
        SERIAL_PRINTLN(error.c_str());
    }
    else
    {
        ;
    }
}
