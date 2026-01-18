#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#include <unordered_map>
#include <cstring>

#define WIFI_SSID     "NathansHome"
#define WIFI_PASSWORD "doge2048"
// #define API_FORCAST   "forecast"
// #define API_INSTANT   "weather"
#define API_URL       "http://api.seniverse.com/v3/weather/daily.json?key="
// #define API_CITY_ID   "1790630"
// #define API_COUNTRY   "CN"
#define API_APPID     "S20jWM82E5Odj8wf4"
#define API_SUFFIX    "&location=xian&language=en&unit=c"

#define DEBUG

#ifdef DEBUG
#define SERIAL_PRINT(...)   Serial.print(__VA_ARGS__)
#define SERIAL_PRINTF(...)  Serial.printf(__VA_ARGS__)
#define SERIAL_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define SERIAL_PRINT(...)
#define SERIAL_PRINTF(...)
#define SERIAL_PRINTLN(...)
#endif

enum class WeatherType
  : uint8_t
{
  DAY_SUNNY = 0,
  NIGHT_CLEAR,
  DAY_FAIR,
  NIGHT_FAIR,
  CLOUDY,
  DAY_PARTLY_CLOUDY,
  NIGHT_PARTLY_CLOUDY,
  DAY_MOSTLY_CLOUDY,
  NIGHT_MOSTLY_CLOUDY,
  OVERCAST,
  SHOWER,
  THUNDERSHOWER,
  THUNDERSHOWER_WITH_HAIL,
  LIGHT_RAIN,
  MODERATE_RAIN,
  HEAVY_RAIN,
  STORM,
  HEAVY_STORM,
  SEVERE_STORM,
  ICE_RAIN,
  SLEET,
  SNOW_FLURRY,
  LIGHT_SNOW,
  MODERATE_SNOW,
  HEAVY_SNOW,
  SNOWSTORM,
  DUST,
  SAND,
  DUSTSTORM,
  SANDSTORM,
  FOGGY,
  HAZE,
  WINDY,
  BLUSTERY,
  HURRICANE,
  TROPICAL_STORM,
  TORNADO,
  COLD,
  HOT,
  UNKNOWN = 99
};

enum class McuRequest
  : uint8_t
{
  NO_REQUEST = 0,
  NETWORK_STATUS,
  DATETIME,
  WEATHER
};

enum class NetworkCondition
  : uint8_t
{
  DISCONNECTED = 0,
  CONNECTED,
  HTTP_ERR
};

template <uint16_t Size>
struct CircularBuffer
{
    uint8_t buffer[Size];
    uint16_t head;
    uint16_t tail;

    CircularBuffer()
    : buffer{0}
    , head(0)
    , tail(0)
    {}

    inline bool isEmpty() const
    {
        return head == tail;
    }

    inline uint16_t available() const
    {
        if (head >= tail)
        {
            return head - tail;
        }
        else
        {
            return Size - tail + head;
        }
    }

    bool readByte(uint8_t& byte)
    {
        if (isEmpty())
        {
            return false;
        }

        byte = buffer[tail];
        tail = (tail + 1) % Size;

        return true;
    }

    void writeByte(uint8_t byte)
    {
        buffer[head] = byte;
        head = (head + 1) % Size;

        if (head == tail)
        {
            tail = (tail + 1) % Size;
        }
    }
};

struct TimeDate
{
  uint16_t _year;
  uint16_t _month;
  uint16_t _day;
  uint16_t _weekday;
  uint16_t _hour;
  uint16_t _minute;
  uint16_t _second;

  TimeDate()
  : _year(1970)
  , _month(0)
  , _day(0)
  , _weekday(0)
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

    // Weekday
    _weekday = (_day + 3) % 7;

    // Seconds left
    _second = utc % __utc_sec_per_day;

    // Hours left
    _hour = _second / 3600;
    _second %= 3600;

    // Minute left
    _minute = _second / 60;
    _second %= 60;

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

    _month += 1;
    _day += 1;
    _weekday += 1;
  }

private:
  static constexpr uint32_t __utc_sec_per_day = 86400;
  static constexpr uint32_t __utc_days_in_normal_year = 365;
  static constexpr uint32_t __utc_days_in_leap_year = 366;

  static inline bool is_leap_year(uint32_t year)
  {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
  }

  static inline uint32_t days_in_year(uint32_t year)
  {
    return is_leap_year(year) ? __utc_days_in_leap_year : __utc_days_in_normal_year;
  }
};

#pragma pack(1)

struct McuReq
{
  McuRequest type;
};

struct NetworkStatusResp
{
  NetworkCondition status;
};

struct TimeDateResp
{
  uint8_t status;
  uint8_t year;
  uint8_t month;
  uint8_t day;
  uint8_t weekday;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

struct WeatherDataResp
{
  uint8_t status;
  uint32_t temp_max_x10;
  uint32_t temp_min_x10;
  WeatherType weather_code_0;
  WeatherType weather_code_1;
};

#pragma pack()

#ifndef V1_MAX_PACKET_LEN
#define V1_MAX_PACKET_LEN   (1024)
#endif

#define V1_FSTART_LEN       (2)         // 0xAA 0x55
#define V1_SIZE_LEN         (2)         // 2 bytes for packet length (0-1023)
#define V1_MAX_ADDR_LEN     (1)         // 1 byte for rx address (0-255)
// #define V1_FSTOP_LEN        (1)         // 0x5A (not used)
#define V1_CHECKSUM_LEN     (1)         // Frame checksum
#define V1_HEADER_LEN       (V1_FSTART_LEN + V1_SIZE_LEN + V1_MAX_ADDR_LEN)

// ----------------------------
// |  2 bytes  | 1 byte |  2 bytes  | N bytes | 1 byte |
// | 0xAA 0x55 |  ADDR  | LENH LENL | PAYLOAD | CHKSUM |
//                            ^ ---  <------->              LEN only represent payload size
//              <----------------------------> --- ^

namespace v1
{

#pragma pack(1)
struct Packet
{
    static constexpr size_t __data_buffer_len = V1_MAX_PACKET_LEN - V1_HEADER_LEN;
    static constexpr size_t __max_payload_len = __data_buffer_len - V1_CHECKSUM_LEN;

    uint16_t header;
    uint8_t addr;
    uint16_t payload_len;
    uint8_t data[__data_buffer_len];    // checksum is implicitly included in data field

    Packet()
    : header(0x55AA)
    , addr(0)
    , payload_len(0)
    , data{0}
    {}

    inline uint16_t packet_size() const
    {
        return payload_len + V1_HEADER_LEN + V1_CHECKSUM_LEN;
    }

    inline uint8_t * ptr()
    {
        return reinterpret_cast<uint8_t *>(&header);
    }

    inline uint16_t raw_in(const uint8_t * raw, uint16_t len)
    {
        uint16_t real_len = len > V1_MAX_PACKET_LEN ? V1_MAX_PACKET_LEN : len;
        memcpy(ptr(), raw, real_len);

        return real_len;
    }

    Packet& operator=(const Packet& other)
    {
        header = other.header;
        addr = other.addr;
        payload_len = other.payload_len;

        // Only copy necessary data to save time (payload + checksum)
        memcpy(data, other.data, payload_len + 1);

        return *this;
    }

    /**
     * Pack user-defined data into packet
     */
    template <typename T>
    void pack(const T& payload, uint8_t rx_addr)
    {
        static_assert(sizeof(T) <= __max_payload_len, "<V1> Payload exceeds packet limit!");
        static_assert(alignof(T) == 1, "<V1> Payload type must be 1-byte aligned to avoid padding!");

        uint16_t datalen = sizeof(T);
        const uint8_t * ptr = reinterpret_cast<const uint8_t *>(&payload);

        addr = rx_addr;
        payload_len = datalen;
        memcpy(data, ptr, datalen);

        // Calculate checksum
        uint8_t sum = addr;
        sum += static_cast<uint8_t>(payload_len >> 8);
        sum += static_cast<uint8_t>(payload_len & 0x00FF);
        for (uint16_t i = 0; i < payload_len; ++i)
        {
            sum += *ptr;
            ++ptr;
        }

        // Checksum
        *ptr = sum;
    }

    /**
     * Unpack the content of packet into user-defined type.
     * Make sure to copy all data to address pointed by ptr()
     *   before calling unpack<...>(...).
     */
    template <typename T>
    bool unpack(T& payload, uint8_t& rx_addr)
    {
        static_assert(sizeof(T) <= __max_payload_len, "<V1> Payload exceeds packet limit!");
        static_assert(alignof(T) == 1, "<V1> Payload type must be 1-byte aligned to avoid padding!");

        if ((sizeof(T) != payload_len))
        {
            return false;
        }

        uint8_t checksum = addr;
        checksum += static_cast<uint8_t>(payload_len >> 8);
        checksum += static_cast<uint8_t>(payload_len & 0x00FF);

        uint8_t * ptr = data;
        for (uint16_t i = 0; i < payload_len; ++i)
        {
            checksum += *ptr;
            ++ptr;
        }

        if (checksum != *ptr)
        {
            return false;
        }

        memcpy(&payload, data, sizeof(T));
        rx_addr = addr;

        return true;
    }

    template <typename T>
    bool unpack(T& payload)
    {
        uint8_t dummy;
        return unpack(payload, dummy);
    }
};
#pragma pack()

}   // V1

class Task
{
    // Arduino framework
    ESP8266WiFiMulti    _wifi_multi;
    String              _weather_url;
    String              _time_url;

    // Communication with MCU
    enum class ReadState
        : uint8_t
    {
        EXPECT_HEADER1 = 0,
        EXPECT_HEADER2,
        EXPECT_ADDR,
        EXPECT_LENGTH1,
        EXPECT_LENGTH2,
        EXPECT_DATA
    };
    ReadState           _read_state;
    uint16_t            _bytes_left;
    uint8_t *           _next_ptr;
    v1::Packet          _rx_packet;
    v1::Packet          _tx_packet;
    CircularBuffer<64>  _cbuf;
    JsonDocument        _json;
    McuReq              _req;
    NetworkStatusResp   _status_resp;
    TimeDateResp        _timedate_resp;
    WeatherDataResp     _weather_resp_today;
    WeatherDataResp     _weather_resp_tomorrow;

public:
    Task()
    : _next_ptr(nullptr)
    , _json()
    {
        _req.type = McuRequest::NO_REQUEST;
    }

    void init()
    {
        Serial.begin(115200);
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

        _req.type = McuRequest::WEATHER;
    }


    void tick()
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
            // resp_ready();
        } break;

        case McuRequest::WEATHER:
        {
            fetch_weather();
            // resp_weather();
        } break;

        case McuRequest::DATETIME:
        {
            fetch_time();
            // resp_datetime();
        } break;

        default:
          break;
        }

        _req.type = McuRequest::NO_REQUEST;

        delay(1000);
    }

    void fetch_time()
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
                        // decrypt_time(payload);
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

    void fetch_weather()
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

    void reset_read_state()
    {
        _read_state = ReadState::EXPECT_HEADER1;
        _bytes_left = 0;
        _next_ptr = _rx_packet.ptr();
    }

    void decrypt_packet()
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

    void decrypt_weather(String& payload)
    {
        // Parse JSON
        auto error = deserializeJson(_json, payload);

        if (error)
        {
            SERIAL_PRINT("Error parsing JSON: ");
            SERIAL_PRINTLN(error.c_str());
        }
        else
        {
            auto&& res = _json["results"][0]["daily"];
            auto sz = res.size();
            for (int i = 0; i < sz; ++i)
            {
                auto&& obj = res[i];

                // below are debug code
                SERIAL_PRINTLN(obj["date"].as<String>());
                SERIAL_PRINTLN(obj["text_day"].as<String>());
                SERIAL_PRINTLN(obj["code_day"].as<String>());
                SERIAL_PRINTLN(obj["text_night"].as<String>());
                SERIAL_PRINTLN(obj["code_night"].as<String>());
                SERIAL_PRINTLN(obj["high"].as<String>());
                SERIAL_PRINTLN(obj["low"].as<String>());
                SERIAL_PRINTLN(obj["rainfall"].as<String>());
                SERIAL_PRINTLN(obj["precip"].as<String>());
                SERIAL_PRINTLN(obj["wind_direction"].as<String>());
                SERIAL_PRINTLN(obj["wind_direction_degree"].as<String>());
                SERIAL_PRINTLN(obj["wind_speed"].as<String>());
                SERIAL_PRINTLN(obj["wind_scale"].as<String>());
                SERIAL_PRINTLN(obj["humidity"].as<String>());
            }
        }
    }
};

Task mytask;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  mytask.init();
}

void loop() {
  // put your main code here, to run repeatedly:
  mytask.tick();
}
