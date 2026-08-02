#include "tasks/main_task.h"
#include "tasks/comm_task.h"
#include "tasks/display_task.h"

#include "rtc.h"

namespace
{

/* MainTask 每 20 ms 运行一次。 */
constexpr uint32_t INITIAL_REQUEST_DELAY_TICKS = 50U;      // 上电 1 秒后开始请求
constexpr uint32_t INTER_REQUEST_GAP_TICKS = 5U;           // 不同请求至少间隔 100 ms
constexpr uint32_t RESPONSE_TIMEOUT_TICKS = 500U;          // 丢包时 10 秒后重试
constexpr uint32_t STATUS_PERIOD_TICKS = 1500U;             // 联网时每 30 秒确认状态
constexpr uint32_t STATUS_RETRY_TICKS = 500U;               // 断网时每 10 秒确认状态
constexpr uint32_t DATETIME_PERIOD_TICKS = 180000U;         // 有效时间每小时校准一次
constexpr uint32_t DATETIME_RETRY_TICKS = 1500U;            // 时间无效时每 30 秒重试
constexpr uint32_t WEATHER_PERIOD_TICKS = 90000U;           // 有效天气每 30 分钟请求一次
constexpr uint32_t WEATHER_RETRY_TICKS = 15000U;            // 无有效天气时每 5 分钟重试

constexpr uint8_t WEATHER_KNOWN_STATUS_MASK =
    bedside::WEATHER_DAILY_VALID |
    bedside::WEATHER_HOURLY_VALID |
    bedside::WEATHER_STALE |
    bedside::WEATHER_PARTIAL_DAY |
    bedside::WEATHER_NETWORK_ERROR |
    bedside::WEATHER_HTTP_ERROR |
    bedside::WEATHER_JSON_ERROR;

void tick_down(uint32_t& ticks)
{
    if (ticks > 0U)
    {
        --ticks;
    }
}

bool is_valid_date(uint16_t year, uint8_t month, uint8_t day)
{
    static constexpr uint8_t DAYS_IN_MONTH[12] =
    {
        31U, 28U, 31U, 30U, 31U, 30U,
        31U, 31U, 30U, 31U, 30U, 31U
    };

    if ((year < 2000U) || (year > 2099U) ||
        (month < 1U) || (month > 12U) || (day < 1U))
    {
        return false;
    }

    uint8_t days = DAYS_IN_MONTH[month - 1U];
    if ((month == 2U) && ((year % 4U) == 0U))
    {
        ++days;
    }
    return day <= days;
}

bool is_valid_weather_type(bedside::WeatherType weather)
{
    const uint8_t value = static_cast<uint8_t>(weather);
    return (value <= static_cast<uint8_t>(bedside::WeatherType::HOT)) ||
           (value == static_cast<uint8_t>(bedside::WeatherType::UNKNOWN));
}

bool is_valid_datetime(const bedside::DateTimeResponse& response)
{
    return is_valid_date(response.year, response.month, response.day) &&
           (response.weekday >= 1U) && (response.weekday <= 7U) &&
           (response.hour <= 23U) &&
           (response.minute <= 59U) &&
           (response.second <= 59U);
}

bool is_valid_weather(const bedside::WeatherResponse& response)
{
    if ((response.status & static_cast<uint8_t>(~WEATHER_KNOWN_STATUS_MASK)) != 0U)
    {
        return false;
    }
    if ((response.daily_count > bedside::DAILY_FORECAST_COUNT) ||
        (response.change_count > bedside::HOURLY_CHANGE_COUNT))
    {
        return false;
    }
    if (((response.status & bedside::WEATHER_DAILY_VALID) != 0U) &&
        (response.daily_count == 0U))
    {
        return false;
    }

    for (uint8_t i = 0U; i < response.daily_count; ++i)
    {
        const bedside::DailyForecast& daily = response.daily[i];
        if (!is_valid_date(daily.year, daily.month, daily.day) ||
            !is_valid_weather_type(daily.weather_day) ||
            !is_valid_weather_type(daily.weather_night))
        {
            return false;
        }
    }

    /* 小时预报当前不参与业务，仅检查收到的数据不会越界。 */
    for (uint8_t i = 0U; i < response.change_count; ++i)
    {
        const bedside::HourlyWeatherChange& change = response.changes[i];
        if ((change.hour > 23U) || !is_valid_weather_type(change.weather))
        {
            return false;
        }
    }
    return true;
}

DisplayWeatherIcon map_weather_icon(bedside::WeatherType weather)
{
    using bedside::WeatherType;

    switch (weather)
    {
        case WeatherType::DAY_SUNNY:
        case WeatherType::NIGHT_CLEAR:
        case WeatherType::DAY_FAIR:
        case WeatherType::NIGHT_FAIR:
            return DisplayWeatherIcon::CLEAR;

        case WeatherType::CLOUDY:
        case WeatherType::DAY_PARTLY_CLOUDY:
        case WeatherType::NIGHT_PARTLY_CLOUDY:
        case WeatherType::DAY_MOSTLY_CLOUDY:
        case WeatherType::NIGHT_MOSTLY_CLOUDY:
        case WeatherType::OVERCAST:
            return DisplayWeatherIcon::CLOUDY;

        case WeatherType::THUNDERSHOWER:
        case WeatherType::THUNDERSHOWER_WITH_HAIL:
            return DisplayWeatherIcon::THUNDER;

        case WeatherType::SHOWER:
        case WeatherType::LIGHT_RAIN:
        case WeatherType::MODERATE_RAIN:
        case WeatherType::HEAVY_RAIN:
        case WeatherType::STORM:
        case WeatherType::HEAVY_STORM:
        case WeatherType::SEVERE_STORM:
        case WeatherType::ICE_RAIN:
            return DisplayWeatherIcon::RAIN;

        case WeatherType::SLEET:
        case WeatherType::SNOW_FLURRY:
        case WeatherType::LIGHT_SNOW:
        case WeatherType::MODERATE_SNOW:
        case WeatherType::HEAVY_SNOW:
        case WeatherType::SNOWSTORM:
            return DisplayWeatherIcon::SNOW;

        case WeatherType::DUST:
        case WeatherType::SAND:
        case WeatherType::DUSTSTORM:
        case WeatherType::SANDSTORM:
        case WeatherType::FOGGY:
        case WeatherType::HAZE:
            return DisplayWeatherIcon::FOG_HAZE;

        case WeatherType::WINDY:
        case WeatherType::BLUSTERY:
        case WeatherType::HURRICANE:
        case WeatherType::TROPICAL_STORM:
        case WeatherType::TORNADO:
            return DisplayWeatherIcon::WIND;

        case WeatherType::HOT:
            return DisplayWeatherIcon::HOT;
        case WeatherType::COLD:
            return DisplayWeatherIcon::COLD;
        default:
            return DisplayWeatherIcon::UNKNOWN;
    }
}

} // namespace

void MainTask::init()
{
    _response_valid_mask = 0U;
    _response_error_count = 0U;

    CommTask * const comm = CommTask::instance();
    if (!comm->register_callback(bedside::ADDR_STATUS, network_status_callback))
    {
        ++_response_error_count;
    }
    if (!comm->register_callback(bedside::ADDR_DATETIME, datetime_callback))
    {
        ++_response_error_count;
    }
    if (!comm->register_callback(bedside::ADDR_WEATHER, weather_callback))
    {
        ++_response_error_count;
    }

    _network_request_ticks = INITIAL_REQUEST_DELAY_TICKS;
    _datetime_request_ticks = INITIAL_REQUEST_DELAY_TICKS;
    _weather_request_ticks = INITIAL_REQUEST_DELAY_TICKS;
    _request_gap_ticks = 0U;
    _last_display_second = 0xFFU;
    _display_dirty = true;
}

void MainTask::tick()
{
    service_requests();

    if (((_response_valid_mask & RESPONSE_DATETIME_VALID) != 0U) &&
        (_datetime.status == 1U))
    {
        date_time_t rtc_time = {};
        MX_RTC_Get(&rtc_time);
        if (rtc_time.second != _last_display_second)
        {
            _last_display_second = rtc_time.second;
            _display_dirty = true;
        }
    }

    if (_display_dirty)
    {
        prepare_display_info();
    }
}

bool MainTask::send_request(bedside::McuRequest type)
{
    if ((type != bedside::McuRequest::NETWORK_STATUS) &&
        (type != bedside::McuRequest::DATETIME) &&
        (type != bedside::McuRequest::WEATHER))
    {
        return false;
    }

    const bedside::McuRequestPayload request = {type};
    if (!CommTask::instance()->send_pack(bedside::ADDR_REQUEST, request))
    {
        return false;
    }

    switch (type)
    {
        case bedside::McuRequest::NETWORK_STATUS:
            _network_request_ticks = RESPONSE_TIMEOUT_TICKS;
            break;
        case bedside::McuRequest::DATETIME:
            _datetime_request_ticks = RESPONSE_TIMEOUT_TICKS;
            break;
        case bedside::McuRequest::WEATHER:
            _weather_request_ticks = RESPONSE_TIMEOUT_TICKS;
            break;
        default:
            break;
    }
    _request_gap_ticks = INTER_REQUEST_GAP_TICKS;
    return true;
}

void MainTask::service_requests()
{
    tick_down(_network_request_ticks);
    tick_down(_datetime_request_ticks);
    tick_down(_weather_request_ticks);
    tick_down(_request_gap_ticks);

    if (_request_gap_ticks != 0U)
    {
        return;
    }

    if (_network_request_ticks == 0U)
    {
        (void)send_request(bedside::McuRequest::NETWORK_STATUS);
    }
    else if (_datetime_request_ticks == 0U)
    {
        (void)send_request(bedside::McuRequest::DATETIME);
    }
    else if (_weather_request_ticks == 0U)
    {
        (void)send_request(bedside::McuRequest::WEATHER);
    }
}

void MainTask::network_status_callback(const v1::Packet& packet)
{
    instance()->handle_network_status(packet);
}

void MainTask::datetime_callback(const v1::Packet& packet)
{
    instance()->handle_datetime(packet);
}

void MainTask::weather_callback(const v1::Packet& packet)
{
    instance()->handle_weather(packet);
}

void MainTask::handle_network_status(const v1::Packet& packet)
{
    bedside::NetworkStatusResponse response = {};
    if (!packet.unpack(response) ||
        (static_cast<uint8_t>(response.status) >
         static_cast<uint8_t>(bedside::NetworkCondition::HTTP_ERROR)))
    {
        ++_response_error_count;
        return;
    }

    _network_status = response;
    _response_valid_mask |= RESPONSE_NETWORK_VALID;
    _network_request_ticks =
        (response.status == bedside::NetworkCondition::CONNECTED) ?
        STATUS_PERIOD_TICKS : STATUS_RETRY_TICKS;
    _display_dirty = true;
}

void MainTask::handle_datetime(const v1::Packet& packet)
{
    bedside::DateTimeResponse response = {};
    if (!packet.unpack(response) || (response.status > 1U) ||
        ((response.status == 1U) && !is_valid_datetime(response)))
    {
        ++_response_error_count;
        return;
    }

    _datetime = response;
    _response_valid_mask |= RESPONSE_DATETIME_VALID;

    if (response.status == 1U)
    {
        const date_time_t rtc_time =
        {
            static_cast<uint8_t>(response.year - 2000U),
            response.month,
            response.day,
            response.hour,
            response.minute,
            response.second,
            response.weekday
        };
        MX_RTC_Set(&rtc_time);
        _datetime_request_ticks = DATETIME_PERIOD_TICKS;
    }
    else
    {
        _datetime_request_ticks = DATETIME_RETRY_TICKS;
    }
    _display_dirty = true;
}

void MainTask::handle_weather(const v1::Packet& packet)
{
    bedside::WeatherResponse response = {};
    if (!packet.unpack(response) || !is_valid_weather(response))
    {
        ++_response_error_count;
        return;
    }

    _weather = response;
    _response_valid_mask |= RESPONSE_WEATHER_VALID;
    _weather_request_ticks =
        ((response.status & bedside::WEATHER_DAILY_VALID) != 0U) ?
        WEATHER_PERIOD_TICKS : WEATHER_RETRY_TICKS;
    _display_dirty = true;
}

void MainTask::prepare_display_info()
{
    DisplayInfo info = {};
    info.weather_icon = DisplayWeatherIcon::UNKNOWN;

    if ((_response_valid_mask & RESPONSE_NETWORK_VALID) != 0U)
    {
        info.status |= DISPLAY_NETWORK_KNOWN;
        if (_network_status.status != bedside::NetworkCondition::DISCONNECTED)
        {
            info.status |= DISPLAY_NETWORK_CONNECTED;
        }
        if (_network_status.status == bedside::NetworkCondition::HTTP_ERROR)
        {
            info.status |= DISPLAY_HTTP_ERROR;
        }
    }

    date_time_t rtc_time = {};
    if (((_response_valid_mask & RESPONSE_DATETIME_VALID) != 0U) &&
        (_datetime.status == 1U))
    {
        MX_RTC_Get(&rtc_time);
        info.status |= DISPLAY_TIME_VALID;
        info.hour = rtc_time.hour;
        info.minute = rtc_time.minute;
    }

    if ((_response_valid_mask & RESPONSE_WEATHER_VALID) != 0U)
    {
        if ((_weather.status & bedside::WEATHER_STALE) != 0U)
        {
            info.status |= DISPLAY_WEATHER_STALE;
        }
        if ((_weather.status & (bedside::WEATHER_NETWORK_ERROR |
                                bedside::WEATHER_HTTP_ERROR |
                                bedside::WEATHER_JSON_ERROR)) != 0U)
        {
            info.status |= DISPLAY_WEATHER_ERROR;
        }

        if (((_weather.status & bedside::WEATHER_DAILY_VALID) != 0U) &&
            (_weather.daily_count > 0U))
        {
            const bedside::DailyForecast& daily = _weather.daily[0];
            info.status |= DISPLAY_WEATHER_VALID;
            info.temp_max_x10 = daily.temp_max_x10;
            info.temp_min_x10 = daily.temp_min_x10;

            const bool daytime = ((info.status & DISPLAY_TIME_VALID) == 0U) ||
                                 ((info.hour >= 6U) && (info.hour < 18U));
            info.weather_icon = map_weather_icon(daytime ?
                                                 daily.weather_day :
                                                 daily.weather_night);
        }
    }

    DisplayTask::instance()->set_display_info(info);
    _display_dirty = false;
}
