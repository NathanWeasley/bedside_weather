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

    // Packet scan state
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

    bool register_callback(uint8_t recv_idx, void (*cb)(const v1::Packet&));
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
    // Check USART1 TX DMA unsent size
    uint16_t byte2send = LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_3);
    if (byte2send > 0)
    {
        if (wait_on_busy)
        {
            while (LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_3));
        }
        else
        {
            return false;
        }
    }
    
    // Pack new data
    _tx_packet.pack(data, rx_addr);

    // Disable DMA so new parameters can be written to register
    LL_USART_DisableDMAReq_TX(USART1);
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_3);

    // Set new parameters
    LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_3,
                           (uint32_t)_tx_packet.ptr(),
                           (uint32_t)&(USART1->TDR),
                           LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_3, _tx_packet.packet_size());

    // Enable DMA and TX request
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_3);
    LL_USART_EnableDMAReq_TX(USART1);

    return true;
}

