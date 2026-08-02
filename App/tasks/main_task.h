#pragma once

#include "tasks/task.h"
#include "v1/v1.hpp"
#include "v1/weather_protocol.h"

#define MAIN_TASK_PRESC             (1)

class MainTask
    : public TaskBase
{
    using Base = TaskBase;

    enum ResponseValidMask : uint8_t
    {
        RESPONSE_NETWORK_VALID = 1U << 0,
        RESPONSE_DATETIME_VALID = 1U << 1,
        RESPONSE_WEATHER_VALID = 1U << 2,
    };

    bedside::NetworkStatusResponse _network_status;
    bedside::DateTimeResponse      _datetime;
    bedside::WeatherResponse       _weather;
    uint8_t             _response_valid_mask;
    uint32_t            _response_error_count;

    uint32_t            _network_request_ticks;
    uint32_t            _datetime_request_ticks;
    uint32_t            _weather_request_ticks;
    uint32_t            _request_gap_ticks;

    uint8_t             _last_display_second;
    bool                _display_dirty;

    MainTask()
    : Base(MAIN_TASK_PRESC)
    , _network_status{bedside::NetworkCondition::DISCONNECTED}
    , _datetime{}
    , _weather{}
    , _response_valid_mask(0)
    , _response_error_count(0)
    , _network_request_ticks(0)
    , _datetime_request_ticks(0)
    , _weather_request_ticks(0)
    , _request_gap_ticks(0)
    , _last_display_second(0xFFU)
    , _display_dirty(true)
    {}
    ~MainTask() = default;

public:
    static inline MainTask * instance()
    {
        static MainTask tsk;
        return &tsk;
    }

    void init() override;
    void tick() override;

    uint32_t response_error_count() const { return _response_error_count; }

private:
    bool send_request(bedside::McuRequest type);
    void service_requests();
    void prepare_display_info();

    static void network_status_callback(const v1::Packet& packet);
    static void datetime_callback(const v1::Packet& packet);
    static void weather_callback(const v1::Packet& packet);

    void handle_network_status(const v1::Packet& packet);
    void handle_datetime(const v1::Packet& packet);
    void handle_weather(const v1::Packet& packet);
};
