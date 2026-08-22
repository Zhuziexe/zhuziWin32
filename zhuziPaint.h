#pragma once

#include <windows.h>
#include <gdiplus.h>
#include "zhuziString.h"
#include "zhuziFont.h"
#include <cmath>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "msimg32.lib")

namespace zhuzi {

    // ---------- 颜色 ----------
    class zhuziColor {
    public:
        zhuziColor();
        zhuziColor(BYTE r, BYTE g, BYTE b, BYTE a = 255);
        zhuziColor(COLORREF cr, BYTE a = 255);
        zhuziColor(const Gdiplus::Color& color);

        Gdiplus::Color toGdiplusColor() const;
        operator Gdiplus::Color() const;
        COLORREF toCOLORREF() const;
        operator COLORREF() const;

        BYTE getR() const;
        BYTE getG() const;
        BYTE getB() const;
        BYTE getA() const;
        void setAlpha(BYTE a);

    private:
        BYTE m_r, m_g, m_b, m_alpha;
    };

    // ---------- 画笔 ----------
    class zhuziPen {
    public:
        zhuziPen(const zhuziColor& color, float width = 1.0f, Gdiplus::DashStyle style = Gdiplus::DashStyleSolid);
        ~zhuziPen();

        zhuziPen(const zhuziPen&) = delete;
        zhuziPen& operator=(const zhuziPen&) = delete;
        zhuziPen(zhuziPen&& other) noexcept;
        zhuziPen& operator=(zhuziPen&& other) noexcept;

        Gdiplus::Pen* get() const;

    private:
        Gdiplus::Pen* m_pen;
    };

    // ---------- 画刷 ----------
    class zhuziBrush {
    public:
        zhuziBrush(const zhuziColor& color);
        zhuziBrush(HBRUSH hBrush);
        zhuziBrush(const zhuziColor& color1, const zhuziColor& color2,
            const POINT& point1, const POINT& point2);
        zhuziBrush(const zhuziColor& color1, const zhuziColor& color2,
            const RECT& rect, float angle, bool isAngleScalable = false);
        ~zhuziBrush();

        zhuziBrush(const zhuziBrush&) = delete;
        zhuziBrush& operator=(const zhuziBrush&) = delete;
        zhuziBrush(zhuziBrush&& other) noexcept;
        zhuziBrush& operator=(zhuziBrush&& other) noexcept;

        Gdiplus::Brush* get() const;

    private:
        Gdiplus::Brush* m_brush;
    };

    // ---------- 路径封装 ----------
    class zhuziPath {
    public:
        zhuziPath();
        ~zhuziPath();

        zhuziPath(zhuziPath&& other) noexcept;
        zhuziPath& operator=(zhuziPath&& other) noexcept;

        zhuziPath(const zhuziPath&) = delete;
        zhuziPath& operator=(const zhuziPath&) = delete;

        // 添加基本图形（参数使用 GDI32 原生类型）
        void addLine(const POINT& p1, const POINT& p2);
        void addRectangle(const RECT& rect);
        void addEllipse(const RECT& rect);
        void addArc(const RECT& rect, float startAngle, float sweepAngle);
        void addBezier(const POINT& p1, const POINT& p2, const POINT& p3, const POINT& p4);
        void addPolygon(const POINT* points, int count);
        void addCurve(const POINT* points, int count);

        // 添加文字（使用点或矩形定位）
        void addString(const zhuziString& text, const zhuziFont& font, const POINT& origin);
        void addString(const zhuziString& text, const zhuziFont& font, const RECT& layoutRect);

        void closeFigure();
        void reset();

        // 仅供内部使用
        Gdiplus::GraphicsPath* getNative() const;

    private:
        Gdiplus::GraphicsPath* m_path;
    };

    // ---------- 设备上下文辅助 ----------
    class zhuziDC {
    public:
        zhuziDC(HWND hwnd);
        ~zhuziDC();

        Gdiplus::Graphics& getGraphics();
        HDC getHDC() const;
        HWND getHwnd() const;

    private:
        HWND m_hwnd;
        HDC  m_hdc;
        Gdiplus::Graphics m_graphics;
    };

    // ---------- 绘图主类 ----------
    class zhuziPaint {
    public:
        zhuziPaint(HDC hdc, const RECT& clientRect);
        ~zhuziPaint() = default;

        void clear(const zhuziColor& color);

        void drawLine(int x1, int y1, int x2, int y2, const zhuziPen& pen);
        void drawRect(int x, int y, int width, int height, const zhuziPen& pen, const zhuziBrush* brush = nullptr);
        void fillRect(int x, int y, int width, int height, const zhuziBrush& brush);
        void drawCircle(int cx, int cy, int radius, const zhuziPen& pen, const zhuziBrush* brush = nullptr);
        void drawRoundRect(int x, int y, int width, int height, int radius, const zhuziPen& pen, const zhuziBrush* brush = nullptr);
        void fillRoundRect(int x, int y, int width, int height, int radius, const zhuziBrush& brush);

        // 文字绘制
        void drawText(const zhuziString& text, int x, int y, const zhuziBrush& brush, const zhuziFont& font);
        void drawText(const zhuziString& text, const zhuziFont& font, const zhuziBrush& brush,
            const RECT& rect, DWORD format = DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        void measureText(const zhuziString& text, const zhuziFont& font, SIZE& size) const;

        // 路径绘制
        void drawPath(const zhuziPath& path, const zhuziPen& pen, const zhuziBrush* brush = nullptr);
        void fillPath(const zhuziPath& path, const zhuziBrush& brush);

        int getWidth() const;
        int getHeight() const;

        Gdiplus::Graphics& getGraphics();
        HDC getHDC();
        void releaseHDC(HDC hdc);

    private:
        Gdiplus::Graphics m_graphics;
        RECT m_clientRect;
    };

} // namespace zhuzi