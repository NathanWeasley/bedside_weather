#pragma once

#include "config.h"
#include <cstdint>
#include <cstring>
#include <utility>
#include <type_traits>

namespace gfx
{

enum class AnimateMethod
    : uint8_t
{
    METHOD_NOANIMATION = 0,
    METHOD_LSHIFT,
    METHOD_RSHIFT,
    METHOD_USHIFT,
    METHOD_DSHIFT,
    METHOD_BLINK,
    METHOD_INVERSE,
    METHOD_FADE
};

template <uint16_t CW, uint16_t CH>
struct Canvas
{
    static constexpr uint16_t __cwidth = CW;
    static constexpr uint16_t __cheight = CH;
    static constexpr uint32_t __csize = CW*CH;

    static uint8_t _data[__csize];

    static uint8_t * data() { return _data; }
    static constexpr uint32_t size() { return __csize; }
    static constexpr uint16_t width() { return __cwidth; }
    static constexpr uint16_t height() { return __cheight; }
};

template <uint16_t CW, uint16_t CH>
uint8_t Canvas<CW, CH>::_data[Canvas<CW, CH>::__csize] = {0};

template <typename T>
struct is_Canvas : std::false_type {};

template <uint16_t CW, uint16_t CH>
struct is_Canvas<Canvas<CW, CH>> : std::true_type {};

template <typename T>
static constexpr bool is_Canvas_v = is_Canvas<T>::value;




template <
    uint16_t CW, uint16_t CH,
    uint16_t WX, uint16_t WY, uint16_t WW, uint16_t WH
>
struct Window
{
    using CanvasType = Canvas<CW, CH>;

    static constexpr uint16_t __wx0 = WX;
    static constexpr uint16_t __wy0 = WY;
    static constexpr uint16_t __wwidth = WW;
    static constexpr uint16_t __wheight = WH;

    static uint8_t * _ptr0;

    static auto canvas() { return Canvas<CW, CH>{}; }
    static uint8_t * data() { return _ptr0; }
    // static constexpr uint32_t size() { return __csize; }
    static constexpr uint16_t x() { return __wx0; }
    static constexpr uint16_t y() { return __wy0; }
    static constexpr uint16_t width() { return __wwidth; }
    static constexpr uint16_t height() { return __wheight; }
};

template <uint16_t CW, uint16_t CH, uint16_t WX, uint16_t WY, uint16_t WW, uint16_t WH>
uint8_t * Window<CW, CH, WX, WY, WW, WH>::_ptr0 = Canvas<CW, CH>::_data + WY * CW + WX;

template <typename T>
struct is_Window : std::false_type {};

template <
    uint16_t CW, uint16_t CH,
    uint16_t WX, uint16_t WY, uint16_t WW, uint16_t WH
>
struct is_Window<Window<CW, CH, WX, WY, WW, WH>> : std::true_type {};

template <typename T>
static constexpr bool is_Window_v = is_Window<T>::value;






template <AnimateMethod Method, typename DerivedMask>
struct Animate;

template <typename DerivedMask>
struct Animate<AnimateMethod::METHOD_NOANIMATION, DerivedMask>
{
private:
    auto derived() { return static_cast<DerivedMask *>(this); }
    const auto derived() const { return static_cast<const DerivedMask *>(this); }

public:
    inline void move()
    {
        // Trivial function - NOANIMATION simply means no change of pointers
    }
};

template <typename DerivedMask>
struct Animate<AnimateMethod::METHOD_LSHIFT, DerivedMask>
{
private:
    auto derived() { return static_cast<DerivedMask *>(this); }
    const auto derived() const { return static_cast<const DerivedMask *>(this); }

public:
    inline void move()
    {
        if (++derived()->_dx == derived()->window().width())
        {
            derived()->_dx = 0;
        }

        derived()->set_ptr();
    }
};

template <typename DerivedMask>
struct Animate<AnimateMethod::METHOD_USHIFT, DerivedMask>
{
private:
    auto derived() { return static_cast<DerivedMask *>(this); }
    const auto derived() const { return static_cast<const DerivedMask *>(this); }

public:
    inline void move()
    {
        if (++derived()->_dy == derived()->window().height())
        {
            derived()->_dy = 0;
        }

        derived()->set_ptr();
    }
};




template <
    uint16_t MW, uint16_t MH, AnimateMethod Method, uint16_t TPM,
    typename WD,
    uint16_t DELAY = 0,
    typename = std::enable_if<is_Window_v<WD>>
>
struct Mask
    : public Animate<Method, Mask<MW, MH, Method, TPM, WD, DELAY>>
{
    using Base = Animate<Method, Mask<MW, MH, Method, TPM, WD, DELAY>>;
    using WindowType = WD;

    static constexpr uint16_t __mwidth = MW;
    static constexpr uint16_t __mheight = MH;
    static constexpr AnimateMethod __mmethod = Method;
    static constexpr uint16_t __tpm = TPM;
    static constexpr int16_t __delay = DELAY;

    uint8_t * _ptr_op;
    uint16_t _dx;
    uint16_t _dy;

    uint16_t _cnt;

    Mask(uint16_t dx, uint16_t dy)
    : _dx(dx)
    , _dy(dy)
    , _cnt(1 + __delay)
    {
        set_ptr();
    }

    Mask()
    : Mask(0, 0)
    {}

    inline bool tick()
    {
        if (--_cnt > 0)
        {
            return false;
        }

        Base::move();
        _cnt = __tpm;

        return true;
    }

    inline void set_ptr()
    {
        _ptr_op = WD::_ptr0 + WD::canvas().width() * _dy + _dx;
    }


    inline void get_line(uint16_t i, uint8_t * pmem)
    {
        // if (i >= WH || !pmem)
        // {
        //     return;
        // }

        const uint8_t * ptr = _ptr_op + i * WD::canvas().width();

        if (_dx + __mwidth < WD::width())
        {
            memcpy(pmem, ptr, __mwidth);
        }
        else
        {
            uint16_t sz = WD::width() - _dx;
            memcpy(pmem, ptr, sz);          ///< copy last part
            ptr -= _dx;
            memcpy(pmem + sz, ptr, __mwidth - sz);    ///< copy first part (wrapped around)
        }
    }

    inline void get_masked(uint8_t * pmem)
    {
        // if (!pmem)
        // {
        //     return;
        // }

        for (uint16_t i = 0; i < MH; ++i)
        {
            get_line(i, pmem);
            pmem += MW;
        }
    }

    static constexpr inline uint16_t x() { return WD::x(); }
    static constexpr inline uint16_t y() { return WD::y(); }
    static constexpr inline uint16_t width() { return __mwidth; }
    static constexpr inline uint16_t height() { return __mheight; }
    static inline auto window() { return WD{}; }
};

template <typename T>
struct is_Mask : std::false_type {};

template <
    uint16_t MW, uint16_t MH, AnimateMethod Method, uint16_t TPM,
    typename WD,
    uint16_t DELAY,
    typename Enabled
>
struct is_Mask<Mask<MW, MH, Method, TPM, WD, DELAY, Enabled>> : std::true_type {};

template <typename T>
static constexpr bool is_Mask_v = is_Mask<T>::value;




template <uint16_t DW, uint16_t DH, uint8_t * PTR>
struct Display
{
    static inline constexpr uint16_t width() { return DW; }
    static inline constexpr uint16_t height() { return DH; }
    static inline constexpr uint32_t size() { return width() * height(); }

    static constexpr inline uint8_t * data() { return PTR; }
};

template <typename M, typename Display, typename = std::enable_if<is_Mask_v<M>>>
struct DisplayZone
{
    static constexpr uint8_t * __ptr0 = Display::data() + M::x() + M::y() * Display::width();

    static constexpr inline uint8_t * ptr() { return __ptr0; }
    static constexpr inline uint16_t width() { return M::width(); }
    static constexpr inline uint16_t height() { return M::height(); }

    template <typename Mask>
    inline void update(Mask&& m)
    {
        static_assert(width() == m.width() && height() == m.height(),
            "Mask and Zone must not differ in size!");

        uint8_t * ptr = __ptr0;
        for (uint16_t i = 0; i < height(); ++i)
        {
            m.get_line(i, ptr);
            ptr += Display::width();
        }
    }

    template <typename Mask>
    inline void tick_then_update(Mask&& m)
    {
        static_assert(width() == m.width() && height() == m.height(),
            "Mask and Zone must not differ in size!");

        if (m.tick())
        {
            uint8_t * ptr = __ptr0;
            for (uint16_t i = 0; i < height(); ++i)
            {
                m.get_line(i, ptr);
                ptr += Display::width();
            }
        }
    }
};

template <typename Z, typename M, typename... ZnM>
inline void tick_all(Z&& z, M&& m, ZnM... z_and_m)
{
    if (m.tick())
    {
        z.update(m);
    }

    tick_all(z_and_m...);
}

template <typename Z, typename M>
inline void tick_all(Z&& z, M&& m)
{
    if (m.tick())
    {
        z.update(m);
    }
}


























}

