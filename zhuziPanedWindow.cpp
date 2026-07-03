#include "zhuziPanedWindow.h"
#include "zhuziCommctrl.h"
#include "zhuziInstance.h"
#include <windowsx.h>

namespace zhuzi {

    const wchar_t* zhuziPanedWindow::WINDOW_CLASS_NAME = L"zhuziPanedWindowClass";
    bool zhuziPanedWindow::s_classRegistered = false;

    bool zhuziPanedWindow::RegisterWindowClass() {
        if (s_classRegistered) return true;
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = zhuziInstance::getHandle();
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = WINDOW_CLASS_NAME;
        s_classRegistered = (RegisterClassExW(&wc) != 0);
        return s_classRegistered;
    }

    zhuziPanedWindow::zhuziPanedWindow(zhuziControl* parent)
        : zhuziControl(parent)
        , m_orientation(Orientation::Horizontal)
        , m_sashSize(5)
        , m_minPaneSize(20)
        , m_dragSashIndex(-1)
        , m_dragStartPos(0)
        , m_originalSashPos(0) {
    }

    zhuziPanedWindow::~zhuziPanedWindow() {
        destroy();
    }

    bool zhuziPanedWindow::onCreate(DWORD style) {
        if (!RegisterWindowClass()) return false;
        if (!createControl(WINDOW_CLASS_NAME, 0, 0, 0, 0,
            style | WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN)) {
            return false;
        }
        SetWindowSubclass(m_hwnd, PanedWindowProc, 0, (DWORD_PTR)this);
        return true;
    }

    void zhuziPanedWindow::setOrientation(Orientation orient) {
        if (m_orientation == orient) return;
        m_orientation = orient;
        if (m_hwnd && !m_panes.empty()) {
            updateSashPositionsFromPanes();
            layoutPanes();
            InvalidateRect(m_hwnd, nullptr, TRUE);
        }
    }

    void zhuziPanedWindow::setSashSize(int size) {
        if (size < 2) size = 2;
        m_sashSize = size;
        if (m_hwnd && !m_panes.empty()) {
            layoutPanes();
            InvalidateRect(m_hwnd, nullptr, TRUE);
        }
    }

    void zhuziPanedWindow::addPane(zhuziControl* pane) {
        if (!pane) return;
        insertPane((int)m_panes.size(), pane);
    }

    void zhuziPanedWindow::insertPane(int index, zhuziControl* pane) {
        if (!pane) return;
        if (index < 0 || index >(int)m_panes.size()) index = (int)m_panes.size();

        if (pane->getParent() != this) {
            pane->setParent(this);
            if (pane->getHandle()) {
                SetParent(pane->getHandle(), m_hwnd);
            }
        }

        m_panes.insert(m_panes.begin() + index, pane);

        if (m_panes.size() >= 2) {
            if (m_panes.size() == 2) {
                m_sashPositions.resize(1);
                updateSashPositionsFromPanes();
            }
            else {
                m_sashPositions.resize(m_panes.size() - 1);
                updateSashPositionsFromPanes();
            }
        }
        else {
            m_sashPositions.clear();
        }

        if (m_hwnd) layoutPanes();
    }

    void zhuziPanedWindow::removePane(int index) {
        if (index < 0 || index >= (int)m_panes.size()) return;
        m_panes.erase(m_panes.begin() + index);
        if (m_panes.size() <= 1) {
            m_sashPositions.clear();
        }
        else {
            m_sashPositions.resize(m_panes.size() - 1);
            updateSashPositionsFromPanes();
        }
        if (m_hwnd) layoutPanes();
    }

    void zhuziPanedWindow::clearPanes() {
        m_panes.clear();
        m_sashPositions.clear();
        if (m_hwnd) layoutPanes();
    }

    zhuziControl* zhuziPanedWindow::getPane(int index) const {
        if (index >= 0 && index < (int)m_panes.size()) return m_panes[index];
        return nullptr;
    }

    int zhuziPanedWindow::getSashPosition(int sashIndex) const {
        if (sashIndex < 0 || sashIndex >= (int)m_sashPositions.size()) return -1;
        return m_sashPositions[sashIndex];
    }

    void zhuziPanedWindow::setSashPosition(int sashIndex, int pos, bool redraw) {
        if (sashIndex < 0 || sashIndex >= (int)m_sashPositions.size()) return;
        int minPos, maxPos;
        getSashValidRange(sashIndex, minPos, maxPos);
        if (pos < minPos) pos = minPos;
        if (pos > maxPos) pos = maxPos;
        m_sashPositions[sashIndex] = pos;
        if (redraw && m_hwnd) {
            layoutPanes();
            invalidateSash(sashIndex);
            UpdateWindow(m_hwnd);
        }
    }

    void zhuziPanedWindow::setMinPaneSize(int minSize) {
        if (minSize < 5) minSize = 5;
        m_minPaneSize = minSize;
        if (m_hwnd && !m_panes.empty()) {
            for (size_t i = 0; i < m_sashPositions.size(); ++i) {
                int minPos, maxPos;
                getSashValidRange((int)i, minPos, maxPos);
                if (m_sashPositions[i] < minPos) m_sashPositions[i] = minPos;
                if (m_sashPositions[i] > maxPos) m_sashPositions[i] = maxPos;
            }
            layoutPanes();
            InvalidateRect(m_hwnd, nullptr, TRUE);
        }
    }

    void zhuziPanedWindow::layoutPanes() {
        if (!m_hwnd) return;
        if (m_panes.empty()) return;

        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);
        int totalLen = (m_orientation == Orientation::Horizontal) ? (rcClient.right - rcClient.left)
            : (rcClient.bottom - rcClient.top);
        if (totalLen <= 0) return;

        int paneCount = (int)m_panes.size();
        if (paneCount != (int)m_sashPositions.size() + 1) {
            updateSashPositionsFromPanes();
        }

        int start = 0;
        for (int i = 0; i < paneCount; ++i) {
            int end = totalLen;
            if (i < paneCount - 1) {
                end = m_sashPositions[i];
            }
            int paneLen = end - start;
            if (paneLen < 0) paneLen = 0;

            RECT rcPane = rcClient;
            if (m_orientation == Orientation::Horizontal) {
                rcPane.left = start;
                rcPane.right = start + paneLen;
            }
            else {
                rcPane.top = start;
                rcPane.bottom = start + paneLen;
            }

            zhuziControl* pane = m_panes[i];
            if (pane && pane->getHandle()) {
                // 更新子控件的绝对布局参数
                pane->setGeometry((int)rcPane.left, rcPane.top,
                    rcPane.right - rcPane.left, rcPane.bottom - rcPane.top);
                // 通知子控件父窗口尺寸变化（触发其内部布局）
                pane->onParentResize(rcPane.right - rcPane.left, rcPane.bottom - rcPane.top);
                // 强制重绘子控件，确保文本对齐等样式更新（关键修复）
                InvalidateRect(pane->getHandle(), nullptr, TRUE);
                ShowWindow(pane->getHandle(), SW_SHOW);
            }

            if (i < paneCount - 1) {
                start = end + m_sashSize;
            }
        }
    }

    void zhuziPanedWindow::onParentResize(int parentWidth, int parentHeight) {
        zhuziControl::onParentResize(parentWidth, parentHeight);
        layoutPanes();
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void zhuziPanedWindow::updateSashPositionsFromPanes() {
        if (!m_hwnd || m_panes.empty()) return;
        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);
        int totalLen = (m_orientation == Orientation::Horizontal) ? (rcClient.right - rcClient.left)
            : (rcClient.bottom - rcClient.top);
        if (totalLen <= 0) totalLen = 100;

        int paneCount = (int)m_panes.size();
        m_sashPositions.resize(paneCount - 1);
        if (paneCount <= 1) return;

        int totalSashWidth = (paneCount - 1) * m_sashSize;
        int totalPaneLen = totalLen - totalSashWidth;
        if (totalPaneLen < paneCount * m_minPaneSize) totalPaneLen = paneCount * m_minPaneSize;

        int avgLen = totalPaneLen / paneCount;
        int remainder = totalPaneLen % paneCount;

        int pos = 0;
        for (int i = 0; i < paneCount - 1; ++i) {
            int paneLen = avgLen + (i < remainder ? 1 : 0);
            pos += paneLen;
            m_sashPositions[i] = pos;
            pos += m_sashSize;
        }
    }

    void zhuziPanedWindow::getSashValidRange(int sashIndex, int& minPos, int& maxPos) const {
        minPos = m_minPaneSize;
        maxPos = 0;

        if (!m_hwnd || m_panes.empty()) return;
        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);
        int totalLen = (m_orientation == Orientation::Horizontal) ? (rcClient.right - rcClient.left)
            : (rcClient.bottom - rcClient.top);
        if (totalLen <= 0) return;

        int paneCount = (int)m_panes.size();
        int leftMin = m_minPaneSize * (sashIndex + 1);
        int rightMin = m_minPaneSize * (paneCount - sashIndex - 1);

        int totalSashWidth = (paneCount - 1) * m_sashSize;
        int maxAvailable = totalLen - totalSashWidth - rightMin;
        minPos = leftMin;
        maxPos = maxAvailable;
        if (maxPos < minPos) maxPos = minPos;
    }

    int zhuziPanedWindow::getPosFromMouse(int x, int y) const {
        return (m_orientation == Orientation::Horizontal) ? x : y;
    }

    void zhuziPanedWindow::drawSashes(HDC hdc, const RECT& clientRect) {
        if (m_panes.size() < 2) return;

        for (size_t i = 0; i < m_sashPositions.size(); ++i) {
            int sashPos = m_sashPositions[i];
            RECT rcSash = clientRect;
            if (m_orientation == Orientation::Horizontal) {
                rcSash.left = sashPos;
                rcSash.right = sashPos + m_sashSize;
                if (rcSash.right > clientRect.right) rcSash.right = clientRect.right;
            }
            else {
                rcSash.top = sashPos;
                rcSash.bottom = sashPos + m_sashSize;
                if (rcSash.bottom > clientRect.bottom) rcSash.bottom = clientRect.bottom;
            }

            HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            FillRect(hdc, &rcSash, hBrush);
            DeleteObject(hBrush);
            DrawEdge(hdc, &rcSash, EDGE_RAISED, BF_RECT);
        }
    }

    void zhuziPanedWindow::invalidateSash(int sashIndex) {
        if (!m_hwnd || sashIndex < 0 || sashIndex >= (int)m_sashPositions.size()) return;
        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);
        RECT rcSash = rcClient;
        int sashPos = m_sashPositions[sashIndex];
        if (m_orientation == Orientation::Horizontal) {
            rcSash.left = sashPos;
            rcSash.right = sashPos + m_sashSize;
        }
        else {
            rcSash.top = sashPos;
            rcSash.bottom = sashPos + m_sashSize;
        }
        InvalidateRect(m_hwnd, &rcSash, FALSE);
    }

    LRESULT CALLBACK zhuziPanedWindow::PanedWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        zhuziPanedWindow* pThis = (zhuziPanedWindow*)dwRefData;
        if (!pThis) return DefSubclassProc(hwnd, msg, wParam, lParam);
       
        switch (msg) {
        case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT) {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hwnd, &pt);
                int pos = pThis->getPosFromMouse(pt.x, pt.y);
                for (size_t i = 0; i < pThis->m_sashPositions.size(); ++i) {
                    int sashPos = pThis->m_sashPositions[i];
                    if (pos >= sashPos && pos <= sashPos + pThis->m_sashSize) {
                        LPCWSTR cursor = (pThis->m_orientation == zhuziPanedWindow::Orientation::Horizontal)
                            ? IDC_SIZEWE : IDC_SIZENS;
                        SetCursor(LoadCursor(nullptr, cursor));
                        return TRUE;
                    }
                }
            }
            break;
        }

        case WM_MOUSEMOVE: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            if (pThis->m_dragSashIndex != -1) {
                LPCWSTR cursor = (pThis->m_orientation == zhuziPanedWindow::Orientation::Horizontal)
                    ? IDC_SIZEWE : IDC_SIZENS;
                SetCursor(LoadCursor(nullptr, cursor));

                int newPos = pThis->getPosFromMouse(x, y);
                int delta = newPos - pThis->m_dragStartPos;
                int newSashPos = pThis->m_originalSashPos + delta;
                int minPos, maxPos;
                pThis->getSashValidRange(pThis->m_dragSashIndex, minPos, maxPos);
                if (newSashPos < minPos) newSashPos = minPos;
                if (newSashPos > maxPos) newSashPos = maxPos;
                if (newSashPos != pThis->m_sashPositions[pThis->m_dragSashIndex]) {
                    pThis->setSashPosition(pThis->m_dragSashIndex, newSashPos, true);
                    pThis->m_originalSashPos = newSashPos;
                    pThis->m_dragStartPos = newPos;
                }
                return 0;
            }
            break;
        }

        case WM_LBUTTONDOWN: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            int pos = pThis->getPosFromMouse(x, y);
            for (size_t i = 0; i < pThis->m_sashPositions.size(); ++i) {
                int sashPos = pThis->m_sashPositions[i];
                if (pos >= sashPos && pos <= sashPos + pThis->m_sashSize) {
                    pThis->m_dragSashIndex = (int)i;
                    pThis->m_dragStartPos = pos;
                    pThis->m_originalSashPos = sashPos;
                    SetCapture(hwnd);
                    return 0;
                }
            }
            break;
        }

        case WM_LBUTTONUP: {
            if (pThis->m_dragSashIndex != -1) {
                pThis->m_dragSashIndex = -1;
                ReleaseCapture();
                return 0;
            }
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (hdc) {
                RECT rcClient;
                GetClientRect(hwnd, &rcClient);
                HBRUSH hBg = (HBRUSH)GetClassLongPtrW(hwnd, GCLP_HBRBACKGROUND);
                if (!hBg) hBg = GetSysColorBrush(COLOR_WINDOW);
                FillRect(hdc, &rcClient, hBg);
                pThis->drawSashes(hdc, rcClient);
                EndPaint(hwnd, &ps);
            }
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_SIZE: {
            pThis->layoutPanes();
            InvalidateRect(hwnd, nullptr, FALSE);
            break;
        }
        }

        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

} // namespace zhuzi