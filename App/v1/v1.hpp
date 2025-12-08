#pragma once

#include <cstdint>
#include <cstring>
#include "utils/cbuffer.h"

#ifndef V1_MAX_PACKET_LEN
#define V1_MAX_PACKET_LEN   (1024)
#endif

#define V1_FSTART_LEN       (2)         // 0xAA 0x55
#define V1_SIZE_LEN         (2)         // 2 bytes for packet length (0-1023)
#define V1_MAX_ADDR_LEN     (1)         // 1 byte for rx address (0-255)
// #define V1_FSTOP_LEN        (1)         // 0x5A (not used)
#define V1_CHECKSUM_LEN     (1)         // Frame checksum
#define V1_HEADER_LEN       (V1_FSTART_LEN + V1_SIZE_LEN + V1_MAX_ADDR_LEN)

// ----------------------------
// |  2 bytes  | 1 byte |  2 bytes  | N bytes | 1 byte |
// | 0xAA 0x55 |  ADDR  | LENH LENL | PAYLOAD | CHKSUM |
//                            ^ ---  <------->              LEN only represent payload size
//              <----------------------------> --- ^

namespace v1
{

#pragma pack(1)
struct Packet
{
    static constexpr size_t __data_buffer_len = V1_MAX_PACKET_LEN - V1_HEADER_LEN;
    static constexpr size_t __max_payload_len = __data_buffer_len - V1_CHECKSUM_LEN;

    uint16_t header;
    uint8_t addr;
    uint16_t payload_len;
    uint8_t data[__data_buffer_len];    // checksum is implicitly included in data field

    Packet()
    : header(0x55AA)
    , addr(0)
    , payload_len(0)
    , data{0}
    {}

    inline uint16_t packet_size() const
    {
        return payload_len + V1_HEADER_LEN + V1_CHECKSUM_LEN;
    }

    inline uint8_t * ptr()
    {
        return reinterpret_cast<uint8_t *>(&header);
    }

    inline uint16_t raw_in(const uint8_t * raw, uint16_t len)
    {
        uint16_t real_len = len > V1_MAX_PACKET_LEN ? V1_MAX_PACKET_LEN : len;
        memcpy(ptr(), raw, real_len);

        return real_len;
    }

    Packet& operator=(const Packet& other)
    {
        header = other.header;
        addr = other.addr;
        payload_len = other.payload_len;

        // Only copy necessary data to save time (payload + checksum)
        memcpy(data, other.data, payload_len + 1);

        return *this;
    }

    /**
     * Pack user-defined data into packet
     */
    template <typename T>
    void pack(const T& payload, uint8_t rx_addr)
    {
        static_assert(sizeof(T) <= __max_payload_len, "<V1> Payload exceeds packet limit!");
        static_assert(alignof(T) == 1, "<V1> Payload type must be 1-byte aligned to avoid padding!");

        uint16_t datalen = sizeof(T);
        const uint8_t * ptr = reinterpret_cast<const uint8_t *>(&payload);

        addr = rx_addr;
        payload_len = datalen;
        memcpy(data, ptr, datalen);

        // Calculate checksum
        uint8_t sum = addr;
        sum += static_cast<uint8_t>(payload_len >> 8);
        sum += static_cast<uint8_t>(payload_len & 0x00FF);
        for (uint16_t i = 0; i < payload_len; ++i)
        {
            sum += *ptr;
            ++ptr;
        }

        // Checksum
        *ptr = sum;
    }

    /**
     * Unpack the content of packet into user-defined type.
     * Make sure to copy all data to address pointed by ptr()
     *   before calling unpack<...>(...).
     */
    template <typename T>
    bool unpack(T& payload, uint8_t& rx_addr)
    {
        static_assert(sizeof(T) <= __max_payload_len, "<V1> Payload exceeds packet limit!");
        static_assert(alignof(T) == 1, "<V1> Payload type must be 1-byte aligned to avoid padding!");

        if ((sizeof(T) != payload_len))
        {
            return false;
        }

        uint8_t checksum = addr;
        checksum += static_cast<uint8_t>(payload_len >> 8);
        checksum += static_cast<uint8_t>(payload_len & 0x00FF);

        uint8_t * ptr = data;
        for (uint16_t i = 0; i < payload_len; ++i)
        {
            checksum += *ptr;
            ++ptr;
        }

        if (checksum != *ptr)
        {
            return false;
        }

        memcpy(&payload, data, sizeof(T));
        rx_addr = addr;

        return true;
    }

    template <typename T>
    bool unpack(T& payload)
    {
        uint8_t dummy;
        return unpack(payload, dummy);
    }
};
#pragma pack()

}   // V1
