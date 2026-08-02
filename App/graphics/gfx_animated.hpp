#pragma once

#include "config.h"

#include <cstdint>
#include <cstring>
#include <type_traits>

namespace gfx
{

/*
 * Tag 用于区分尺寸相同但存储独立的画布。画布仍为静态分配，避免裸机环境动态内存。
 */
template <uint16_t CW, uint16_t CH, typename Tag = void>
struct Canvas
{
    static_assert((CW > 0U) && (CH > 0U), "Canvas dimensions must be non-zero");

    static constexpr uint16_t __cwidth = CW;
    static constexpr uint16_t __cheight = CH;
    static constexpr uint32_t __csize = static_cast<uint32_t>(CW) * CH;

    static uint8_t _data[__csize];

    static uint8_t * data() { return _data; }
    static constexpr uint32_t size() { return __csize; }
    static constexpr uint16_t width() { return __cwidth; }
    static constexpr uint16_t height() { return __cheight; }
};

template <uint16_t CW, uint16_t CH, typename Tag>
uint8_t Canvas<CW, CH, Tag>::_data[Canvas<CW, CH, Tag>::__csize] = {0};

template <typename T>
struct is_Canvas : std::false_type {};

template <uint16_t CW, uint16_t CH, typename Tag>
struct is_Canvas<Canvas<CW, CH, Tag>> : std::true_type {};

template <typename T>
static constexpr bool is_Canvas_v = is_Canvas<T>::value;


template <
    typename CV,
    uint16_t WX, uint16_t WY, uint16_t WW, uint16_t WH,
    typename = std::enable_if_t<is_Canvas_v<CV>>
>
struct Window
{
    using CanvasType = CV;

    static_assert((WW > 0U) && (WH > 0U), "Window dimensions must be non-zero");
    static_assert((WX + WW) <= CV::width(), "Window exceeds canvas width");
    static_assert((WY + WH) <= CV::height(), "Window exceeds canvas height");

    static uint8_t * data()
    {
        return CV::data() + static_cast<uint32_t>(WY) * CV::width() + WX;
    }

    static auto canvas() { return CV{}; }
    static constexpr uint16_t x() { return WX; }
    static constexpr uint16_t y() { return WY; }
    static constexpr uint16_t width() { return WW; }
    static constexpr uint16_t height() { return WH; }
};

template <typename T>
struct is_Window : std::false_type {};

template <typename CV, uint16_t WX, uint16_t WY, uint16_t WW, uint16_t WH, typename Enabled>
struct is_Window<Window<CV, WX, WY, WW, WH, Enabled>> : std::true_type {};

template <typename T>
static constexpr bool is_Window_v = is_Window<T>::value;


/* 动画策略只修改状态；Viewport 负责依据状态采样和应用灰度效果。 */
struct AnimationState
{
    uint16_t offset_x;
    uint16_t offset_y;
    uint16_t cycle_width;
    uint16_t cycle_height;
    uint8_t opacity;
    bool visible;
    bool inverted;
};

class StaticAnimator
{
public:
    void reset() {}
    bool tick(AnimationState&) { return false; }
};

enum class ScrollDirection : uint8_t
{
    LEFT = 0,
    RIGHT,
    UP,
    DOWN,
};

class ScrollAnimator
{
    ScrollDirection _direction;
    uint16_t _period_ticks;
    uint16_t _delay_ticks;
    uint16_t _ticks_left;
    uint16_t _delay_left;
    bool _enabled;

public:
    ScrollAnimator()
    : _direction(ScrollDirection::LEFT)
    , _period_ticks(1U)
    , _delay_ticks(0U)
    , _ticks_left(1U)
    , _delay_left(0U)
    , _enabled(false)
    {}

    void configure(ScrollDirection direction,
                   uint16_t period_ticks,
                   uint16_t delay_ticks = 0U,
                   bool enabled = true)
    {
        _direction = direction;
        _period_ticks = (period_ticks == 0U) ? 1U : period_ticks;
        _delay_ticks = delay_ticks;
        _enabled = enabled;
        reset();
    }

    void reset()
    {
        _ticks_left = _period_ticks;
        _delay_left = _delay_ticks;
    }

    bool tick(AnimationState& state)
    {
        if (!_enabled)
        {
            return false;
        }
        if (_delay_left > 0U)
        {
            --_delay_left;
            return false;
        }
        if (--_ticks_left > 0U)
        {
            return false;
        }
        _ticks_left = _period_ticks;

        switch (_direction)
        {
            case ScrollDirection::LEFT:
                state.offset_x = (state.offset_x + 1U >= state.cycle_width) ?
                                 0U : static_cast<uint16_t>(state.offset_x + 1U);
                break;
            case ScrollDirection::RIGHT:
                state.offset_x = (state.offset_x == 0U) ?
                                 static_cast<uint16_t>(state.cycle_width - 1U) :
                                 static_cast<uint16_t>(state.offset_x - 1U);
                break;
            case ScrollDirection::UP:
                state.offset_y = (state.offset_y + 1U >= state.cycle_height) ?
                                 0U : static_cast<uint16_t>(state.offset_y + 1U);
                break;
            case ScrollDirection::DOWN:
                state.offset_y = (state.offset_y == 0U) ?
                                 static_cast<uint16_t>(state.cycle_height - 1U) :
                                 static_cast<uint16_t>(state.offset_y - 1U);
                break;
        }
        return true;
    }
};

class BlinkAnimator
{
    uint16_t _period_ticks;
    uint16_t _ticks_left;
    bool _enabled;

public:
    BlinkAnimator()
    : _period_ticks(1U)
    , _ticks_left(1U)
    , _enabled(false)
    {}

    void configure(uint16_t period_ticks, bool enabled = true)
    {
        _period_ticks = (period_ticks == 0U) ? 1U : period_ticks;
        _enabled = enabled;
        reset();
    }

    void reset() { _ticks_left = _period_ticks; }

    bool tick(AnimationState& state)
    {
        if (!_enabled || (--_ticks_left > 0U))
        {
            return false;
        }
        _ticks_left = _period_ticks;
        state.visible = !state.visible;
        return true;
    }
};

class InvertAnimator
{
    uint16_t _period_ticks;
    uint16_t _ticks_left;
    bool _enabled;

public:
    InvertAnimator()
    : _period_ticks(1U)
    , _ticks_left(1U)
    , _enabled(false)
    {}

    void configure(uint16_t period_ticks, bool enabled = true)
    {
        _period_ticks = (period_ticks == 0U) ? 1U : period_ticks;
        _enabled = enabled;
        reset();
    }

    void reset() { _ticks_left = _period_ticks; }

    bool tick(AnimationState& state)
    {
        if (!_enabled || (--_ticks_left > 0U))
        {
            return false;
        }
        _ticks_left = _period_ticks;
        state.inverted = !state.inverted;
        return true;
    }
};

class FadeAnimator
{
    uint16_t _period_ticks;
    uint16_t _ticks_left;
    uint8_t _step;
    int8_t _direction;
    bool _ping_pong;
    bool _enabled;

public:
    FadeAnimator()
    : _period_ticks(1U)
    , _ticks_left(1U)
    , _step(1U)
    , _direction(-1)
    , _ping_pong(true)
    , _enabled(false)
    {}

    void configure(uint16_t period_ticks,
                   uint8_t step,
                   bool ping_pong = true,
                   bool enabled = true)
    {
        _period_ticks = (period_ticks == 0U) ? 1U : period_ticks;
        _step = (step == 0U) ? 1U : step;
        _ping_pong = ping_pong;
        _enabled = enabled;
        _direction = -1;
        reset();
    }

    void reset()
    {
        _ticks_left = _period_ticks;
        _direction = -1;
    }

    bool tick(AnimationState& state)
    {
        if (!_enabled || (--_ticks_left > 0U))
        {
            return false;
        }
        _ticks_left = _period_ticks;

        if (_direction < 0)
        {
            if (state.opacity <= _step)
            {
                state.opacity = 0U;
                if (_ping_pong)
                {
                    _direction = 1;
                }
                else
                {
                    _enabled = false;
                }
            }
            else
            {
                state.opacity = static_cast<uint8_t>(state.opacity - _step);
            }
        }
        else
        {
            const uint16_t next = static_cast<uint16_t>(state.opacity) + _step;
            if (next >= 255U)
            {
                state.opacity = 255U;
                _direction = -1;
            }
            else
            {
                state.opacity = static_cast<uint8_t>(next);
            }
        }
        return true;
    }
};

/* 可嵌套组合，例如 ParallelAnimator<ScrollAnimator, BlinkAnimator>。 */
template <typename FirstAnimator, typename SecondAnimator>
class ParallelAnimator
{
    FirstAnimator _first;
    SecondAnimator _second;

public:
    FirstAnimator& first() { return _first; }
    SecondAnimator& second() { return _second; }

    void reset()
    {
        _first.reset();
        _second.reset();
    }

    bool tick(AnimationState& state)
    {
        const bool first_changed = _first.tick(state);
        const bool second_changed = _second.tick(state);
        return first_changed || second_changed;
    }
};


template <
    uint16_t VW, uint16_t VH,
    typename WD,
    typename = std::enable_if_t<is_Window_v<WD>>
>
class Viewport
{
    static_assert((VW > 0U) && (VH > 0U), "Viewport dimensions must be non-zero");
    static_assert(VW <= WD::width(), "Viewport exceeds window width");
    static_assert(VH <= WD::height(), "Viewport exceeds window height");

    AnimationState _state;

    static void apply_effect(uint8_t * data, uint16_t size, const AnimationState& state)
    {
        if (!state.visible)
        {
            memset(data, 0, size);
            return;
        }
        if ((state.opacity == 255U) && !state.inverted)
        {
            return;
        }

        for (uint16_t i = 0U; i < size; ++i)
        {
            uint8_t value = state.inverted ? static_cast<uint8_t>(255U - data[i]) : data[i];
            value = static_cast<uint8_t>((static_cast<uint16_t>(value) * state.opacity + 127U) / 255U);
            data[i] = value;
        }
    }

public:
    using WindowType = WD;

    Viewport()
    : _state{0U, 0U, WD::width(), WD::height(), 255U, true, false}
    {}

    AnimationState& state() { return _state; }
    const AnimationState& state() const { return _state; }

    void set_cycle_size(uint16_t cycle_width, uint16_t cycle_height)
    {
        if (cycle_width < VW)
        {
            cycle_width = VW;
        }
        else if (cycle_width > WD::width())
        {
            cycle_width = WD::width();
        }

        if (cycle_height < VH)
        {
            cycle_height = VH;
        }
        else if (cycle_height > WD::height())
        {
            cycle_height = WD::height();
        }

        _state.cycle_width = cycle_width;
        _state.cycle_height = cycle_height;
        if (_state.offset_x >= cycle_width)
        {
            _state.offset_x = 0U;
        }
        if (_state.offset_y >= cycle_height)
        {
            _state.offset_y = 0U;
        }
    }

    void reset()
    {
        _state.offset_x = 0U;
        _state.offset_y = 0U;
        _state.opacity = 255U;
        _state.visible = true;
        _state.inverted = false;
    }

    void get_line(uint16_t line, uint8_t * output) const
    {
        if ((line >= VH) || (output == nullptr))
        {
            return;
        }

        const uint16_t source_y = static_cast<uint16_t>(
            (_state.offset_y + line) % _state.cycle_height);
        const uint8_t * const row = WD::data() +
            static_cast<uint32_t>(source_y) * WD::canvas().width();

        const uint16_t first_size = static_cast<uint16_t>(
            _state.cycle_width - _state.offset_x);
        if (first_size >= VW)
        {
            memcpy(output, row + _state.offset_x, VW);
        }
        else
        {
            memcpy(output, row + _state.offset_x, first_size);
            memcpy(output + first_size, row, VW - first_size);
        }
        apply_effect(output, VW, _state);
    }

    static constexpr uint16_t x() { return WD::x(); }
    static constexpr uint16_t y() { return WD::y(); }
    static constexpr uint16_t width() { return VW; }
    static constexpr uint16_t height() { return VH; }
};

template <typename T>
struct is_Viewport : std::false_type {};

template <uint16_t VW, uint16_t VH, typename WD, typename Enabled>
struct is_Viewport<Viewport<VW, VH, WD, Enabled>> : std::true_type {};

template <typename T>
static constexpr bool is_Viewport_v = is_Viewport<T>::value;


template <typename VP, typename Animator, typename = std::enable_if_t<is_Viewport_v<VP>>>
class AnimatedView
{
    VP _viewport;
    Animator _animator;

public:
    VP& viewport() { return _viewport; }
    const VP& viewport() const { return _viewport; }
    Animator& animator() { return _animator; }

    void reset()
    {
        _viewport.reset();
        _animator.reset();
    }

    bool tick()
    {
        return _animator.tick(_viewport.state());
    }

    void get_line(uint16_t line, uint8_t * output) const
    {
        _viewport.get_line(line, output);
    }

    static constexpr uint16_t x() { return VP::x(); }
    static constexpr uint16_t y() { return VP::y(); }
    static constexpr uint16_t width() { return VP::width(); }
    static constexpr uint16_t height() { return VP::height(); }
};


template <uint16_t DW, uint16_t DH, uint8_t * PTR>
struct Display
{
    static_assert((DW > 0U) && (DH > 0U), "Display dimensions must be non-zero");

    static constexpr uint16_t width() { return DW; }
    static constexpr uint16_t height() { return DH; }
    static constexpr uint32_t size() { return static_cast<uint32_t>(DW) * DH; }
    static constexpr uint8_t * data() { return PTR; }
};

template <
    typename View, typename DisplayType,
    uint16_t DX = View::x(), uint16_t DY = View::y()
>
class DisplayZone
{
    static_assert((DX + View::width()) <= DisplayType::width(),
                  "Display zone exceeds display width");
    static_assert((DY + View::height()) <= DisplayType::height(),
                  "Display zone exceeds display height");

public:
    void update(const View& view)
    {
        uint8_t * output = DisplayType::data() + DX +
                           static_cast<uint32_t>(DY) * DisplayType::width();
        for (uint16_t line = 0U; line < View::height(); ++line)
        {
            view.get_line(line, output);
            output += DisplayType::width();
        }
    }

    bool tick_then_update(View& view)
    {
        if (!view.tick())
        {
            return false;
        }
        update(view);
        return true;
    }
};

inline bool tick_all()
{
    return false;
}

template <typename Zone, typename View, typename... Remaining>
bool tick_all(Zone& zone, View& view, Remaining&... remaining)
{
    static_assert((sizeof...(remaining) % 2U) == 0U,
                  "tick_all expects zone/view pairs");
    const bool changed = zone.tick_then_update(view);
    return tick_all(remaining...) || changed;
}

} // namespace gfx
