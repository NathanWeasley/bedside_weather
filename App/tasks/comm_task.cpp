#include "tasks/comm_task.h"

void CommTask::init()
{
    MX_USART1_UART_StartReceive();
    reset_packet_state();
}

void CommTask::tick()
{
    const uint32_t overrun_count = MX_USART1_UART_GetRxOverrunCount();
    if (overrun_count != _last_rx_overrun_count)
    {
        /* DMA 已覆盖未消费数据，丢弃当前残帧并从后续帧头重新同步。 */
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
            if (byte == 0xAAU)
            {
                *_next_ptr++ = byte;
                _read_state = ReadState::EXPECT_HEADER2;
            }
        } break;

        case ReadState::EXPECT_HEADER2:
        {
            if (byte == 0x55U)
            {
                *_next_ptr++ = byte;
                _read_state = ReadState::EXPECT_ADDR;
            }
            else
            {
                reset_packet_state();
                if (byte == 0xAAU)
                {
                    *_next_ptr++ = byte;
                    _read_state = ReadState::EXPECT_HEADER2;
                }
            }
        } break;

        case ReadState::EXPECT_ADDR:
        {
            *_next_ptr++ = byte;
            _read_state = ReadState::EXPECT_LENGTH1;
        } break;

        case ReadState::EXPECT_LENGTH1:
        {
            *_next_ptr++ = byte;
            _read_state = ReadState::EXPECT_LENGTH2;
        } break;

        case ReadState::EXPECT_LENGTH2:
        {
            *_next_ptr++ = byte;
            if (_rx_packet.payload_len > v1::Packet::__max_payload_len)
            {
                reset_packet_state();
            }
            else
            {
                _bytes_left = _rx_packet.payload_len + V1_CHECKSUM_LEN;
                _read_state = ReadState::EXPECT_DATA;
            }
        } break;

        case ReadState::EXPECT_DATA:
        {
            *_next_ptr++ = byte;
            if (--_bytes_left == 0U)
            {
                const uint8_t addr = _rx_packet.addr;
                if (_callbacks[addr] != nullptr)
                {
                    /* CommTask 只分发完整帧，载荷含义和校验由注册者处理。 */
                    _callbacks[addr](_rx_packet);
                }
                reset_packet_state();
            }
        } break;

        default:
            reset_packet_state();
            break;
    }
}

bool CommTask::register_callback(uint8_t recv_idx, CBFunc cb)
{
    if ((_callbacks[recv_idx] != nullptr) || (cb == nullptr))
    {
        return false;
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
    _bytes_left = 0U;
    _next_ptr = _rx_packet.ptr();
}
