#pragma once

#include <cstdint>

namespace bedside
{

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

constexpr uint8_t DAILY_FORECAST_COUNT = 3;
constexpr uint8_t HOURLY_CHANGE_COUNT = 11;

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
    uint8_t status;
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
    uint8_t status;
    uint8_t daily_count;
    DailyForecast daily[DAILY_FORECAST_COUNT];
    uint8_t change_count;
    HourlyWeatherChange changes[HOURLY_CHANGE_COUNT];
    uint32_t updated_at;
};

#pragma pack(pop)

static_assert(sizeof(McuRequestPayload) == 1, "请求载荷尺寸异常");
static_assert(sizeof(DateTimeResponse) == 9, "时间载荷尺寸异常");
static_assert(sizeof(DailyForecast) == 10, "逐日天气载荷尺寸异常");
static_assert(sizeof(HourlyWeatherChange) == 4, "逐小时变化载荷尺寸异常");
static_assert(sizeof(WeatherResponse) == 81, "天气载荷尺寸异常");

} // namespace bedside
