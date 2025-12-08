#include "rtc.h"

void MX_RTC_Init()
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  LL_RTC_InitTypeDef RTC_InitStruct = {0};
  LL_RTC_TimeTypeDef RTC_TimeStruct = {0};
  LL_RTC_DateTypeDef RTC_DateStruct = {0};
  LL_RTC_AlarmTypeDef RTC_AlarmStruct = {0};

  if(LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSI)
  {
    LL_RCC_ForceBackupDomainReset();
    LL_RCC_ReleaseBackupDomainReset();
    LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);
  }

  /* Peripheral clock enable */
  LL_RCC_EnableRTC();
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_RTC);

  /* RTC interrupt Init */
  NVIC_SetPriority(RTC_TAMP_IRQn, 1);
  NVIC_EnableIRQ(RTC_TAMP_IRQn);

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  RTC_InitStruct.HourFormat = LL_RTC_HOURFORMAT_24HOUR;
  RTC_InitStruct.AsynchPrescaler = 127;
  RTC_InitStruct.SynchPrescaler = 255;
  LL_RTC_Init(RTC, &RTC_InitStruct);
  RTC_TimeStruct.Hours = 0x11;
  RTC_TimeStruct.Minutes = 0x45;
  RTC_TimeStruct.Seconds = 0x14;

  LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BCD, &RTC_TimeStruct);
  RTC_DateStruct.WeekDay = LL_RTC_WEEKDAY_SATURDAY;
  RTC_DateStruct.Month = LL_RTC_MONTH_NOVEMBER;
  RTC_DateStruct.Day = 0x29;
  RTC_DateStruct.Year = 0x25;

  LL_RTC_DATE_Init(RTC, LL_RTC_FORMAT_BCD, &RTC_DateStruct);

  /** Enable the Alarm A
  */
  RTC_AlarmStruct.AlarmTime.Hours = 0x08;
  RTC_AlarmStruct.AlarmTime.Minutes = 0x00;
  RTC_AlarmStruct.AlarmTime.Seconds = 0x00;
  RTC_AlarmStruct.AlarmMask = LL_RTC_ALMA_MASK_NONE;
  RTC_AlarmStruct.AlarmDateWeekDaySel = LL_RTC_ALMA_DATEWEEKDAYSEL_DATE;
  RTC_AlarmStruct.AlarmDateWeekDay = 0x1;
  LL_RTC_ALMA_Init(RTC, LL_RTC_FORMAT_BCD, &RTC_AlarmStruct);
  LL_RTC_EnableIT_ALRA(RTC);
  LL_RTC_DisableAlarmPullUp(RTC);
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

void MX_RTC_Get(date_time_t * pdatetime)
{
    pdatetime->year = LL_RTC_DATE_GetYear(RTC);
    pdatetime->month = LL_RTC_DATE_GetMonth(RTC);
    pdatetime->day = LL_RTC_DATE_GetDay(RTC);
    pdatetime->weekday = LL_RTC_DATE_GetWeekDay(RTC);

    pdatetime->hour = LL_RTC_TIME_GetHour(RTC);
    pdatetime->minute = LL_RTC_TIME_GetMinute(RTC);
    pdatetime->second = LL_RTC_TIME_GetSecond(RTC);
}

void MX_RTC_Set(const date_time_t * pdatetime)
{
    LL_RTC_DATE_SetYear(RTC, pdatetime->year);
    LL_RTC_DATE_SetMonth(RTC, pdatetime->month);
    LL_RTC_DATE_SetDay(RTC, pdatetime->day);
    LL_RTC_DATE_SetWeekDay(RTC, pdatetime->weekday);

    LL_RTC_TIME_SetHour(RTC, pdatetime->hour);
    LL_RTC_TIME_SetMinute(RTC, pdatetime->minute);
    LL_RTC_TIME_SetSecond(RTC, pdatetime->second);
}
