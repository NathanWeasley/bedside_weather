#pragma once

#include <cstdint>

class TaskBase
{
public:
    TaskBase(uint8_t presc)
    : _presc(presc)
    , _entry_cnt(0)
    {}
    virtual ~TaskBase() = default;

    virtual void init() = 0;
    virtual void tick() = 0;

    inline bool test_tick()
    {
        if (++_entry_cnt == _presc)
        {
            _entry_cnt = 0;
            return true;
        }
        else
        {
            return false;
        }
    }

public:
    uint8_t _presc;
    uint8_t _entry_cnt;
};
