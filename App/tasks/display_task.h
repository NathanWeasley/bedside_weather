#pragma once

#include "tasks/task.h"
#include "v1/v1.hpp"

#define DISPLAY_TASK_PRESC             (5)

using CBFunc = void (*)(const v1::Packet&);

#pragma pack(1)
struct RxData
{
    char str[6];
};
#pragma pack()

enum DisplayInfoStatus : uint8_t
{
    DISPLAY_NETWORK_KNOWN = 1U << 0,
    DISPLAY_NETWORK_CONNECTED = 1U << 1,
    DISPLAY_HTTP_ERROR = 1U << 2,
    DISPLAY_TIME_VALID = 1U << 3,
    DISPLAY_WEATHER_VALID = 1U << 4,
    DISPLAY_WEATHER_STALE = 1U << 5,
    DISPLAY_WEATHER_ERROR = 1U << 6,
};

enum class DisplayWeatherIcon : uint8_t
{
    UNKNOWN = 0,
    CLEAR,
    CLOUDY,
    RAIN,
    THUNDER,
    SNOW,
    FOG_HAZE,
    WIND,
    HOT,
    COLD,
};

/* MainTask 已整理好的显示数据，不包含 ESP8266 协议细节。 */
struct DisplayInfo
{
    uint8_t status;
    uint8_t hour;
    uint8_t minute;
    int16_t temp_max_x10;
    int16_t temp_min_x10;
    DisplayWeatherIcon weather_icon;
};

class DisplayTask
    : public TaskBase
{
    using Base = TaskBase;

    static RxData  _rxdata;
    static DisplayInfo _display_info;

     DisplayTask()
    : Base(DISPLAY_TASK_PRESC)
    {}
    ~DisplayTask() = default;

public:
    static inline DisplayTask * instance()
    {
        static DisplayTask tsk;
        return &tsk;
    }

    void init() override;
    void tick() override;

    void set_display_info(const DisplayInfo& info);

    void set_string(uint8_t slot, const char * str);
    void set_icon(uint8_t icon_id);

    static void message_callback(const v1::Packet&);
};





/**
 * Task APIs 
 */

// #define WIFI_SSID     "ghq"
// #define WIFI_PASSWORD "gghhqq1963"
// #define API_URL       "https://api.openweathermap.org/data/2.5/forecast?id="
// #define API_CITY_ID   "1790630"
// #define API_COUNTRY   "CN"
// #define API_APPID     "4da28fda20eca6cb0e9a8a6b5da9002d"

// #define API_FULL_URL  API_URL API_CITY_ID ## "&appid=" ## API_APPID

// const char url[] = API_FULL_URL;


