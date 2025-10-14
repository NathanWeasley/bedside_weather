#pragma once

#include "gfx_animated.hpp"

namespace gfx
{

typedef enum : uint8_t
{
    DOT_FILL_AROUND = 1,
    DOT_FILL_RIGHTUP
} dot_fill_e;

typedef enum : uint8_t
{
    DOT_SIZE_1 = 1,
    DOT_SIZE_2,
    DOT_SIZE_3,
    DOT_SIZE_4,
    DOT_SIZE_5,
    DOT_SIZE_6,
    DOT_SIZE_7,
    DOT_SIZE_8
} dot_size_e;

typedef enum : uint8_t
{
    LINE_SOLID = 0,
    LINE_DOTTED
} line_style_e;

typedef enum : uint8_t
{
    SHAPE_FILL = 1,
    SHAPE_NOFILL = 0
} shape_fill_e;



template <typename T>
static inline void set_whole(uint8_t color)
{
    if constexpr (is_Canvas_v<T>)
    {
        memset(T::data(), color, T::size());
    }
    else if constexpr (is_Window_v<T>)
    {
        for (uint16_t i = 0; i < T::height(); ++i)
        {
            uint8_t * ptr = T::data() + T::canvas().width() * i;
            memset(ptr, color, T::width());
        }
    }
}

template <typename T, typename = std::enable_if_t<is_Window_v<T>>>
static inline void draw_px(uint16_t x, uint16_t y, uint8_t color)
{
    if (x >= T::width() || y >= T::height())
        return;

    T::data()[x + T::canvas().width()*y] = color;
}

template <typename T, typename = std::enable_if_t<is_Window_v<T>>>
void draw_pt(uint16_t x, uint16_t y, uint16_t color, uint8_t size, dot_fill_e fill = DOT_FILL_AROUND)
{
    if (x >= T::width() || y >= T::height())
    {
        return;
    }

    int16_t XDir_Num , YDir_Num;
    if (fill == DOT_FILL_AROUND)
    {
        for (XDir_Num = 0; XDir_Num < 2 * size - 1; ++XDir_Num)
        {
            for (YDir_Num = 0; YDir_Num < 2 * size - 1; ++YDir_Num)
            {
                if(x + XDir_Num - size < 0 || y + YDir_Num - size < 0)
                    break;

                draw_px<T>(x + XDir_Num - size, y + YDir_Num - size, color);
            }
        }
    }
    else
    {
        for (XDir_Num = 0; XDir_Num < size; XDir_Num++)
        {
            for (YDir_Num = 0; YDir_Num < size; YDir_Num++)
            {
                draw_px<T>(x + XDir_Num - 1, y + YDir_Num - 1, color);
            }
        }
    }
}

template <typename T, typename = std::enable_if_t<is_Window_v<T>>>
static inline void draw_line(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color, uint8_t width, line_style_e style)
{
    if (x0 >= T::width() || y0 >= T::height() || x1 >= T::width() || y1 >= T::height())
        return;

    uint16_t Xpoint = x0;
    uint16_t Ypoint = y0;
    int dx = (int)x1 - (int)x0 >= 0 ? x1 - x0 : x0 - x1;
    int dy = (int)y1 - (int)y0 <= 0 ? y1 - y0 : y0 - y1;

    // Increment direction, 1 is positive, -1 is counter;
    int XAddway = x0 < x1 ? 1 : -1;
    int YAddway = y0 < y1 ? 1 : -1;

    //Cumulative error
    int Esp = dx + dy;
    uint8_t Dotted_Len = 0;

    for (;;)
    {
        ++Dotted_Len;
        //Painted dotted line, 2 point is really virtual
        if (style == LINE_DOTTED && Dotted_Len % 3 == 0)
        {
            // draw_pt<T>(Xpoint, Ypoint, 0, width);
            Dotted_Len = 0;
        }
        else
        {
            draw_pt<T>(Xpoint, Ypoint, color, width);
        }

        if (2 * Esp >= dy)
        {
            if (Xpoint == x1)
                break;
            Esp += dy;
            Xpoint += XAddway;
        }
        if (2 * Esp <= dx)
        {
            if (Ypoint == y1)
                break;
            Esp += dx;
            Ypoint += YAddway;
        }
    }
}

template <typename T, typename = std::enable_if_t<is_Window_v<T>>>
void draw_rectangle(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                         uint16_t color, uint8_t width, shape_fill_e fill)
{
    if (x0 > T::width() || y0 > T::height() || x1 > T::width() || y1 > T::height())
    {
        return;
    }

    if (fill)
    {
        uint16_t Ypoint;
        for(Ypoint = y0; Ypoint <= y1; Ypoint++)
        {
            draw_line<T>(x0, Ypoint, x1, Ypoint, color, width, LINE_SOLID);
        }
    }
    else
    {
        draw_line<T>(x0, y0, x1, y0, color, width, LINE_SOLID);
        draw_line<T>(x0, y0, x0, y1, color, width, LINE_SOLID);
        draw_line<T>(x1, y1, x1, y0, color, width, LINE_SOLID);
        draw_line<T>(x1, y1, x0, y1, color, width, LINE_SOLID);
    }
}

template <typename T, typename = std::enable_if_t<is_Window_v<T>>>
void draw_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color, uint8_t width, shape_fill_e fill)
{
    if (x > T::width() || y >= T::height())
    {
        return;
    }

    //Draw a circle from(0, R) as a starting point
    int16_t XCurrent, YCurrent;
    XCurrent = 0;
    YCurrent = r;

    //Cumulative error,judge the next point of the logo
    int16_t Esp = 3 - (r << 1);

    int16_t sCountY;
    if (fill == SHAPE_FILL)
    {
        while (XCurrent <= YCurrent)
        { //Realistic circles
            for (sCountY = XCurrent; sCountY <= YCurrent; ++sCountY)
            {
                draw_pt<T>(x + XCurrent, y + sCountY, color, 1);//1
                draw_pt<T>(x - XCurrent, y + sCountY, color, 1);//2
                draw_pt<T>(x - sCountY, y + XCurrent, color, 1);//3
                draw_pt<T>(x - sCountY, y - XCurrent, color, 1);//4
                draw_pt<T>(x - XCurrent, y - sCountY, color, 1);//5
                draw_pt<T>(x + XCurrent, y - sCountY, color, 1);//6
                draw_pt<T>(x + sCountY, y - XCurrent, color, 1);//7
                draw_pt<T>(x + sCountY, y + XCurrent, color, 1);
            }
            if (Esp < 0)
            {
                Esp += 4 * XCurrent + 6;
            }
            else
            {
                Esp += 10 + 4 * (XCurrent - YCurrent);
                --YCurrent;
            }
            ++XCurrent;
        }
    }
    else
    { //Draw a hollow circle
        while (XCurrent <= YCurrent)
        {
            draw_pt<T>(x + XCurrent, y + YCurrent, color, width);   //1
            draw_pt<T>(x - XCurrent, y + YCurrent, color, width);   //2
            draw_pt<T>(x - YCurrent, y + XCurrent, color, width);   //3
            draw_pt<T>(x - YCurrent, y - XCurrent, color, width);   //4
            draw_pt<T>(x - XCurrent, y - YCurrent, color, width);   //5
            draw_pt<T>(x + XCurrent, y - YCurrent, color, width);   //6
            draw_pt<T>(x + YCurrent, y - XCurrent, color, width);   //7
            draw_pt<T>(x + YCurrent, y + XCurrent, color, width);   //0

            if (Esp < 0)
            {
                Esp += 4 * XCurrent + 6;
            }
            else
            {
                Esp += 10 + 4 * (XCurrent - YCurrent);
                --YCurrent;
            }
            ++XCurrent;
        }
    }
}


template <typename T, typename = std::enable_if_t<is_Window_v<T>>>
uint16_t draw_icon(uint16_t x, uint16_t y, uint16_t icon)
{
    if (icon >= )
}












#if 0

template <typename T>
static inline void dump(const char * path)
{
    FILE * fid = fopen(path, "w");
    if (fid)
    {
        fwrite(T::data(), T::size(), 1, fid);

        fclose(fid);
    }
}

#endif

}

