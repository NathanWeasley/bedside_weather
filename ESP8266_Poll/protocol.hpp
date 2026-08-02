#pragma once

#include <Arduino.h>
#include <cstring>

namespace bedside
{

constexpr uint8_t ADDR_REQUEST = 0;
constexpr uint8_t ADDR_STATUS = 10;
constexpr uint8_t ADDR_DATETIME = 11;
constexpr uint8_t ADDR_WEATHER = 12;

enum class McuRequest : uint8_t
{
    NONE = 0,
    NETWORK_STATUS = 1,
    DATETIME = 2,
    WEATHER = 3,
};

enum class NetworkCondition : uint8_t
{
    DISCONNECTED = 0,
    CONNECTED = 1,
    HTTP_ERROR = 2,
};

// 心知天气现象代码。数值与 API 原始 code 一致，MCU 可直接据此选择图标。
enum class WeatherType : uint8_t
{
    DAY_SUNNY = 0,
    NIGHT_CLEAR = 1,
    DAY_FAIR = 2,
    NIGHT_FAIR = 3,
    CLOUDY = 4,
    DAY_PARTLY_CLOUDY = 5,
    NIGHT_PARTLY_CLOUDY = 6,
    DAY_MOSTLY_CLOUDY = 7,
    NIGHT_MOSTLY_CLOUDY = 8,
    OVERCAST = 9,
    SHOWER = 10,
    THUNDERSHOWER = 11,
    THUNDERSHOWER_WITH_HAIL = 12,
    LIGHT_RAIN = 13,
    MODERATE_RAIN = 14,
    HEAVY_RAIN = 15,
    STORM = 16,
    HEAVY_STORM = 17,
    SEVERE_STORM = 18,
    ICE_RAIN = 19,
    SLEET = 20,
    SNOW_FLURRY = 21,
    LIGHT_SNOW = 22,
    MODERATE_SNOW = 23,
    HEAVY_SNOW = 24,
    SNOWSTORM = 25,
    DUST = 26,
    SAND = 27,
    DUSTSTORM = 28,
    SANDSTORM = 29,
    FOGGY = 30,
    HAZE = 31,
    WINDY = 32,
    BLUSTERY = 33,
    HURRICANE = 34,
    TROPICAL_STORM = 35,
    TORNADO = 36,
    COLD = 37,
    HOT = 38,
    UNKNOWN = 99,
};

enum WeatherStatus : uint8_t
{
    WEATHER_DAILY_VALID = 1U << 0,
    WEATHER_HOURLY_VALID = 1U << 1,
    WEATHER_STALE = 1U << 2,
    WEATHER_PARTIAL_DAY = 1U << 3,
    WEATHER_NETWORK_ERROR = 1U << 4,
    WEATHER_HTTP_ERROR = 1U << 5,
    WEATHER_JSON_ERROR = 1U << 6,
};

constexpr size_t DAILY_FORECAST_COUNT = 3;
constexpr size_t HOURLY_CHANGE_COUNT = 11; // 08:00 至 18:00（含首尾）

#pragma pack(push, 1)

struct McuRequestPayload
{
    McuRequest type;
};

struct NetworkStatusResponse
{
    NetworkCondition status;
};

struct DateTimeResponse
{
    uint8_t status; // 1：时间有效；0：NTP 尚未同步
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t weekday; // 1=星期一，...，7=星期日
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

struct DailyForecast
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    int16_t temp_max_x10;
    int16_t temp_min_x10;
    WeatherType weather_day;
    WeatherType weather_night;
};

struct HourlyWeatherChange
{
    uint8_t hour;
    int16_t temperature_x10;
    WeatherType weather;
};

struct WeatherResponse
{
    uint8_t status; // WeatherStatus 位组合
    uint8_t daily_count;
    DailyForecast daily[DAILY_FORECAST_COUNT];
    uint8_t change_count;
    HourlyWeatherChange changes[HOURLY_CHANGE_COUNT];
    uint32_t updated_at; // Unix 时间；NTP 未同步时为 0
};

#pragma pack(pop)

static_assert(sizeof(McuRequestPayload) == 1, "请求载荷尺寸异常");
static_assert(sizeof(DateTimeResponse) == 9, "时间载荷尺寸异常");
static_assert(sizeof(DailyForecast) == 10, "逐日天气载荷尺寸异常");
static_assert(sizeof(HourlyWeatherChange) == 4, "逐小时变化载荷尺寸异常");
static_assert(sizeof(WeatherResponse) == 81, "天气载荷尺寸异常");

namespace v1
{

constexpr size_t MAX_PACKET_SIZE = 256;
constexpr size_t HEADER_SIZE = 5;
constexpr size_t CHECKSUM_SIZE = 1;
constexpr size_t DATA_BUFFER_SIZE = MAX_PACKET_SIZE - HEADER_SIZE;
constexpr size_t MAX_PAYLOAD_SIZE = DATA_BUFFER_SIZE - CHECKSUM_SIZE;

// 线格式：AA 55 | 地址 | 载荷长度（小端）| 载荷 | 8 位累加校验。
#pragma pack(push, 1)
struct Packet
{
    uint16_t header;
    uint8_t address;
    uint16_t payload_length;
    uint8_t data[DATA_BUFFER_SIZE];

    Packet()
        : header(0x55AA)
        , address(0)
        , payload_length(0)
        , data{0}
    {
    }

    uint8_t* bytes()
    {
        return reinterpret_cast<uint8_t*>(&header);
    }

    const uint8_t* bytes() const
    {
        return reinterpret_cast<const uint8_t*>(&header);
    }

    uint16_t packetSize() const
    {
        return payload_length + HEADER_SIZE + CHECKSUM_SIZE;
    }

    template <typename T>
    void pack(const T& payload, uint8_t receiver)
    {
        static_assert(sizeof(T) <= MAX_PAYLOAD_SIZE, "V1 载荷超过上限");
        static_assert(alignof(T) == 1, "V1 载荷必须按 1 字节对齐");

        address = receiver;
        payload_length = sizeof(T);
        memcpy(data, &payload, sizeof(T));

        uint8_t checksum = address;
        checksum += static_cast<uint8_t>(payload_length >> 8);
        checksum += static_cast<uint8_t>(payload_length & 0xFF);
        for (uint16_t i = 0; i < payload_length; ++i)
        {
            checksum += data[i];
        }
        data[payload_length] = checksum;
    }

    template <typename T>
    bool unpack(T& payload, uint8_t* receiver = nullptr) const
    {
        static_assert(sizeof(T) <= MAX_PAYLOAD_SIZE, "V1 载荷超过上限");
        static_assert(alignof(T) == 1, "V1 载荷必须按 1 字节对齐");

        if (payload_length != sizeof(T))
        {
            return false;
        }

        uint8_t checksum = address;
        checksum += static_cast<uint8_t>(payload_length >> 8);
        checksum += static_cast<uint8_t>(payload_length & 0xFF);
        for (uint16_t i = 0; i < payload_length; ++i)
        {
            checksum += data[i];
        }
        if (checksum != data[payload_length])
        {
            return false;
        }

        memcpy(&payload, data, sizeof(T));
        if (receiver != nullptr)
        {
            *receiver = address;
        }
        return true;
    }
};
#pragma pack(pop)

static_assert(sizeof(Packet) == MAX_PACKET_SIZE, "V1 数据包尺寸异常");

} // namespace v1
} // namespace bedside
