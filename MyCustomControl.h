#include "zhuziControl.h"
#include "zhuziPaint.h"
#include <functional>

using namespace zhuzi;

class MyCustomControl : public zhuziControl {
public:
    MyCustomControl(zhuziControl* parent = nullptr)
        : zhuziControl(parent), m_hovering(false), m_radius(0) {
        m_isCustomDraw = true;
    }

    void setRoundedRect(int radius) { 
        m_radius = radius; 
        if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    virtual bool onCreate(DWORD style) override {
        if (!createControl(L"STATIC", 0, 0, 0, 0, style | SS_NOTIFY))
            return false;
        TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_HOVER | TME_LEAVE, m_hwnd, 400 };
        TrackMouseEvent(&tme);
        return true;
    }

    virtual void onParentResize(int parentWidth, int parentHeight) override {
        zhuziControl::onParentResize(parentWidth, parentHeight);
        if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void setOnClick(std::function<void()> onc) { m_onClick = onc; }

protected:
    virtual void onPaint(zhuziPaint& paint) override {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;
        if (w <= 0 || h <= 0) return;

        // 首先用白色填充整个客户区，消除黑色背景
        zhuziBrush whiteBrush(zhuziColor(255, 255, 255));
        zhuziPen whitePen(zhuziColor(255, 255, 255));
        paint.drawRect(rc.left, rc.top, w, h, whitePen, &whiteBrush);

        int radius = m_radius;
        if (radius > w / 2) radius = w / 2;
        if (radius > h / 2) radius = h / 2;

        // 背景画刷（根据悬停状态）
        zhuziBrush bgBrush(m_hovering ? zhuziColor(0x3C, 0x3C, 0x3C) : zhuziColor(0x33, 0x00, 0x66));
        // 边框画笔
        zhuziPen borderPen(zhuziColor(0x44, 0x00, 0x66), 3.0f);

        if (radius > 0) {
            // 绘制圆角矩形背景和边框（需要 drawRoundRect 方法，见下文）
            paint.drawRoundRect(rc.left, rc.top, w, h, radius, borderPen, &bgBrush);
        } else {
            paint.fillRect(rc.left, rc.top, w, h, bgBrush);
            paint.drawRect(rc.left, rc.top, w, h, borderPen);
        }

        // 绘制文本
        zhuziString text = getText();
        if (!text.empty()) {
            zhuziBrush textBrush(zhuziColor(255, 255, 255));
            zhuziFont fnt = getFont();
            paint.drawText(text, fnt, textBrush, rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }

    virtual void onMouseMove(int x, int y) override {
        if (!m_hovering) {
            m_hovering = true;
            InvalidateRect(m_hwnd, nullptr, TRUE);
        }
        TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hwnd, 0 };
        TrackMouseEvent(&tme);
    }

    virtual void onMouseLeave() override {
        if (m_hovering) {
            m_hovering = false;
            InvalidateRect(m_hwnd, nullptr, TRUE);
        }
    }

    virtual void onLButtonUp(int x, int y) override {
        if (m_onClick) m_onClick();
    }

private:
    std::function<void()> m_onClick;
    bool m_hovering;
    int m_radius;
};