#pragma once
#include <windows.h>
#include <gdiplus.h>
#include "zhuziString.h"
#include "zhuziFont.h"
#include <cmath>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "msimg32.lib")

namespace zhuzi {

    class zhuziColor {
    public:
        // 构造
        zhuziColor() : m_color(RGB(0, 0, 0)) {}
        zhuziColor(BYTE r, BYTE g, BYTE b, BYTE a = 255)
            : m_color(RGB(r, g, b)), m_alpha(a) {
        }
        zhuziColor(COLORREF cr, BYTE a = 255)
            : m_color(cr), m_alpha(a) {
        }
        zhuziColor(const Gdiplus::Color& color)
            : m_color(RGB(color.GetR(), color.GetG(), color.GetB())),
            m_alpha(color.GetA()) {
        }

        // 转换为 Gdiplus::Color（用于绘图）
        Gdiplus::Color toGdiplusColor() const {
            return Gdiplus::Color(m_alpha, GetR(), GetG(), GetB());
        }
        operator Gdiplus::Color() const { return toGdiplusColor(); }

        // 转换为 COLORREF
        COLORREF toCOLORREF() const { return m_color; }
        operator COLORREF() const { return m_color; }

        // 获取 RGBA 分量
        BYTE GetR() const { return GetRValue(m_color); }
        BYTE GetG() const { return GetGValue(m_color); }
        BYTE GetB() const { return GetBValue(m_color); }
        BYTE GetA() const { return m_alpha; }

        // 设置 alpha（透明度）
        void setAlpha(BYTE a) { m_alpha = a; }

    private:
        COLORREF m_color;
        BYTE m_alpha; // 透明度 0-255
    };

    class zhuziPen {
    public:
        zhuziPen(const zhuziColor& color, float width = 1.0f, Gdiplus::DashStyle style = Gdiplus::DashStyleSolid)
            : m_pen(new Gdiplus::Pen(color, width)) {
            m_pen->SetDashStyle(style);
        }
        ~zhuziPen() { delete m_pen; }
        Gdiplus::Pen* get() const { return m_pen; }
    private:
        Gdiplus::Pen* m_pen;
    };

    class zhuziBrush {
    public:
        zhuziBrush(const zhuziColor& color) : m_brush(new Gdiplus::SolidBrush(color)) {}
        ~zhuziBrush() { delete m_brush; }
        Gdiplus::Brush* get() const { return m_brush; }
    private:
        Gdiplus::Brush* m_brush;
    };

    class zhuziDC {
    public:
        zhuziDC(HWND hwnd) : m_hwnd(hwnd), m_hdc(GetDC(hwnd)), m_graphics(m_hdc) {
            m_graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        }
        ~zhuziDC() { if (m_hdc) ReleaseDC(m_hwnd, m_hdc); }
        Gdiplus::Graphics& getGraphics() { return m_graphics; }
        HDC getHDC() const { return m_hdc; }
        HWND getHwnd() const { return m_hwnd; }
    private:
        HWND m_hwnd;
        HDC m_hdc;
        Gdiplus::Graphics m_graphics;
    };

    class zhuziPaint {
    public:
        zhuziPaint(HDC hdc, const RECT& clientRect)
            : m_graphics(hdc), m_clientRect(clientRect) {
            m_graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            m_graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            m_graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
        }

        void clear(const zhuziColor& color) {
            zhuziBrush brush(color);
            m_graphics.FillRectangle(brush.get(), 0, 0, getWidth(), getHeight());
        }

        void drawLine(int x1, int y1, int x2, int y2, const zhuziPen& pen) {
            m_graphics.DrawLine(pen.get(), x1, y1, x2, y2);
        }

        void drawRect(int x, int y, int width, int height, const zhuziPen& pen, const zhuziBrush* brush = nullptr) {
            if (brush) m_graphics.FillRectangle(brush->get(), x, y, width, height);
            m_graphics.DrawRectangle(pen.get(), x, y, width, height);
        }

        void drawCircle(int cx, int cy, int radius, const zhuziPen& pen, const zhuziBrush* brush = nullptr) {
            if (brush) m_graphics.FillEllipse(brush->get(), cx - radius, cy - radius, radius * 2, radius * 2);
            m_graphics.DrawEllipse(pen.get(), cx - radius, cy - radius, radius * 2, radius * 2);
        }

        void drawText(const zhuziString& text, int x, int y, const zhuziBrush& brush, const zhuziFont& font) {
            Gdiplus::Font gdiFont(font.getFontFamily(), (Gdiplus::REAL)font.getSize(), font.getStyle());
            Gdiplus::PointF point((float)x, (float)y);
            m_graphics.DrawString(text.c_str(), (int)text.length(), &gdiFont, point, brush.get());
        }

        void drawText(const zhuziString& text, const zhuziFont& font, const zhuziBrush& brush,
            const RECT& rect, DWORD format = DT_CENTER | DT_VCENTER | DT_SINGLELINE) {
            float fontSize = (float)font.getSize();
            if (fontSize <= 0) fontSize = 16.0f;
            Gdiplus::Font gdiFont(font.getFontFamily(), fontSize, font.getStyle(), Gdiplus::UnitPixel);
            Gdiplus::StringFormat stringFormat;

            if (format & DT_CENTER)
                stringFormat.SetAlignment(Gdiplus::StringAlignmentCenter);
            else if (format & DT_RIGHT)
                stringFormat.SetAlignment(Gdiplus::StringAlignmentFar);
            else
                stringFormat.SetAlignment(Gdiplus::StringAlignmentNear);

            if (format & DT_VCENTER)
                stringFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);
            else if (format & DT_BOTTOM)
                stringFormat.SetLineAlignment(Gdiplus::StringAlignmentFar);
            else
                stringFormat.SetLineAlignment(Gdiplus::StringAlignmentNear);

            if (format & DT_SINGLELINE)
                stringFormat.SetFormatFlags(stringFormat.GetFormatFlags() | Gdiplus::StringFormatFlagsNoWrap);

            Gdiplus::RectF layoutRect((Gdiplus::REAL)rect.left, (Gdiplus::REAL)rect.top,
                (Gdiplus::REAL)(rect.right - rect.left), (Gdiplus::REAL)(rect.bottom - rect.top));
            m_graphics.DrawString(text.c_str(), (int)text.length(), &gdiFont, layoutRect, &stringFormat, brush.get());
        }

        void measureText(const zhuziString& text, const zhuziFont& font, SIZE& size) const {
            Gdiplus::Font gdiFont(font.getFontFamily(), (Gdiplus::REAL)font.getSize(), font.getStyle(), Gdiplus::UnitPixel);
            Gdiplus::RectF bounds;
            m_graphics.MeasureString(text.c_str(), (int)text.length(), &gdiFont, Gdiplus::PointF(0, 0), &bounds);
            size.cx = (int)ceil(bounds.Width);
            size.cy = (int)ceil(bounds.Height);
        }

        void fillRect(int x, int y, int width, int height, const zhuziBrush& brush) {
            m_graphics.FillRectangle(brush.get(), x, y, width, height);
        }

        void drawRoundRect(int x, int y, int width, int height, int radius, const zhuziPen& pen, const zhuziBrush* brush = nullptr) {
            if (radius <= 0) {
                drawRect(x, y, width, height, pen, brush);
                return;
            }
            Gdiplus::GraphicsPath path;
            path.AddArc(x, y, radius * 2, radius * 2, 180, 90);
            path.AddArc(x + width - radius * 2, y, radius * 2, radius * 2, 270, 90);
            path.AddArc(x + width - radius * 2, y + height - radius * 2, radius * 2, radius * 2, 0, 90);
            path.AddArc(x, y + height - radius * 2, radius * 2, radius * 2, 90, 90);
            path.CloseFigure();
            if (brush) m_graphics.FillPath(brush->get(), &path);
            m_graphics.DrawPath(pen.get(), &path);
        }

        int getWidth() const { return m_clientRect.right - m_clientRect.left; }
        int getHeight() const { return m_clientRect.bottom - m_clientRect.top; }

        Gdiplus::Graphics& getGraphics() { return m_graphics; }

        HDC getHDC() { return m_graphics.GetHDC(); }
        void releaseHDC(HDC hdc) { m_graphics.ReleaseHDC(hdc); }
    private:
        Gdiplus::Graphics m_graphics;
        RECT m_clientRect;
    };

} // namespace zhuzi