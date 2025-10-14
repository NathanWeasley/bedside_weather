#pragma once

#include "tasks/task.h"

/**
 * Task APIs 
 */

#ifdef __cplusplus
extern "C"
{
#endif

// #define WIFI_SSID     "ghq"
// #define WIFI_PASSWORD "gghhqq1963"
// #define API_URL       "https://api.openweathermap.org/data/2.5/forecast?id="
// #define API_CITY_ID   "1790630"
// #define API_COUNTRY   "CN"
// #define API_APPID     "4da28fda20eca6cb0e9a8a6b5da9002d"

// #define API_FULL_URL  API_URL API_CITY_ID ## "&appid=" ## API_APPID

// const char url[] = API_FULL_URL;

uint32_t test_task_param_size();
void test_task_init(task_param_t * param);
void test_task_tick();

void test_task_update_icon();


#ifdef __cplusplus
}
#endif
