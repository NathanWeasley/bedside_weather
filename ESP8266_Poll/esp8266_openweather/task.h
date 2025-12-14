#pragma once

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#include "task_types.h"
#include "v1.hpp"
#include "cbuffer.hpp"

class Task
{
    // Arduino framework
    ESP8266WiFiMulti    _wifi_multi;
    String              _weather_url;
    String              _time_url;

    // Communication with MCU
    enum class ReadState
        : uint8_t
    {
        EXPECT_HEADER1 = 0,
        EXPECT_HEADER2,
        EXPECT_ADDR,
        EXPECT_LENGTH1,
        EXPECT_LENGTH2,
        EXPECT_DATA
    };
    ReadState           _read_state;
    uint16_t            _bytes_left;
    uint8_t *           _next_ptr;
    v1::Packet          _rx_packet;
    v1::Packet          _tx_packet;
    CircularBuffer<64>  _cbuf;
    DynamicJsonDocument _json;
    McuReq              _req;
    NetworkStatusResp   _status_resp;
    TimeDateResp        _timedate_resp;
    WeatherDataResp     _weather_resp_today;
    WeatherDataResp     _weather_resp_tomorrow;

public:
    Task()
    : _next_ptr(nullptr)
    , _json(2000)
    {
        _req.type = McuRequest::NO_REQUEST;
    }

    void init();
    void tick();

private:
    void fetch_time();
    void fetch_weather();

    void reset_read_state();
    void decrypt_packet();
    void decrypt_weather(String& payload);
    void decrypt_time(String& payload);

    void resp_ready();
    void resp_datetime();
    void resp_weather();
};
