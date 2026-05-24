#include "zhuziProgressBar.h"
#include "zhuziInstance.h"
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

namespace zhuzi {

    zhuziProgressBar::zhuziProgressBar(zhuziControl* parent)
        : zhuziControl(parent), m_styleExtra(0) {}

    zhuziProgressBar::~zhuziProgressBar() { destroy(); }
    bool zhuziProgressBar::onCreate(DWORD style) {
        DWORD finalStyle = style | WS_CHILD | WS_VISIBLE;
        if (!createControl(PROGRESS_CLASS, 0, 0, 0, 0, finalStyle))
            return false;
        setRange(0, 100);
        return true;
    }

    void zhuziProgressBar::setRange(int min, int max) {
        if (m_hwnd) SendMessageW(m_hwnd, PBM_SETRANGE, 0, MAKELPARAM(min, max));
    }

    void zhuziProgressBar::setRange32(int min, int max) {
        if (m_hwnd) SendMessageW(m_hwnd, PBM_SETRANGE32, min, max);
    }

    void zhuziProgressBar::getRange(int& min, int& max) const {
        if (m_hwnd) {
            DWORD range = (DWORD)SendMessageW(m_hwnd, PBM_GETRANGE, TRUE, 0);
            min = LOWORD(range);
            max = HIWORD(range);
        }
        else {
            min = max = 0;
        }
    }

    int zhuziProgressBar::getPos() const {
        if (m_hwnd) return (int)SendMessageW(m_hwnd, PBM_GETPOS, 0, 0);
        return 0;
    }

    void zhuziProgressBar::setPos(int pos) {
        if (m_hwnd) SendMessageW(m_hwnd, PBM_SETPOS, pos, 0);
    }

    void zhuziProgressBar::deltaPos(int delta) {
        if (m_hwnd) SendMessageW(m_hwnd, PBM_DELTAPOS, delta, 0);
    }

    void zhuziProgressBar::setStep(int step) {
        if (m_hwnd) SendMessageW(m_hwnd, PBM_SETSTEP, step, 0);
    }

    void zhuziProgressBar::stepIt() {
        if (m_hwnd) SendMessageW(m_hwnd, PBM_STEPIT, 0, 0);
    }

    void zhuziProgressBar::setSmooth(bool smooth) {
        if (!m_hwnd) return;
        LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
        if (smooth)
            style |= PBS_SMOOTH;
        else
            style &= ~PBS_SMOOTH;
        SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void zhuziProgressBar::setVertical(bool vertical) {
        if (!m_hwnd) return;
        LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
        if (vertical)
            style |= PBS_VERTICAL;
        else
            style &= ~PBS_VERTICAL;
        SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);
        // 垂直样式需要重新设置大小以正确重绘
        RECT rc;
        GetWindowRect(m_hwnd, &rc);
        SetWindowPos(m_hwnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    void zhuziProgressBar::setMarquee(bool enable, DWORD timeMs) {
        if (!m_hwnd) return;
        if (enable) {
            // 设置 Marquee 样式
            LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
            style |= PBS_MARQUEE;
            SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);
            // 启动动画，timeMs 为刷新间隔（毫秒），若为0则使用系统默认（约30ms）
            SendMessageW(m_hwnd, PBM_SETMARQUEE, enable, timeMs);
        }
        else {
            // 停止动画并移除样式
            SendMessageW(m_hwnd, PBM_SETMARQUEE, FALSE, 0);
            LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
            style &= ~PBS_MARQUEE;
            SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);
        }
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    bool zhuziProgressBar::isMarquee() const {
        if (!m_hwnd) return false;
        return (GetWindowLongPtrW(m_hwnd, GWL_STYLE) & PBS_MARQUEE) != 0;
    }

    void zhuziProgressBar::setBarColor(COLORREF color) {
        if (m_hwnd) SendMessageW(m_hwnd, PBM_SETBARCOLOR, 0, color);
    }

    void zhuziProgressBar::setBackgroundColor(COLORREF color) {
        if (m_hwnd) SendMessageW(m_hwnd, PBM_SETBKCOLOR, 0, color);
    }

} // namespace zhuzi