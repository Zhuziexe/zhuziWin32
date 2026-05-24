#include "zhuziScrollArea.h"
#include "zhuziInstance.h"
#include <windowsx.h>
#include <algorithm>

namespace zhuzi {

    const wchar_t* zhuziScrollArea::SCROLL_AREA_CLASS_NAME = L"zhuziScrollAreaClass";
    bool zhuziScrollArea::s_classRegistered = false;

    zhuziScrollArea::zhuziScrollArea(zhuziControl* parent)
        : zhuziControl(parent), m_contentFrame(nullptr),
        m_scrollX(0), m_scrollY(0),
        m_contentWidth(0), m_contentHeight(0),
        m_hPolicy(AsNeeded), m_vPolicy(AsNeeded) {
    }

    zhuziScrollArea::~zhuziScrollArea() {
        destroy();
    }

    bool zhuziScrollArea::RegisterScrollAreaClass() {
        if (s_classRegistered) return true;
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = ScrollAreaWndProc;
        wc.hInstance = zhuziInstance::getHandle();
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = SCROLL_AREA_CLASS_NAME;
        s_classRegistered = (RegisterClassExW(&wc) != 0);
        return s_classRegistered;
    }

    bool zhuziScrollArea::onCreate(DWORD style) {
        if (!RegisterScrollAreaClass()) return false;

        // 直接创建自定义窗口，不通过 createControl
        if (m_id == -1) {
            try { m_id = allocateId(); }
            catch (...) { return false; }
        }
        HWND hParent = m_parent ? m_parent->getHandle() : nullptr;
        DWORD dwStyle = style | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;
        m_hwnd = CreateWindowExW(0, SCROLL_AREA_CLASS_NAME, L"", dwStyle,
            0, 0, 0, 0, hParent, (HMENU)(INT_PTR)m_id,
            zhuziInstance::getHandle(), this);
        if (!m_hwnd) {
            releaseId(m_id);
            m_id = -1;
            return false;
        }
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

        // 创建内容面板
        m_contentFrame = new zhuziFrame(this);
        if (!m_contentFrame->create(0, 0, 0, 0)) {
            delete m_contentFrame;
            m_contentFrame = nullptr;
            return false;
        }
        m_contentFrame->setBackgroundSysColor(COLOR_WINDOW);

        updateScrollBars();
        updateContentPosition();
        return true;
    }

    void zhuziScrollArea::setContentSize(int width, int height) {
        if (!m_contentFrame) return;
        m_contentWidth = width;
        m_contentHeight = height;
        m_contentFrame->setRect(0, 0, width, height);
        updateScrollBars();
        updateContentPosition();
    }

    void zhuziScrollArea::autoFitContent(bool adjustChildPositions) {
        if (!m_contentFrame || !m_contentFrame->getHandle()) return;

        HWND hChild = GetWindow(m_contentFrame->getHandle(), GW_CHILD);
        if (!hChild) {
            setContentSize(0, 0);
            return;
        }

        int minX = 0, minY = 0, maxRight = 0, maxBottom = 0;
        bool first = true;

        while (hChild) {
            zhuziControl* ctrl = (zhuziControl*)GetWindowLongPtrW(hChild, GWLP_USERDATA);
            if (ctrl) {
                RECT rc;
                GetWindowRect(hChild, &rc);
                MapWindowPoints(HWND_DESKTOP, m_contentFrame->getHandle(), (LPPOINT)&rc, 2);
                int left = rc.left;
                int top = rc.top;
                int right = rc.right;
                int bottom = rc.bottom;

                if (first) {
                    minX = left;
                    minY = top;
                    maxRight = right;
                    maxBottom = bottom;
                    first = false;
                }
                else {
                    if (left < minX) minX = left;
                    if (top < minY) minY = top;
                    if (right > maxRight) maxRight = right;
                    if (bottom > maxBottom) maxBottom = bottom;
                }
            }
            hChild = GetWindow(hChild, GW_HWNDNEXT);
        }

        if (first) {
            setContentSize(0, 0);
            return;
        }

        int newWidth = maxRight - minX;
        int newHeight = maxBottom - minY;

        if (adjustChildPositions && (minX != 0 || minY != 0)) {
            hChild = GetWindow(m_contentFrame->getHandle(), GW_CHILD);
            while (hChild) {
                zhuziControl* ctrl = (zhuziControl*)GetWindowLongPtrW(hChild, GWLP_USERDATA);
                if (ctrl) {
                    RECT rc;
                    GetWindowRect(hChild, &rc);
                    MapWindowPoints(HWND_DESKTOP, m_contentFrame->getHandle(), (LPPOINT)&rc, 2);
                    int newLeft = rc.left - minX;
                    int newTop = rc.top - minY;
                    ctrl->setRect(newLeft, newTop, rc.right - rc.left, rc.bottom - rc.top);
                }
                hChild = GetWindow(hChild, GW_HWNDNEXT);
            }
        }

        setContentSize(newWidth, newHeight);
    }

    void zhuziScrollArea::setScrollOffset(int x, int y) {
        if (!m_hwnd || !m_contentFrame) return;

        RECT clientRect;
        GetClientRect(m_hwnd, &clientRect);
        int viewWidth = clientRect.right - clientRect.left;
        int viewHeight = clientRect.bottom - clientRect.top;

        int maxScrollX = (std::max)(0, m_contentWidth - viewWidth);
        int maxScrollY = (std::max)(0, m_contentHeight - viewHeight);

        int newX = (std::max)(0, (std::min)(x, maxScrollX));
        int newY = (std::max)(0, (std::min)(y, maxScrollY));

        if (newX == m_scrollX && newY == m_scrollY) return;

        m_scrollX = newX;
        m_scrollY = newY;
        updateContentPosition();

        SCROLLINFO si = { sizeof(SCROLLINFO), SIF_POS };
        si.nPos = m_scrollX;
        SetScrollInfo(m_hwnd, SB_HORZ, &si, TRUE);
        si.nPos = m_scrollY;
        SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
    }

    void zhuziScrollArea::setHorizontalScrollBarPolicy(ScrollBarPolicy policy) {
        m_hPolicy = policy;
        updateScrollBars();
    }

    void zhuziScrollArea::setVerticalScrollBarPolicy(ScrollBarPolicy policy) {
        m_vPolicy = policy;
        updateScrollBars();
    }

    void zhuziScrollArea::updateScrollBars() {
        if (!m_hwnd) return;

        RECT clientRect;
        GetClientRect(m_hwnd, &clientRect);
        int viewWidth = clientRect.right - clientRect.left;
        int viewHeight = clientRect.bottom - clientRect.top;

        bool needHorz = (m_hPolicy == AlwaysOn) ||
            (m_hPolicy == AsNeeded && m_contentWidth > viewWidth);
        bool needVert = (m_vPolicy == AlwaysOn) ||
            (m_vPolicy == AsNeeded && m_contentHeight > viewHeight);

        ShowScrollBar(m_hwnd, SB_HORZ, needHorz);
        ShowScrollBar(m_hwnd, SB_VERT, needVert);

        if (needHorz) {
            SCROLLINFO si = { sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE | SIF_POS };
            si.nMin = 0;
            si.nMax = m_contentWidth;
            si.nPage = viewWidth;
            si.nPos = m_scrollX;
            SetScrollInfo(m_hwnd, SB_HORZ, &si, TRUE);
        }
        if (needVert) {
            SCROLLINFO si = { sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE | SIF_POS };
            si.nMin = 0;
            si.nMax = m_contentHeight;
            si.nPage = viewHeight;
            si.nPos = m_scrollY;
            SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
        }
    }

    void zhuziScrollArea::updateContentPosition() {
        if (!m_contentFrame) return;
        m_contentFrame->setRect(-m_scrollX, -m_scrollY, m_contentWidth, m_contentHeight);
    }

    LRESULT CALLBACK zhuziScrollArea::ScrollAreaWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        zhuziScrollArea* pThis = (zhuziScrollArea*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (!pThis) return DefWindowProcW(hwnd, msg, wParam, lParam);
        return pThis->handleScrollMessage(msg, wParam, lParam);
    }

    LRESULT zhuziScrollArea::handleScrollMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_HSCROLL: {
            int newScrollX = m_scrollX;
            int code = LOWORD(wParam);
            SCROLLINFO si = { sizeof(SCROLLINFO), SIF_TRACKPOS | SIF_POS | SIF_RANGE | SIF_PAGE };
            GetScrollInfo(m_hwnd, SB_HORZ, &si);

            switch (code) {
            case SB_LINELEFT:      newScrollX -= 10; break;
            case SB_LINERIGHT:     newScrollX += 10; break;
            case SB_PAGELEFT:      newScrollX -= si.nPage; break;
            case SB_PAGERIGHT:     newScrollX += si.nPage; break;
            case SB_THUMBTRACK:    newScrollX = si.nTrackPos; break;
            case SB_THUMBPOSITION: newScrollX = HIWORD(wParam); break;
            default: break;
            }
            setScrollOffset(newScrollX, m_scrollY);
            return 0;
        }

        case WM_VSCROLL: {
            int newScrollY = m_scrollY;
            int code = LOWORD(wParam);
            SCROLLINFO si = { sizeof(SCROLLINFO), SIF_TRACKPOS | SIF_POS | SIF_RANGE | SIF_PAGE };
            GetScrollInfo(m_hwnd, SB_VERT, &si);

            switch (code) {
            case SB_LINEUP:        newScrollY -= 10; break;
            case SB_LINEDOWN:      newScrollY += 10; break;
            case SB_PAGEUP:        newScrollY -= si.nPage; break;
            case SB_PAGEDOWN:      newScrollY += si.nPage; break;
            case SB_THUMBTRACK:    newScrollY = si.nTrackPos; break;
            case SB_THUMBPOSITION: newScrollY = HIWORD(wParam); break;
            default: break;
            }
            setScrollOffset(m_scrollX, newScrollY);
            return 0;
        }

        case WM_SIZE: {
            updateScrollBars();
            updateContentPosition();
            return 0;
        }

        case WM_ERASEBKGND: {
            // 避免闪烁，可以选择自定义背景
            return 1;
        }
        }
        return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }

} // namespace zhuzi