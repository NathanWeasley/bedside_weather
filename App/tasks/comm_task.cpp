#include "tasks/comm_task.h"
#include "usart.h"
#include "v1/v1.hpp"

void CommTask::init()
{
    MX_USART1_UART_StartReceive();
}

void CommTask::tick()
{
    uint8_t byte;
    uint16_t size;
    circular_buffer_t * pbuf = reinterpret_cast<circular_buffer_t *>(MX_USART1_UART_GetRecvBuffer());

    // Update head, like taking a snapshot of the USART1 RX buffer at this moment
    MX_USART1_UART_UpdateBufferHead();

    // Process captured data (scan for packets)
    size = circular_buffer_available(pbuf);
    for (uint16_t i = 0; i < size; ++i)
    {
        if (circular_buffer_read_byte(pbuf, &byte))
        {
            switch (_read_state)
            {
            case ReadState::EXPECT_HEADER1:
            {
                // Expect header 0xAA
                if (byte == 0xAA)
                {
                    *_next_ptr = byte;
                    ++_next_ptr;
                    _read_state = ReadState::EXPECT_HEADER2;
                }
            } break;
            case ReadState::EXPECT_HEADER2:
            {
                // Expect header 0x55
                if (byte == 0x55)
                {
                    *_next_ptr = byte;
                    ++_next_ptr;
                    _read_state = ReadState::EXPECT_ADDR;
                }
                else
                {
                    reset_packet_state();
                }
            } break;
            case ReadState::EXPECT_ADDR:
            {
                // Expect RX address
                *_next_ptr = byte;
                ++_next_ptr;
                _read_state = ReadState::EXPECT_LENGTH1;
            } break;
            case ReadState::EXPECT_LENGTH1:
            {
                // Expect 1st byte of length
                *_next_ptr = byte;
                ++_next_ptr;
                _read_state = ReadState::EXPECT_LENGTH2;
            } break;
            case ReadState::EXPECT_LENGTH2:
            {
                // Expect 2nd byte of length
                *_next_ptr = byte;
                ++_next_ptr;

                // Check for size exceeding
                if (_v1_packet.payload_len > v1::Packet::__max_payload_len)
                {
                    // Size exceeds maximum limit, must be something wrong
                    reset_packet_state();
                }
                else
                {
                    _bytes_left = _v1_packet.payload_len + 1; // +1 for reading checksum
                    _read_state = ReadState::EXPECT_DATA;
                }
            } break;
            case ReadState::EXPECT_DATA:
            {
                // Expect data (or checksum)
                if (_bytes_left > 0)
                {
                    // Continue push packet data
                    *_next_ptr = byte;
                    ++_next_ptr;
                    --_bytes_left;
                }
                else
                {
                    // A complete packet is received, partial decode and dispatch

                    uint8_t addr = _v1_packet.addr;
                    if (_callbacks[addr])
                    {
                        _callbacks[addr](_v1_packet);
                    }

                    // reset read state to default
                    reset_packet_state();
                }
            } break;

            default:
                // Unknown error, force to default
                reset_packet_state();
            }
        }
    }
}

bool CommTask::register_callback(uint8_t recv_idx, void (*cb)(const v1::Packet&))
{
    if (_callbacks[recv_idx])
    {
        return false;           // Slot already taken
    }

    _callbacks[recv_idx] = cb;
    return true;
}

void CommTask::release_callback(uint8_t recv_idx)
{
    _callbacks[recv_idx] = nullptr;
}

void CommTask::reset_packet_state()
{
    _read_state = ReadState::EXPECT_HEADER1;
    _bytes_left = 0;
    _next_ptr = _v1_packet.ptr();
}
