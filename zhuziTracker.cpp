#define _WIN32_WINNT 0x0600
#include "zhuziTracker.h"
#include "zhuziInstance.h"
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

#ifndef TBM_GETRANGE
#define TBM_GETRANGE (WM_USER+106)
#endif
#ifndef TBM_GETRANGEMAX
#define TBM_GETRANGEMAX (WM_USER+104)
#endif
#ifndef TBM_GETSELSTART
#define TBM_GETSELSTART (WM_USER+111)
#endif
#ifndef TBM_GETSELEND
#define TBM_GETSELEND (WM_USER+112)
#endif

namespace zhuzi {

    zhuziTracker::zhuziTracker(zhuziControl* parent)
        : zhuziControl(parent), m_orientation(Orientation::Horizontal), m_autoTicks(true), m_tickFreq(1), m_onPosChange(nullptr) {
    }

    zhuziTracker::~zhuziTracker() {
        destroy();
    }

    bool zhuziTracker::onCreate(DWORD style) {
        DWORD finalStyle = style | WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS;
        if (m_orientation == Orientation::Vertical) finalStyle |= TBS_VERT;
        if (!createControl(TRACKBAR_CLASSW, 0, 0, 0, 0, finalStyle))
            return false;

        // 向父窗口注册链式消息处理，捕获滚动消息
        zhuziWindow* pWin = dynamic_cast<zhuziWindow*>(m_parent);
        if (pWin) {
            auto handler = [this](WPARAM wParam, LPARAM lParam) -> bool {
                // 判断消息是否来自当前滑动条
                if ((HWND)lParam == m_hwnd) {
                    int code = LOWORD(wParam);
                    if (code == SB_THUMBPOSITION || code == SB_ENDSCROLL || code == SB_THUMBTRACK) {
                        if (m_onPosChange) {
                            m_onPosChange(getPos());
                        }
                    }
                    // 返回 false 以便消息继续传递（如果有其他控件也要处理）
                    return false;
                }
                return false;
                };
            pWin->BindChain(WM_HSCROLL, handler);
            pWin->BindChain(WM_VSCROLL, handler);
        }

        setRange(0, 100);
        return true;
    }

    void zhuziTracker::setRange(int min, int max) {
        if (m_hwnd) {
            SendMessageW(m_hwnd, TBM_SETRANGE, TRUE, MAKELPARAM(min, max));
        }
    }

    void zhuziTracker::getRange(int& min, int& max) const {
        if (m_hwnd) {
            SendMessageW(m_hwnd, TBM_GETRANGE, TRUE, (LPARAM)&min);
            max = (int)SendMessageW(m_hwnd, TBM_GETRANGEMAX, 0, 0);
        }
        else {
            min = max = 0;
        }
    }

    void zhuziTracker::setPos(int pos) {
        if (m_hwnd) {
            SendMessageW(m_hwnd, TBM_SETPOS, TRUE, pos);
        }
    }

    int zhuziTracker::getPos() const {
        if (!m_hwnd) return 0;
        return (int)SendMessageW(m_hwnd, TBM_GETPOS, 0, 0);
    }

    void zhuziTracker::setTickFreq(int freq) {
        m_tickFreq = freq;
        if (m_hwnd && m_autoTicks) {
            SendMessageW(m_hwnd, TBM_SETTICFREQ, freq, 0);
        }
    }

    void zhuziTracker::setTicks(bool autoTicks, int freq) {
        m_autoTicks = autoTicks;
        if (autoTicks) {
            if (m_hwnd) {
                LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
                style |= TBS_AUTOTICKS;
                SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);
                setTickFreq(freq);
            }
        }
        else {
            if (m_hwnd) {
                LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
                style &= ~TBS_AUTOTICKS;
                SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);
                SendMessageW(m_hwnd, TBM_CLEARTICS, 0, 0);
            }
        }
    }

    void zhuziTracker::setOrientation(Orientation orient) {
        m_orientation = orient;
        if (m_hwnd) {
            LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
            if (orient == Orientation::Vertical)
                style |= TBS_VERT;
            else
                style &= ~TBS_VERT;
            SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);
            SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }

    void zhuziTracker::setSelectionRange(bool enable) {
        if (m_hwnd) {
            LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
            if (enable)
                style |= TBS_ENABLESELRANGE;
            else
                style &= ~TBS_ENABLESELRANGE;
            SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);
        }
    }

    void zhuziTracker::setSelStart(int start) {
        if (m_hwnd) {
            SendMessageW(m_hwnd, TBM_SETSELSTART, TRUE, start);
        }
    }

    void zhuziTracker::setSelEnd(int end) {
        if (m_hwnd) {
            SendMessageW(m_hwnd, TBM_SETSELEND, TRUE, end);
        }
    }

    void zhuziTracker::getSelRange(int& start, int& end) const {
        if (m_hwnd) {
            SendMessageW(m_hwnd, TBM_GETSELSTART, 0, (LPARAM)&start);
            SendMessageW(m_hwnd, TBM_GETSELEND, 0, (LPARAM)&end);
        }
        else {
            start = end = 0;
        }
    }

    void zhuziTracker::setPageSize(int pageSize) {
        if (m_hwnd) {
            SendMessageW(m_hwnd, TBM_SETPAGESIZE, 0, pageSize);
        }
    }

    void zhuziTracker::setLineSize(int lineSize) {
        if (m_hwnd) {
            SendMessageW(m_hwnd, TBM_SETLINESIZE, 0, lineSize);
        }
    }

    void zhuziTracker::clearSel() {
        if (m_hwnd) {
            SendMessageW(m_hwnd, TBM_CLEARSEL, 0, 0);
        }
    }

    void zhuziTracker::setOnPosChange(std::function<void(int)> callback) {
        m_onPosChange = callback;
    }

} // namespace zhuzi