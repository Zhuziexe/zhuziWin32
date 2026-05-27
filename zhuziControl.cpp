#include "zhuziControl.h"
#include "zhuziInstance.h"
#include <windowsx.h>
#include <cmath>

namespace zhuzi {
    void PremultiplyAlphaBits(void* bits, int width, int height) {
        // 假设 32 位 BGRA 格式（Windows DIB 默认）
        BYTE* p = (BYTE*)bits;
        int stride = width * 4;
        for (int y = 0; y < height; ++y) {
            BYTE* row = p + y * stride;
            for (int x = 0; x < width; ++x) {
                BYTE* pixel = row + x * 4;
                BYTE a = pixel[3];
                if (a == 0) {
                    pixel[0] = pixel[1] = pixel[2] = 0;
                }
                else if (a != 255) {
                    pixel[0] = (BYTE)((pixel[0] * a) / 255);
                    pixel[1] = (BYTE)((pixel[1] * a) / 255);
                    pixel[2] = (BYTE)((pixel[2] * a) / 255);
                }
                // a == 255 时 RGB 不变
            }
        }
    }

    std::bitset<1000> zhuziControl::s_idUsed;

    int zhuziControl::allocateId() {
        for (size_t i = 0; i < 1000; ++i) {
            if (!s_idUsed[i]) {
                s_idUsed.set(i);
                return 2000 + static_cast<int>(i);
            }
        }
        throw std::out_of_range("No available control ID in range 2000-2999");
    }

    void zhuziControl::releaseId(int id) {
        if (id >= 2000 && id <= 2999) {
            size_t idx = static_cast<size_t>(id - 2000);
            if (s_idUsed[idx]) s_idUsed.reset(idx);
        }
    }

    zhuziControl::zhuziControl(zhuziControl* parent)
        : m_layoutType(LayoutType::None), m_hwnd(nullptr), m_id(-1),
        m_parent(parent) {
        m_layoutParam[0] = m_layoutParam[1] = m_layoutParam[2] = m_layoutParam[3] = 0;
    }

    zhuziControl::~zhuziControl() { destroy(); }

    void zhuziControl::initCreate(LayoutType type) {
        if (m_hwnd) throw std::runtime_error("Control already created");
        m_layoutType = type;
    }

    bool zhuziControl::create(int x, int y, int w, int h, DWORD style) {
        initCreate(LayoutType::Absolute);
        m_layoutParam[0] = x; m_layoutParam[1] = y;
        m_layoutParam[2] = w; m_layoutParam[3] = h;
        return onCreate(style);
    }

    bool zhuziControl::create(double xp, double yp, double wp, double hp, DWORD style) {
        initCreate(LayoutType::Percent);
        m_layoutParam[0] = static_cast<int>(xp * 10000);
        m_layoutParam[1] = static_cast<int>(yp * 10000);
        m_layoutParam[2] = static_cast<int>(wp * 10000);
        m_layoutParam[3] = static_cast<int>(hp * 10000);
        return onCreate(style);
    }

    bool zhuziControl::create(UINT left, UINT top, UINT right, UINT bottom, DWORD style) {
        initCreate(LayoutType::Anchor);
        m_layoutParam[0] = left; m_layoutParam[1] = top;
        m_layoutParam[2] = right; m_layoutParam[3] = bottom;
        return onCreate(style);
    }

    void zhuziControl::setAnchor(UINT left, UINT top, UINT right, UINT bottom) {
        if (m_hwnd) return;
        m_layoutType = LayoutType::Anchor;
        m_layoutParam[0] = left;
        m_layoutParam[1] = top;
        m_layoutParam[2] = right;
        m_layoutParam[3] = bottom;
    }

    bool zhuziControl::createControl(const wchar_t* className, int x, int y, int w, int h, DWORD style, DWORD exStyle, bool doSubClass) {
        if (m_hwnd) return false;
        if (m_id == -1) {
            try { m_id = allocateId(); }
            catch (...) { return false; }
        }
        HWND hParent = m_parent ? m_parent->getHandle() : nullptr;
        m_hwnd = CreateWindowExW(exStyle, className, L"", style, x, y, w, h, hParent,
            (HMENU)(INT_PTR)m_id, zhuziInstance::getHandle(), nullptr);
        if (!m_hwnd) {
            releaseId(m_id); m_id = -1;
            return false;
        }
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
        if(doSubClass) SetWindowSubclass(m_hwnd, ControlWndProc, 0, (DWORD_PTR)this);
        if (m_parent) {
            RECT rc; GetClientRect(m_parent->getHandle(), &rc);
            applyLayout(rc.right - rc.left, rc.bottom - rc.top);
        }
        return true;
    }

    void zhuziControl::applyLayout(int parentWidth, int parentHeight) {
        if (!m_hwnd) return;
        int x, y, w, h;
        switch (m_layoutType) {
        case LayoutType::Absolute:
            x = (m_layoutParam[0] >= 0) ? m_layoutParam[0] : parentWidth + m_layoutParam[0];
            y = (m_layoutParam[1] >= 0) ? m_layoutParam[1] : parentHeight + m_layoutParam[1];
            w = m_layoutParam[2];
            h = m_layoutParam[3];
            break;
        case LayoutType::Percent:
            x = static_cast<int>(parentWidth * (m_layoutParam[0] / 10000.0));
            y = static_cast<int>(parentHeight * (m_layoutParam[1] / 10000.0));
            w = static_cast<int>(parentWidth * (m_layoutParam[2] / 10000.0));
            h = static_cast<int>(parentHeight * (m_layoutParam[3] / 10000.0));
            break;
        case LayoutType::Anchor:
            x = m_layoutParam[0];
            y = m_layoutParam[1];
            w = parentWidth - m_layoutParam[0] - m_layoutParam[2];
            h = parentHeight - m_layoutParam[1] - m_layoutParam[3];
            break;
        default: return;
        }
        SetWindowPos(m_hwnd, nullptr, x, y, w, h, SWP_NOZORDER);
    }

    void zhuziControl::onParentResize(int parentWidth, int parentHeight) {
        applyLayout(parentWidth, parentHeight);
    }

    void zhuziControl::destroy() {
        if (m_hwnd) {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
            if (m_id >= 2000 && m_id <= 2999) releaseId(m_id);
            m_id = -1;
        }
    }

    void zhuziControl::setText(const zhuziString& text) {
        if (m_hwnd) SetWindowTextW(m_hwnd, text.c_str());
    }

    zhuziString zhuziControl::getText() const {
        if (!m_hwnd) return L"";
        int len = GetWindowTextLengthW(m_hwnd);
        zhuziString s(len, L'\0');
        GetWindowTextW(m_hwnd, &s[0], len + 1);
        return s;
    }

    void zhuziControl::setRect(int x, int y, int w, int h) {
        if (m_hwnd) SetWindowPos(m_hwnd, nullptr, x, y, w, h, SWP_NOZORDER);
    }

    void zhuziControl::show(bool visible) {
        if (m_hwnd) ShowWindow(m_hwnd, visible ? SW_SHOW : SW_HIDE);
    }

    void zhuziControl::enable(bool enabled) {
        if (m_hwnd) EnableWindow(m_hwnd, enabled);
    }

    void zhuziControl::setFont(const zhuziFont& font) {
        font.applyTo(m_hwnd);
    }

    void zhuziControl::setGeometry(int x, int y, int w, int h) {
        m_layoutType = LayoutType::Absolute;
        m_layoutParam[0] = x; m_layoutParam[1] = y;
        m_layoutParam[2] = w; m_layoutParam[3] = h;
        if (m_parent) {
            RECT rc; GetClientRect(m_parent->getHandle(), &rc);
            applyLayout(rc.right - rc.left, rc.bottom - rc.top);
        }
    }

    void zhuziControl::setGeometry(double xp, double yp, double wp, double hp) {
        m_layoutType = LayoutType::Percent;
        m_layoutParam[0] = static_cast<int>(xp * 10000);
        m_layoutParam[1] = static_cast<int>(yp * 10000);
        m_layoutParam[2] = static_cast<int>(wp * 10000);
        m_layoutParam[3] = static_cast<int>(hp * 10000);
        if (m_parent) {
            RECT rc; GetClientRect(m_parent->getHandle(), &rc);
            applyLayout(rc.right - rc.left, rc.bottom - rc.top);
        }
    }

    void zhuziControl::setLayoutParamOnly(int x, int y, int w, int h) {
        m_layoutType = LayoutType::Absolute;
        m_layoutParam[0] = x;
        m_layoutParam[1] = y;
        m_layoutParam[2] = w;
        m_layoutParam[3] = h;
    }

    LRESULT zhuziControl::SendWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam) const {
        return SendMessageW(m_hwnd, msg, wParam, lParam);
    }

    LRESULT CALLBACK zhuziControl::ControlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        zhuziControl* pThis = (zhuziControl*)dwRefData;
        if (!pThis) return DefSubclassProc(hwnd, msg, wParam, lParam);

        if (msg == WM_KEYDOWN) {
            SendMessage(GetParent(hwnd), msg, wParam, lParam);
        }

        // 如果不是自定义绘制控件，直接放行所有消息
        if (!pThis->m_isCustomDraw) {
            return DefSubclassProc(hwnd, msg, wParam, lParam);
        }

        // 以下仅对自定义控件生效
        switch (msg) {
        case WM_ERASEBKGND:
            return 1;  // 阻止擦除背景，减少闪烁

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (hdc) {
                RECT rcClient;
                GetClientRect(hwnd, &rcClient);
                int width = rcClient.right - rcClient.left;
                int height = rcClient.bottom - rcClient.top;
                if (width > 0 && height > 0) {
                    // 根据是否需要透明背景选择绘制方式
                    if (pThis->getTransparent()) {
                        HDC memDC = CreateCompatibleDC(hdc);
                        BITMAPINFO bmi = { 0 };
                        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                        bmi.bmiHeader.biWidth = width;
                        bmi.bmiHeader.biHeight = -height;
                        bmi.bmiHeader.biPlanes = 1;
                        bmi.bmiHeader.biBitCount = 32;
                        bmi.bmiHeader.biCompression = BI_RGB;
                        VOID* pvBits = nullptr;
                        HBITMAP memBitmap = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &pvBits, nullptr, 0);
                        if (memBitmap && pvBits) {
                            HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
                            memset(pvBits, 0, width * height * 4);   // 全透明
                            zhuziPaint paint(memDC, rcClient);
                            pThis->onPaint(paint);
                            // ========= 新增预乘 =========
                            PremultiplyAlphaBits(pvBits, width, height);
                            // ===========================
                            BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
                            AlphaBlend(hdc, 0, 0, width, height, memDC, 0, 0, width, height, blend);
                            SelectObject(memDC, oldBitmap);
                            DeleteObject(memBitmap);
                        }
                        DeleteDC(memDC);
                    }
                    else {
                        // ===== 普通双缓冲方式：兼容原行为 =====
                        HDC memDC = CreateCompatibleDC(hdc);
                        HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
                        HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
                        zhuziPaint paint(memDC, rcClient);
                        pThis->onPaint(paint);
                        BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
                        SelectObject(memDC, oldBitmap);
                        DeleteObject(memBitmap);
                        DeleteDC(memDC);
                    }
                }
                EndPaint(hwnd, &ps);
            }
            return 0;
        }

        case WM_LBUTTONUP:
            pThis->onLButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_RBUTTONUP:
            pThis->onRButtonUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_MOUSEMOVE: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            pThis->onMouseMove(x, y);
            return 0;
        }
        case WM_MOUSELEAVE:
            pThis->onMouseLeave();
            return 0;
        }
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    void zhuziControl::setFocus() {
        if (m_hwnd && IsWindow(m_hwnd)) {
            ::SetFocus(m_hwnd);
        }
        
    }

    // ==================== zhuziWindow ====================
    const wchar_t* zhuziWindow::WINDOW_CLASS_NAME = L"zhuziWindowClass";
    bool zhuziWindow::s_classRegistered = false;

    zhuziWindow::zhuziWindow(zhuziWindow* parent)
        : zhuziControl(parent), m_hBgBrush(nullptr) {
        m_flag1 = (parent == nullptr);
        if (m_flag1) zhuziInstance::registerTopLevelWindow();
    }

    zhuziWindow::~zhuziWindow() {
        if (m_hBgBrush) DeleteObject(m_hBgBrush);
        destroy();
    }

    void zhuziWindow::setBgColor(COLORREF color) {
        if (m_hBgBrush) DeleteObject(m_hBgBrush);
        m_hBgBrush = CreateSolidBrush(color);
        if (m_hwnd) {
            // 触发重绘
            InvalidateRect(m_hwnd, nullptr, TRUE);
        }
    }

    bool zhuziWindow::RegisterWindowClass() {
        if (s_classRegistered) return true;
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = zhuziInstance::getHandle();
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = WINDOW_CLASS_NAME;
        s_classRegistered = (RegisterClassExW(&wc) != 0);
        return s_classRegistered;
    }

    bool zhuziWindow::create(const zhuziString& title, int x, int y, int w, int h, DWORD style) {
        m_windowTitle = title;
        return zhuziControl::create(x, y, w, h, style);
    }

    bool zhuziWindow::onCreate(DWORD style) {
        if (!RegisterWindowClass()) return false;
        HWND hParent = m_parent ? m_parent->getHandle() : nullptr;
        int x = m_layoutParam[0];
        int y = m_layoutParam[1];
        int width = m_layoutParam[2];
        int height = m_layoutParam[3];
        m_hwnd = CreateWindowExW(0, WINDOW_CLASS_NAME, m_windowTitle.c_str(), style,
            x, y, width, height, hParent, nullptr, zhuziInstance::getHandle(), this);
        if (!m_hwnd) return false;
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
        UpdateWindow(m_hwnd);
        return true;
    }

    void zhuziWindow::destroy() {
        if (m_hwnd && IsWindow(m_hwnd)) DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
        if (m_flag1) zhuziInstance::unregisterTopLevelWindow();
    }

    void zhuziWindow::show(int cmdShow) {
        if (m_hwnd) ShowWindow(m_hwnd, cmdShow);
    }

    void zhuziWindow::update() {
        if (m_hwnd) UpdateWindow(m_hwnd);
    }

    void zhuziWindow::Bind(UINT uMsg, std::function<void(WPARAM, LPARAM)> callback) {
        m_msgHandlers[uMsg] = callback;
    }

    void zhuziWindow::Bind(UINT uMsg, WPARAM wParam, std::function<void(LPARAM)> callback) {
        m_msgWParamHandlers[{uMsg, wParam}] = callback;
    }

    int zhuziWindow::BindChain(UINT uMsg, std::function<bool(WPARAM, LPARAM)> callback) {
        m_msgChainHandlers[uMsg].push_back(callback);
        return static_cast<int>(m_msgChainHandlers[uMsg].size()) - 1;
    }

    void zhuziWindow::Unbind(UINT uMsg) {
        m_msgHandlers.erase(uMsg);
    }

    void zhuziWindow::Unbind(UINT uMsg, WPARAM wParam) {
        m_msgWParamHandlers.erase({ uMsg, wParam });
    }

    int zhuziWindow::getNextControlId() {
        static int counter = 0;
        return 1000 + counter++;
    }

    void zhuziWindow::setIcon(const zhuziString& filename) {
        if (!m_hwnd) return;
        HICON hIcon = (HICON)LoadImageW(nullptr, filename.c_str(), IMAGE_ICON, 32, 32, LR_LOADFROMFILE | LR_DEFAULTSIZE);
        if (hIcon) SendMessageW(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        HICON hIconSmall = (HICON)LoadImageW(nullptr, filename.c_str(), IMAGE_ICON, 16, 16, LR_LOADFROMFILE | LR_DEFAULTSIZE);
        if (hIconSmall) SendMessageW(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
    }

    void zhuziWindow::setIcon(int resourceId) {
        if (!m_hwnd) return;
        HICON hIcon = LoadIconW(zhuziInstance::getHandle(), MAKEINTRESOURCEW(resourceId));
        if (hIcon) {
            SendMessageW(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            SendMessageW(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        }
    }

    void zhuziWindow::onParentResize(int parentWidth, int parentHeight) {
        if (m_hwnd) {
            HWND child = GetWindow(m_hwnd, GW_CHILD);
            while (child) {
                zhuziControl* ctrl = (zhuziControl*)GetWindowLongPtrW(child, GWLP_USERDATA);
                if (ctrl) ctrl->onParentResize(parentWidth, parentHeight);
                child = GetWindow(child, GW_HWNDNEXT);
            }
        }
    }

    LRESULT CALLBACK zhuziWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        zhuziWindow* pThis = nullptr;
        if (msg == WM_CTLCOLOREDIT) {
            HDC hdc = (HDC)wParam;
            SetBkColor(hdc, RGB(255, 255, 255));
            SetTextColor(hdc, RGB(0, 0, 0));
            return (LRESULT)GetStockObject(WHITE_BRUSH);
        }
        if (msg == WM_NCCREATE) {
            CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
            pThis = (zhuziWindow*)pCreate->lpCreateParams;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        }
        else {
            pThis = (zhuziWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        }
        if (pThis) {
            LRESULT result = pThis->handleMessage(msg, wParam, lParam);
            if (result != -1) return result;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    LRESULT zhuziWindow::handleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_ERASEBKGND) {
            if (m_hBgBrush) {
                HDC hdc = (HDC)wParam;
                RECT rc;
                GetClientRect(m_hwnd, &rc);
                FillRect(hdc, &rc, m_hBgBrush);
                return 1;  // 表示已处理背景擦除
            }
            // 没有自定义背景，调用默认处理（返回 -1 让系统处理）
            return -1;
        }

        if (msg == WM_GETMINMAXINFO) {
            MINMAXINFO* pMMI = (MINMAXINFO*)lParam;
            if (m_minWidth > 0) pMMI->ptMinTrackSize.x = m_minWidth;
            if (m_minHeight > 0) pMMI->ptMinTrackSize.y = m_minHeight;
            if (m_maxWidth > 0) pMMI->ptMaxTrackSize.x = m_maxWidth;
            if (m_maxHeight > 0) pMMI->ptMaxTrackSize.y = m_maxHeight;
            return 0;
        }

        auto itChain = m_msgChainHandlers.find(msg);
        if (itChain != m_msgChainHandlers.end()) {
            for (auto& cb : itChain->second) {
                if (cb(wParam, lParam)) return 0;
            }
        }
        auto itExact = m_msgWParamHandlers.find({ msg, wParam });
        if (itExact != m_msgWParamHandlers.end()) {
            itExact->second(lParam);
            return 0;
        }
        auto itGeneral = m_msgHandlers.find(msg);
        if (itGeneral != m_msgHandlers.end()) {
            itGeneral->second(wParam, lParam);
            return 0;
        }
        if (msg == WM_CTLCOLORSTATIC) {
            HDC hdc = (HDC)wParam;
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }
        if (msg == WM_SIZE) {
            int width = LOWORD(lParam);
            int height = HIWORD(lParam);
            onParentResize(width, height);
            return 0;
        }
        if (msg == WM_NCDESTROY) {
            if (m_flag1) zhuziInstance::unregisterTopLevelWindow();
            m_hwnd = nullptr;
            return 0;
        }
        return -1;

    }

    zhuziFont zhuziControl::getFont() const {
        if (!m_hwnd) return zhuziFont(L"Microsoft YaHei", 16);

        // 获取当前字体句柄
        HFONT hFont = (HFONT)SendMessage(m_hwnd, WM_GETFONT, 0, 0);
        if (!hFont) {
            // 未设置字体，返回默认字体
            return zhuziFont(L"Microsoft YaHei", 16);
        }

        LOGFONTW lf = { 0 };
        if (GetObjectW(hFont, sizeof(LOGFONTW), &lf) == 0) {
            return zhuziFont(L"Microsoft YaHei", 16);
        }

        // 将 lfHeight（逻辑单位）转换为点大小
        HDC screenDC = GetDC(nullptr);
        int dpiY = GetDeviceCaps(screenDC, LOGPIXELSY);
        ReleaseDC(nullptr, screenDC);
        int pointSize = MulDiv(-lf.lfHeight, 72, dpiY);
        if (pointSize <= 0) pointSize = 16; // 安全回退

        bool bold = (lf.lfWeight >= FW_BOLD);
        bool italic = (lf.lfItalic != 0);
        bool underline = (lf.lfUnderline != 0);

        return zhuziFont(lf.lfFaceName, pointSize, bold, italic, underline);
    }

} // namespace zhuzi