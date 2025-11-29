#pragma once

#include "tasks/task.h"
#include "utils/cbuffer.h"
#include "v1/v1.hpp"

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

    v1::Packet          _v1_packet;


    CommTask()
    : Base(COMM_TASK_PRESC)
    , _callbacks{nullptr}
    , _read_state(ReadState::EXPECT_HEADER1)
    , _bytes_left(0)
    , _next_ptr(nullptr)
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

private:
    void reset_packet_state();
};

