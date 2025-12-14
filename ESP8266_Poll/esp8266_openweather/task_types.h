#pragma once

#include <cstdint>

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

enum class WeatherType
  : uint8_t
{
  THUNDERSTORM = 1,
  DRIZZLE,
  RAIN,
  SNOW,
  MIST,
  CLEAR,
  CLOUDS,
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
