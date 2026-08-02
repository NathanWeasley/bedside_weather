#pragma once

#include "tasks/task.h"
#include "v1/v1.hpp"

#include "usart.h"

#define COMM_TASK_PRESC             (1)
#define COMM_TASK_MAX_RX_CALLBACK   (256)

class CommTask
    : public TaskBase
{
    using Base = TaskBase;
    using CBFunc = void (*)(const v1::Packet&);

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

    CBFunc              _callbacks[COMM_TASK_MAX_RX_CALLBACK];

    ReadState           _read_state;
    uint16_t            _bytes_left;
    uint8_t *           _next_ptr;
    uint32_t            _last_rx_overrun_count;

    v1::Packet          _rx_packet;
    v1::Packet          _tx_packet;

    CommTask()
    : Base(COMM_TASK_PRESC)
    , _callbacks{nullptr}
    , _read_state(ReadState::EXPECT_HEADER1)
    , _bytes_left(0)
    , _next_ptr(nullptr)
    , _last_rx_overrun_count(0)
    {}
    ~CommTask() = default;

public:
    static inline CommTask * instance()
    {
        static CommTask tsk;
        return &tsk;
    }

    void init() override;
    void tick() override;

    bool register_callback(uint8_t recv_idx, CBFunc cb);
    void release_callback(uint8_t recv_idx);

    template <typename T>
    bool send_pack(uint8_t rx_addr, const T& data, bool wait_on_busy = false);

private:
    void reset_packet_state();
    void process_received_byte(uint8_t byte);
};

template <typename T>
bool CommTask::send_pack(uint8_t rx_addr, const T& data, bool wait_on_busy)
{
    /* 普通文本与协议包共用底层 TX 仲裁，不能中止正在进行的 DMA 发送。 */
    if (wait_on_busy)
    {
        while (!MX_USART1_UART_CheckTXAvailability())
        {
        }
    }
    else if (!MX_USART1_UART_CheckTXAvailability())
    {
        return false;
    }

    _tx_packet.pack(data, rx_addr);
    return MX_USART1_UART_TryDMASend(_tx_packet.ptr(), _tx_packet.packet_size()) != 0U;
}
