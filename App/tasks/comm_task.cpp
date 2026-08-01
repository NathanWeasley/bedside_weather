#include "tasks/comm_task.h"
#include "v1/v1.hpp"

void CommTask::init()
{
    MX_USART1_UART_StartReceive();

    MX_USART1_UART_DMASend((const uint8_t *)"V1 comm task is on!\n", 20);

    reset_packet_state();
}

void CommTask::tick()
{
    const uint32_t overrun_count = MX_USART1_UART_GetRxOverrunCount();
    if (overrun_count != _last_rx_overrun_count)
    {
        // DMA 已覆盖未消费数据，丢弃当前残帧并从后续帧头重新同步。
        _last_rx_overrun_count = overrun_count;
        reset_packet_state();
    }

    uint8_t received[64];
    uint16_t received_size;
    do
    {
        received_size = MX_USART1_UART_GetReceived(received, sizeof(received));
        const uint32_t updated_overrun_count = MX_USART1_UART_GetRxOverrunCount();
        if (updated_overrun_count != _last_rx_overrun_count)
        {
            _last_rx_overrun_count = updated_overrun_count;
            reset_packet_state();
        }
        for (uint16_t i = 0; i < received_size; ++i)
        {
            process_received_byte(received[i]);
        }
    }
    while (received_size == sizeof(received));
}

void CommTask::process_received_byte(uint8_t byte)
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
                if (byte == 0xAA)
                {
                    *_next_ptr = byte;
                    ++_next_ptr;
                    _read_state = ReadState::EXPECT_HEADER2;
                }
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
            if (_rx_packet.payload_len > v1::Packet::__max_payload_len)
            {
                // Size exceeds maximum limit, must be something wrong
                reset_packet_state();
            }
            else
            {
                _bytes_left = _rx_packet.payload_len + 1; // +1 for reading checksum
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

                if (_bytes_left == 0)
                {
                    // A complete packet is received, partial decode and dispatch
                    uint8_t addr = _rx_packet.addr;
                    if (_callbacks[addr])
                    {
                        _callbacks[addr](_rx_packet);   // Unpacking are done in callbacks
                    }

                    // reset read state to default
                    reset_packet_state();
                }
            }
        } break;

        default:
            // Unknown error, force to default
            reset_packet_state();
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
    _next_ptr = _rx_packet.ptr();
}
