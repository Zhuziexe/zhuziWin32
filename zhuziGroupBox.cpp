#include "zhuziGroupBox.h"
#include "zhuziInstance.h"
#include "zhuziControl.h"
#include "zhuziCommctrl.h"
#include <commctrl.h>

namespace zhuzi {

    zhuziGroupBox::zhuziGroupBox(zhuziControl* parent)
        : zhuziControl(parent)
        , m_textAlign(0)
        , m_bgColor(0)
        , m_hasBgColor(false)
        , m_hBgBrush(nullptr)
    {
    }

    zhuziGroupBox::~zhuziGroupBox()
    {
        if (m_hBgBrush) DeleteObject(m_hBgBrush);
        destroy();
    }

    // 子类窗口过程
    LRESULT CALLBACK zhuziGroupBox::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
    {
        zhuziGroupBox* pThis = (zhuziGroupBox*)dwRefData;
        if (!pThis) return DefSubclassProc(hwnd, msg, wParam, lParam);

        // 背景色处理（如果设置了背景色）
        if (msg == WM_CTLCOLORSTATIC && pThis->m_hasBgColor && pThis->m_hBgBrush) {
            HDC hdc = (HDC)wParam;
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
            return (LRESULT)pThis->m_hBgBrush;
        }

        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    bool zhuziGroupBox::onCreate(DWORD style)
    {
        DWORD dwStyle = style | BS_GROUPBOX;
        switch (m_textAlign) {
        case 0: dwStyle |= BS_LEFT; break;
        case 1: dwStyle |= BS_CENTER; break;
        case 2: dwStyle |= BS_RIGHT; break;
        }

        if (!createControl(L"BUTTON", 0, 0, 0, 0, dwStyle))
            return false;

        // 设置子类化，用于转发 WM_COMMAND 和背景色处理
        SetWindowSubclass(m_hwnd, SubclassProc, 0, (DWORD_PTR)this);

        // 应用之前保存的文本
        if (!m_pendingText.empty()) {
            zhuziControl::setText(m_pendingText);
        }

        // 背景色资源（如果已有）
        if (m_hasBgColor) {
            updateBackgroundBrush();
        }

        return true;
    }

    void zhuziGroupBox::updateBackgroundBrush()
    {
        if (m_hBgBrush) DeleteObject(m_hBgBrush);
        m_hBgBrush = CreateSolidBrush(m_bgColor);
    }

    void zhuziGroupBox::applyStyles()
    {
        if (!m_hwnd) return;
        LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
        style &= ~(BS_LEFT | BS_CENTER | BS_RIGHT);
        switch (m_textAlign) {
        case 0: style |= BS_LEFT; break;
        case 1: style |= BS_CENTER; break;
        case 2: style |= BS_RIGHT; break;
        }
        SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);
        SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void zhuziGroupBox::setTextAlignLeft() { m_textAlign = 0; applyStyles(); }
    void zhuziGroupBox::setTextAlignCenter() { m_textAlign = 1; applyStyles(); }
    void zhuziGroupBox::setTextAlignRight() { m_textAlign = 2; applyStyles(); }

    void zhuziGroupBox::setBackgroundColor(COLORREF color)
    {
        m_bgColor = color;
        m_hasBgColor = true;
        updateBackgroundBrush();
        if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void zhuziGroupBox::setBackgroundSysColor(int sysColorIndex)
    {
        setBackgroundColor(GetSysColor(sysColorIndex));
    }

    void zhuziGroupBox::clearBackgroundColor()
    {
        m_hasBgColor = false;
        if (m_hBgBrush) DeleteObject(m_hBgBrush);
        m_hBgBrush = nullptr;
        if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void zhuziGroupBox::enable(bool enabled)
    {
        zhuziControl::enable(enabled);
    }

    void zhuziGroupBox::setText(const zhuziString& text)
    {
        m_pendingText = text;
        if (m_hwnd) {
            zhuziControl::setText(text);
        }
    }

} // namespace zhuzi