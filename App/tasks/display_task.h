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

class DisplayTask
    : public TaskBase
{
    using Base = TaskBase;

    static RxData  _rxdata;

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


