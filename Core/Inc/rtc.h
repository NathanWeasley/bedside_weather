#pragma once

#include "main.h"

typedef struct
{
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t weekday;
} date_time_t;

void MX_RTC_Init();

void MX_RTC_Get(date_time_t * pdatetime);
void MX_RTC_Set(const date_time_t * pdatetime);

