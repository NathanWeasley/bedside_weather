#include <cstdint>

template <uint16_t Size>
struct CircularBuffer
{
    uint8_t buffer[Size];
    uint16_t head;
    uint16_t tail;

    CircularBuffer()
    : buffer{0}
    , head(0)
    , tail(0)
    {}

    inline bool isEmpty() const
    {
        return head == tail;
    }

    inline uint16_t available() const
    {
        if (head >= tail)
        {
            return head - tail;
        }
        else
        {
            return Size - tail + head;
        }
    }

    bool readByte(uint8_t& byte)
    {
        if (isEmpty())
        {
            return false;
        }

        byte = buffer[tail];
        tail = (tail + 1) % Size;

        return true;
    }

    void writeByte(uint8_t byte)
    {
        buffer[head] = byte;
        head = (head + 1) % Size;

        if (head == tail)
        {
            tail = (tail + 1) % Size;
        }
    }
};

