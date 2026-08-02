#pragma once

// 公共运行参数；Wi-Fi 与 API 密钥请填写在 secrets.h 中。
#define WEATHER_LOCATION "xian"
#define WEATHER_LANGUAGE "zh-Hans"
#define WEATHER_UNIT "c"
#define DEVICE_TIMEZONE "CST-8"

// 当前 API 套餐不包含逐小时天气权限。设为 1 可重新启用接口和 08:00--18:00 变化提取。
#define ENABLE_HOURLY_FORECAST 0

// 00:00（含）至 21:00（不含）返回当天，21:00 起返回次日。
#define NEXT_DAY_WEATHER_START_HOUR 21

#define WEATHER_REFRESH_INTERVAL_MS (30UL * 60UL * 1000UL)
#define WEATHER_RETRY_INTERVAL_MS (5UL * 60UL * 1000UL)
#define DATETIME_PUSH_INTERVAL_MS (60UL * 60UL * 1000UL)
#define WIFI_POLL_INTERVAL_MS 1000UL
#define HTTP_TIMEOUT_MS 8000U
#define API_MIN_REQUEST_INTERVAL_MS 1100U

// 设为 1 时，调试信息从 Serial1（GPIO2，仅 TX）输出，不污染 MCU 协议串口。
#define ENABLE_DEBUG_LOG 0
