#include "rtc.h"

#define RTC_BACKUP_MAGIC                (0x42535731UL) /* "BSW1" */
#define RTC_LSI_ASYNC_PRESCALER         (124U)        /* 125 分频 */
#define RTC_LSI_SYNC_PRESCALER          (255U)        /* 256 分频，32000 / 125 / 256 = 1Hz */

static uint8_t MX_RTC_IsDateTimeValid(const date_time_t * pdatetime)
{
    if (pdatetime == 0)
    {
        return 0;
    }

    return pdatetime->year <= 99U &&
           pdatetime->month >= 1U && pdatetime->month <= 12U &&
           pdatetime->day >= 1U && pdatetime->day <= 31U &&
           pdatetime->weekday >= 1U && pdatetime->weekday <= 7U &&
           pdatetime->hour <= 23U &&
           pdatetime->minute <= 59U &&
           pdatetime->second <= 59U;
}

void MX_RTC_Init(void)
{
    uint8_t backup_domain_reset = 0;

    LL_PWR_EnableBkUpAccess();

    if (LL_RCC_GetRTCClockSource() != LL_RCC_RTC_CLKSOURCE_LSI)
    {
        LL_RCC_ForceBackupDomainReset();
        LL_RCC_ReleaseBackupDomainReset();
        LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);
        backup_domain_reset = 1;
    }

    LL_RCC_EnableRTC();
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_RTC);

    /* 当前功能不使用 RTC Alarm，避免空中断和未清标志造成重复进入。 */
    NVIC_DisableIRQ(RTC_TAMP_IRQn);
    NVIC_ClearPendingIRQ(RTC_TAMP_IRQn);
    LL_RTC_DisableWriteProtection(RTC);
    LL_RTC_DisableIT_ALRA(RTC);
    LL_RTC_ALMA_Disable(RTC);
    LL_RTC_ClearFlag_ALRA(RTC);
    LL_RTC_EnableWriteProtection(RTC);

    if (backup_domain_reset ||
        LL_RTC_BKP_GetRegister(TAMP, LL_RTC_BKP_DR0) != RTC_BACKUP_MAGIC)
    {
        LL_RTC_InitTypeDef rtc = {0};
        LL_RTC_TimeTypeDef time = {0};
        LL_RTC_DateTypeDef date = {0};

        rtc.HourFormat = LL_RTC_HOURFORMAT_24HOUR;
        rtc.AsynchPrescaler = RTC_LSI_ASYNC_PRESCALER;
        rtc.SynchPrescaler = RTC_LSI_SYNC_PRESCALER;

        /* 首次启动使用一个合法基准值；后续系统复位不再覆盖备份域时间。 */
        time.Hours = 0;
        time.Minutes = 0;
        time.Seconds = 0;

        date.WeekDay = LL_RTC_WEEKDAY_SATURDAY;
        date.Month = 1;
        date.Day = 1;
        date.Year = 0; /* 2000-01-01 */

        if (LL_RTC_Init(RTC, &rtc) == SUCCESS &&
            LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BIN, &time) == SUCCESS &&
            LL_RTC_DATE_Init(RTC, LL_RTC_FORMAT_BIN, &date) == SUCCESS)
        {
            LL_RTC_BKP_SetRegister(TAMP, LL_RTC_BKP_DR0, RTC_BACKUP_MAGIC);
        }
    }
}

void MX_RTC_Get(date_time_t * pdatetime)
{
    if (pdatetime == 0)
    {
        return;
    }

    /* 先读 TR 再读 DR，以获得同一个 RTC shadow snapshot 并解除 shadow lock。 */
    const uint32_t rtc_time = LL_RTC_TIME_Get(RTC);
    const uint32_t rtc_date = LL_RTC_DATE_Get(RTC);

    pdatetime->hour = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_HOUR(rtc_time));
    pdatetime->minute = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MINUTE(rtc_time));
    pdatetime->second = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_SECOND(rtc_time));

    pdatetime->year = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_YEAR(rtc_date));
    pdatetime->month = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MONTH(rtc_date));
    pdatetime->day = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_DAY(rtc_date));
    pdatetime->weekday = __LL_RTC_GET_WEEKDAY(rtc_date);
}

void MX_RTC_Set(const date_time_t * pdatetime)
{
    if (!MX_RTC_IsDateTimeValid(pdatetime))
    {
        return;
    }

    LL_RTC_TimeTypeDef time = {0};
    LL_RTC_DateTypeDef date = {0};

    time.Hours = pdatetime->hour;
    time.Minutes = pdatetime->minute;
    time.Seconds = pdatetime->second;

    date.WeekDay = pdatetime->weekday;
    date.Month = pdatetime->month;
    date.Day = pdatetime->day;
    date.Year = pdatetime->year;

    if (LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BIN, &time) == SUCCESS &&
        LL_RTC_DATE_Init(RTC, LL_RTC_FORMAT_BIN, &date) == SUCCESS)
    {
        LL_RTC_BKP_SetRegister(TAMP, LL_RTC_BKP_DR0, RTC_BACKUP_MAGIC);
    }
}
