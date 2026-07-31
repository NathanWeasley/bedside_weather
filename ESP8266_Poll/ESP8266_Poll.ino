#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClientSecureBearSSL.h>

#include <cmath>
#include <cstdlib>
#include <ctime>

#include "protocol.hpp"
#include "secrets.h"
#include "settings.h"

using namespace bedside;

#if ENABLE_DEBUG_LOG
#define DEBUG_BEGIN(baud) Serial1.begin(baud)
#define DEBUG_PRINT(...) Serial1.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...) Serial1.println(__VA_ARGS__)
#define DEBUG_PRINTF(...) Serial1.printf(__VA_ARGS__)
#else
#define DEBUG_BEGIN(baud) ((void)(baud))
#define DEBUG_PRINT(...) ((void)0)
#define DEBUG_PRINTLN(...) ((void)0)
#define DEBUG_PRINTF(...) ((void)0)
#endif

namespace
{

enum class FetchResult : uint8_t
{
    OK = 0,
    NETWORK_ERROR,
    HTTP_ERROR,
    JSON_ERROR,
};

bool timeReached(uint32_t now, uint32_t deadline)
{
    return static_cast<int32_t>(now - deadline) >= 0;
}

bool parseIsoDateTime(const char* value, uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& hour)
{
    if (value == nullptr || strlen(value) < 13 || value[4] != '-' || value[7] != '-' || value[10] != 'T')
    {
        return false;
    }

    year = static_cast<uint16_t>(atoi(value));
    month = static_cast<uint8_t>(atoi(value + 5));
    day = static_cast<uint8_t>(atoi(value + 8));
    hour = static_cast<uint8_t>(atoi(value + 11));
    return year >= 2000 && month >= 1 && month <= 12 && day >= 1 && day <= 31 && hour <= 23;
}

bool parseIsoDate(const char* value, uint16_t& year, uint8_t& month, uint8_t& day)
{
    uint8_t ignoredHour = 0;
    if (value == nullptr || strlen(value) < 10)
    {
        return false;
    }

    char dateTime[14] = {0};
    memcpy(dateTime, value, 10);
    memcpy(dateTime + 10, "T00", 3);
    return parseIsoDateTime(dateTime, year, month, day, ignoredHour);
}

WeatherType parseWeatherCode(JsonVariantConst value)
{
    if (value.isNull())
    {
        return WeatherType::UNKNOWN;
    }

    const int code = value.as<int>();
    if (code >= static_cast<int>(WeatherType::DAY_SUNNY) && code <= static_cast<int>(WeatherType::HOT))
    {
        return static_cast<WeatherType>(code);
    }
    return WeatherType::UNKNOWN;
}

int16_t parseTemperatureX10(JsonVariantConst value)
{
    long result = lroundf(value.as<float>() * 10.0F);
    if (result < INT16_MIN)
    {
        result = INT16_MIN;
    }
    else if (result > INT16_MAX)
    {
        result = INT16_MAX;
    }
    return static_cast<int16_t>(result);
}

void applyFetchError(uint8_t& status, FetchResult result)
{
    switch (result)
    {
    case FetchResult::NETWORK_ERROR:
        status |= WEATHER_NETWORK_ERROR;
        break;
    case FetchResult::HTTP_ERROR:
        status |= WEATHER_HTTP_ERROR;
        break;
    case FetchResult::JSON_ERROR:
        status |= WEATHER_JSON_ERROR;
        break;
    default:
        break;
    }
}

} // namespace

class WeatherBridge
{
    enum class ReadState : uint8_t
    {
        HEADER_AA = 0,
        HEADER_55,
        ADDRESS,
        LENGTH_LOW,
        LENGTH_HIGH,
        DATA,
    };

    ESP8266WiFiMulti _wifi;
    v1::Packet _rxPacket;
    v1::Packet _txPacket;
    WeatherResponse _weather{};
    ReadState _readState = ReadState::HEADER_AA;
    uint8_t* _nextRxByte = nullptr;
    uint16_t _rxBytesLeft = 0;
    uint32_t _nextWifiPoll = 0;
    uint32_t _nextWeatherRefresh = 0;
    uint32_t _nextDateTimePush = 0;
    bool _ntpStarted = false;
    bool _weatherRefreshPending = true;
    bool _lastFetchFailed = false;
    bool _wasConnected = false;

public:
    void begin()
    {
        Serial.begin(115200);
        DEBUG_BEGIN(115200);

        resetPacketReader();
        WiFi.persistent(false);
        WiFi.mode(WIFI_STA);
        WiFi.setAutoReconnect(true);
        _wifi.addAP(WIFI_SSID, WIFI_PASSWORD);

        // 用 UTC 同步系统时钟，再由 TZ 规则转换为本地时间。
        setenv("TZ", DEVICE_TIMEZONE, 1);
        tzset();
        DEBUG_PRINTLN(F("ESP8266 weather bridge started"));
    }

    void tick()
    {
        processSerial();

        const uint32_t now = millis();
        if (timeReached(now, _nextWifiPoll))
        {
            _wifi.run();
            _nextWifiPoll = now + WIFI_POLL_INTERVAL_MS;
        }

        const bool connected = WiFi.status() == WL_CONNECTED;
        if (connected && !_wasConnected)
        {
            // 每次重新联网都立即刷新，避免继续展示断网前的缓存。
            _weatherRefreshPending = true;
        }
        _wasConnected = connected;

        if (connected)
        {
            if (!_ntpStarted)
            {
                configTime(0, 0, "ntp.aliyun.com", "ntp1.aliyun.com", "pool.ntp.org");
                _ntpStarted = true;
            }

            if (_weatherRefreshPending || timeReached(now, _nextWeatherRefresh))
            {
                const bool complete = refreshWeather();
                _weatherRefreshPending = false;
                _nextWeatherRefresh = now + (complete ? WEATHER_REFRESH_INTERVAL_MS : WEATHER_RETRY_INTERVAL_MS);
                sendWeather();
            }

            if (isClockValid() && timeReached(now, _nextDateTimePush))
            {
                sendDateTime();
                _nextDateTimePush = now + DATETIME_PUSH_INTERVAL_MS;
            }
        }

        yield();
    }

private:
    String makeWeatherUrl(const __FlashStringHelper* resource, const __FlashStringHelper* suffix) const
    {
        String url;
        url.reserve(192);
        url += F("https://api.seniverse.com/v3/weather/");
        url += resource;
        url += F(".json?key=");
        url += SENIVERSE_API_KEY;
        url += F("&location=");
        url += WEATHER_LOCATION;
        url += F("&language=");
        url += WEATHER_LANGUAGE;
        url += F("&unit=");
        url += WEATHER_UNIT;
        url += suffix;
        return url;
    }

    FetchResult httpGet(const String& url, String& payload)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            return FetchResult::NETWORK_ERROR;
        }

        BearSSL::WiFiClientSecure client;
        // ESP8266 未内置稳定的 CA 存储；HTTPS 仍可避免 API key 明文传输。
        // 如产品化部署，应改为 setTrustAnchors() 并随证书更新固件。
        client.setInsecure();

        HTTPClient http;
        http.setTimeout(HTTP_TIMEOUT_MS);
        if (!http.begin(client, url))
        {
            return FetchResult::HTTP_ERROR;
        }

        const int code = http.GET();
        if (code != HTTP_CODE_OK)
        {
            DEBUG_PRINTF("HTTP GET failed: %d\n", code);
            http.end();
            return code < 0 ? FetchResult::NETWORK_ERROR : FetchResult::HTTP_ERROR;
        }

        payload = http.getString();
        http.end();
        return payload.length() > 0 ? FetchResult::OK : FetchResult::HTTP_ERROR;
    }

    FetchResult fetchDaily(WeatherResponse& output)
    {
        String payload;
        FetchResult result = httpGet(makeWeatherUrl(F("daily"), F("&start=0&days=3")), payload);
        if (result != FetchResult::OK)
        {
            return result;
        }

#if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument document;
#else
        DynamicJsonDocument document(6144);
#endif
        const DeserializationError error = deserializeJson(document, payload);
        if (error)
        {
            DEBUG_PRINTF("Daily JSON error: %s\n", error.c_str());
            return FetchResult::JSON_ERROR;
        }

        JsonArrayConst daily = document["results"][0]["daily"].as<JsonArrayConst>();
        if (daily.isNull())
        {
            return FetchResult::JSON_ERROR;
        }

        output.daily_count = 0;
        for (JsonObjectConst item : daily)
        {
            if (output.daily_count >= DAILY_FORECAST_COUNT)
            {
                break;
            }

            DailyForecast& day = output.daily[output.daily_count];
            if (!parseIsoDate(item["date"].as<const char*>(), day.year, day.month, day.day) ||
                item["high"].isNull() || item["low"].isNull())
            {
                continue;
            }

            day.temp_max_x10 = parseTemperatureX10(item["high"]);
            day.temp_min_x10 = parseTemperatureX10(item["low"]);
            day.weather_day = parseWeatherCode(item["code_day"]);
            day.weather_night = parseWeatherCode(item["code_night"]);
            ++output.daily_count;
        }

        if (output.daily_count == 0)
        {
            return FetchResult::JSON_ERROR;
        }
        output.status |= WEATHER_DAILY_VALID;
        return FetchResult::OK;
    }

    FetchResult fetchHourly(WeatherResponse& output)
    {
        String payload;
        FetchResult result = httpGet(makeWeatherUrl(F("hourly"), F("&start=0&hours=24")), payload);
        if (result != FetchResult::OK)
        {
            return result;
        }

#if ARDUINOJSON_VERSION_MAJOR >= 7
        JsonDocument document;
#else
        DynamicJsonDocument document(12288);
#endif
        const DeserializationError error = deserializeJson(document, payload);
        if (error)
        {
            DEBUG_PRINTF("Hourly JSON error: %s\n", error.c_str());
            return FetchResult::JSON_ERROR;
        }

        JsonArrayConst hourly = document["results"][0]["hourly"].as<JsonArrayConst>();
        if (hourly.isNull())
        {
            return FetchResult::JSON_ERROR;
        }

        uint16_t targetYear = 0;
        uint8_t targetMonth = 0;
        uint8_t targetDay = 0;
        if (output.daily_count > 0)
        {
            targetYear = output.daily[0].year;
            targetMonth = output.daily[0].month;
            targetDay = output.daily[0].day;
        }
        else if (!getLocalDate(targetYear, targetMonth, targetDay))
        {
            const char* firstTime = hourly[0]["time"] | "";
            uint8_t ignoredHour = 0;
            if (!parseIsoDateTime(firstTime, targetYear, targetMonth, targetDay, ignoredHour))
            {
                return FetchResult::JSON_ERROR;
            }
        }

        output.change_count = 0;
        bool foundWindowData = false;
        uint8_t firstHour = 0xFF;
        uint8_t lastHour = 0;
        WeatherType previousWeather = WeatherType::UNKNOWN;

        for (JsonObjectConst item : hourly)
        {
            uint16_t year = 0;
            uint8_t month = 0;
            uint8_t day = 0;
            uint8_t hour = 0;
            if (!parseIsoDateTime(item["time"].as<const char*>(), year, month, day, hour) ||
                year != targetYear || month != targetMonth || day != targetDay || hour < 8 || hour > 18)
            {
                continue;
            }

            const WeatherType weather = parseWeatherCode(item["code"]);
            if (!foundWindowData)
            {
                firstHour = hour;
            }
            foundWindowData = true;
            lastHour = hour;

            // 只记录首个时点和天气现象发生变化的时点，避免传输重复数据。
            if (output.change_count == 0 || weather != previousWeather)
            {
                if (output.change_count < HOURLY_CHANGE_COUNT)
                {
                    HourlyWeatherChange& change = output.changes[output.change_count++];
                    change.hour = hour;
                    change.temperature_x10 = parseTemperatureX10(item["temperature"]);
                    change.weather = weather;
                }
            }
            previousWeather = weather;
        }

        output.status |= WEATHER_HOURLY_VALID;
        if (!foundWindowData || firstHour > 8 || lastHour < 18)
        {
            // 逐小时接口仅返回“当前起未来 24 小时”，午后启动时当天早间数据天然不完整。
            output.status |= WEATHER_PARTIAL_DAY;
        }
        return FetchResult::OK;
    }

    bool refreshWeather()
    {
        WeatherResponse fresh{};
        const FetchResult dailyResult = fetchDaily(fresh);
        // 心知天气的基础访问频率可能只有 1 次/秒，两类接口之间主动留出间隔。
        delay(API_MIN_REQUEST_INTERVAL_MS);
        const FetchResult hourlyResult = fetchHourly(fresh);

        if (dailyResult != FetchResult::OK && (_weather.status & WEATHER_DAILY_VALID) != 0)
        {
            fresh.daily_count = _weather.daily_count;
            memcpy(fresh.daily, _weather.daily, sizeof(fresh.daily));
            fresh.status |= WEATHER_DAILY_VALID | WEATHER_STALE;
        }
        if (hourlyResult != FetchResult::OK && (_weather.status & WEATHER_HOURLY_VALID) != 0)
        {
            fresh.change_count = _weather.change_count;
            memcpy(fresh.changes, _weather.changes, sizeof(fresh.changes));
            fresh.status |= WEATHER_HOURLY_VALID | WEATHER_STALE;
            fresh.status |= _weather.status & WEATHER_PARTIAL_DAY;
        }

        applyFetchError(fresh.status, dailyResult);
        applyFetchError(fresh.status, hourlyResult);
        fresh.updated_at = isClockValid() ? static_cast<uint32_t>(time(nullptr)) : 0;
        _weather = fresh;

        _lastFetchFailed = dailyResult != FetchResult::OK || hourlyResult != FetchResult::OK;
        DEBUG_PRINTF("Weather refreshed: daily=%u hourly=%u status=0x%02X\n",
                     _weather.daily_count, _weather.change_count, _weather.status);
        return !_lastFetchFailed;
    }

    bool isClockValid() const
    {
        return time(nullptr) >= 1700000000;
    }

    bool getLocalDate(uint16_t& year, uint8_t& month, uint8_t& day) const
    {
        if (!isClockValid())
        {
            return false;
        }

        const time_t now = time(nullptr);
        tm local{};
        localtime_r(&now, &local);
        year = static_cast<uint16_t>(local.tm_year + 1900);
        month = static_cast<uint8_t>(local.tm_mon + 1);
        day = static_cast<uint8_t>(local.tm_mday);
        return true;
    }

    void resetPacketReader()
    {
        _readState = ReadState::HEADER_AA;
        _rxBytesLeft = 0;
        _nextRxByte = _rxPacket.bytes();
    }

    void processSerial()
    {
        while (Serial.available() > 0)
        {
            consumeSerialByte(static_cast<uint8_t>(Serial.read()));
        }
    }

    void consumeSerialByte(uint8_t byte)
    {
        switch (_readState)
        {
        case ReadState::HEADER_AA:
            if (byte == 0xAA)
            {
                *_nextRxByte++ = byte;
                _readState = ReadState::HEADER_55;
            }
            break;

        case ReadState::HEADER_55:
            if (byte == 0x55)
            {
                *_nextRxByte++ = byte;
                _readState = ReadState::ADDRESS;
            }
            else
            {
                resetPacketReader();
                if (byte == 0xAA)
                {
                    *_nextRxByte++ = byte;
                    _readState = ReadState::HEADER_55;
                }
            }
            break;

        case ReadState::ADDRESS:
            *_nextRxByte++ = byte;
            _readState = ReadState::LENGTH_LOW;
            break;

        case ReadState::LENGTH_LOW:
            *_nextRxByte++ = byte;
            _readState = ReadState::LENGTH_HIGH;
            break;

        case ReadState::LENGTH_HIGH:
            *_nextRxByte++ = byte;
            if (_rxPacket.payload_length > v1::MAX_PAYLOAD_SIZE)
            {
                resetPacketReader();
            }
            else
            {
                _rxBytesLeft = _rxPacket.payload_length + v1::CHECKSUM_SIZE;
                _readState = ReadState::DATA;
            }
            break;

        case ReadState::DATA:
            *_nextRxByte++ = byte;
            if (--_rxBytesLeft == 0)
            {
                handlePacket();
                resetPacketReader();
            }
            break;
        }
    }

    void handlePacket()
    {
        McuRequestPayload request{};
        if (!_rxPacket.unpack(request))
        {
            DEBUG_PRINTLN(F("Invalid V1 packet"));
            return;
        }

        switch (request.type)
        {
        case McuRequest::NETWORK_STATUS:
            sendNetworkStatus();
            break;
        case McuRequest::DATETIME:
            sendDateTime();
            break;
        case McuRequest::WEATHER:
            sendWeather();
            if ((_weather.status & (WEATHER_DAILY_VALID | WEATHER_HOURLY_VALID)) == 0)
            {
                _weatherRefreshPending = true;
            }
            break;
        default:
            break;
        }
    }

    template <typename T>
    void sendResponse(uint8_t address, const T& payload)
    {
        _txPacket.pack(payload, address);
        Serial.write(_txPacket.bytes(), _txPacket.packetSize());
        Serial.flush();
    }

    void sendNetworkStatus()
    {
        NetworkStatusResponse response{};
        if (WiFi.status() != WL_CONNECTED)
        {
            response.status = NetworkCondition::DISCONNECTED;
        }
        else
        {
            response.status = _lastFetchFailed ? NetworkCondition::HTTP_ERROR : NetworkCondition::CONNECTED;
        }
        sendResponse(ADDR_STATUS, response);
    }

    void sendDateTime()
    {
        DateTimeResponse response{};
        if (isClockValid())
        {
            const time_t now = time(nullptr);
            tm local{};
            localtime_r(&now, &local);
            response.status = 1;
            response.year = static_cast<uint16_t>(local.tm_year + 1900);
            response.month = static_cast<uint8_t>(local.tm_mon + 1);
            response.day = static_cast<uint8_t>(local.tm_mday);
            response.weekday = static_cast<uint8_t>(((local.tm_wday + 6) % 7) + 1);
            response.hour = static_cast<uint8_t>(local.tm_hour);
            response.minute = static_cast<uint8_t>(local.tm_min);
            response.second = static_cast<uint8_t>(local.tm_sec);
        }
        sendResponse(ADDR_DATETIME, response);
    }

    void sendWeather()
    {
        WeatherResponse response = _weather;
        if (WiFi.status() != WL_CONNECTED)
        {
            response.status |= WEATHER_NETWORK_ERROR;
            if ((response.status & (WEATHER_DAILY_VALID | WEATHER_HOURLY_VALID)) != 0)
            {
                response.status |= WEATHER_STALE;
            }
        }
        sendResponse(ADDR_WEATHER, response);
    }
};

WeatherBridge weatherBridge;

void setup()
{
    weatherBridge.begin();
}

void loop()
{
    weatherBridge.tick();
}
