#include "zhuziControl.h"
#include "zhuziInstance.h"
#include <windowsx.h>
#include <cmath>
#include "zhuziCommctrl.h"
#include <unordered_map>
#include <vector>
#include <algorithm>

#ifndef RGBA
#define RGBA(r,g,b,a) ((COLORREF)(((a)&0xFF)<<24) | ((r)&0xFF) | (((g)&0xFF)<<8) | (((b)&0xFF)<<16))
#endif

namespace zhuzi {

    // ========== 全局绑定存储（分开存储）==========
    struct Handler {
        int id;
        UINT msg;
        std::function<void(zhuziMsg*)> callback;
    };
    static std::unordered_map<HWND, std::vector<Handler>> s_handlerMap;

    struct WParamHandler {
        int id;
        UINT msg;
        WPARAM wParam;
        std::function<void(zhuziMsg*)> callback;
    };
    static std::unordered_map<HWND, std::vector<WParamHandler>> s_wParamHandlerMap;

    static std::vector<Handler*> s_idToHandler;
    static std::vector<WParamHandler*> s_idToWParamHandler;
    static int s_nextHandlerId = 1;

    // ---------- Bind 实现 ----------
    int Bind(zhuziControl* pCtrl, UINT uMsg, std::function<void(zhuziMsg*)> callback) {
        if (!pCtrl || !pCtrl->getHandle()) return -1;
        HWND hwnd = pCtrl->getHandle();
        int id = s_nextHandlerId++;
        if ((int)s_idToHandler.size() <= id) s_idToHandler.resize(id + 1, nullptr);
        Handler handler{ id, uMsg, callback };
        s_handlerMap[hwnd].push_back(handler);
        s_idToHandler[id] = &s_handlerMap[hwnd].back();
        return id;
    }

    int Bind(zhuziControl* pCtrl, UINT uMsg, WPARAM wParam, std::function<void(zhuziMsg*)> callback) {
        if (!pCtrl || !pCtrl->getHandle()) return -1;
        HWND hwnd = pCtrl->getHandle();
        int id = s_nextHandlerId++;
        if ((int)s_idToWParamHandler.size() <= id) s_idToWParamHandler.resize(id + 1, nullptr);
        WParamHandler handler{ id, uMsg, wParam, callback };
        s_wParamHandlerMap[hwnd].push_back(handler);
        s_idToWParamHandler[id] = &s_wParamHandlerMap[hwnd].back();
        return id;
    }

    int Bind(HWND hwnd, UINT uMsg, std::function<void(zhuziMsg*)> callback) {
        if (!hwnd) return -1;
        int id = s_nextHandlerId++;
        if ((int)s_idToHandler.size() <= id) s_idToHandler.resize(id + 1, nullptr);
        Handler handler{ id, uMsg, callback };
        s_handlerMap[hwnd].push_back(handler);
        s_idToHandler[id] = &s_handlerMap[hwnd].back();
        return id;
    }

    // ---------- Unbind ----------
    void Unbind(int handlerId) {
        if (handlerId <= 0 || handlerId >= s_nextHandlerId) return;
        if (handlerId < (int)s_idToHandler.size() && s_idToHandler[handlerId]) {
            for (auto& pair : s_handlerMap) {
                auto& vec = pair.second;
                auto it = std::find_if(vec.begin(), vec.end(), [handlerId](const Handler& h) { return h.id == handlerId; });
                if (it != vec.end()) {
                    vec.erase(it);
                    if (vec.empty()) s_handlerMap.erase(pair.first);
                    break;
                }
            }
            s_idToHandler[handlerId] = nullptr;
        }
        else if (handlerId < (int)s_idToWParamHandler.size() && s_idToWParamHandler[handlerId]) {
            for (auto& pair : s_wParamHandlerMap) {
                auto& vec = pair.second;
                auto it = std::find_if(vec.begin(), vec.end(), [handlerId](const WParamHandler& h) { return h.id == handlerId; });
                if (it != vec.end()) {
                    vec.erase(it);
                    if (vec.empty()) s_wParamHandlerMap.erase(pair.first);
                    break;
                }
            }
            s_idToWParamHandler[handlerId] = nullptr;
        }
    }

    void Unbind(zhuziControl* pCtrl, UINT uMsg) {
        if (!pCtrl) return;
        HWND hwnd = pCtrl->getHandle();
        auto it = s_handlerMap.find(hwnd);
        if (it != s_handlerMap.end()) {
            auto& vec = it->second;
            for (auto& h : vec) if (h.id >= 0 && h.id < (int)s_idToHandler.size()) s_idToHandler[h.id] = nullptr;
            vec.erase(std::remove_if(vec.begin(), vec.end(), [uMsg](const Handler& h) { return h.msg == uMsg; }), vec.end());
            if (vec.empty()) s_handlerMap.erase(hwnd);
        }
        auto it2 = s_wParamHandlerMap.find(hwnd);
        if (it2 != s_wParamHandlerMap.end()) {
            auto& vec = it2->second;
            for (auto& h : vec) if (h.id >= 0 && h.id < (int)s_idToWParamHandler.size()) s_idToWParamHandler[h.id] = nullptr;
            vec.erase(std::remove_if(vec.begin(), vec.end(), [uMsg](const WParamHandler& h) { return h.msg == uMsg; }), vec.end());
            if (vec.empty()) s_wParamHandlerMap.erase(hwnd);
        }
    }

    bool DispatchMessageToControl(HWND hwnd, zhuziMsg& msg) {
        if (!hwnd) return false;

        // 1. 检查带 wParam 的精确处理器
        auto itW = s_wParamHandlerMap.find(hwnd);
        if (itW != s_wParamHandlerMap.end()) {
            for (auto& handler : itW->second) {
                if (handler.msg == msg.msg && handler.wParam == msg.wParam) {
                    handler.callback(&msg);
                    if (msg.handled) { return true; OutputDebugString(L"LOL\n"); }
                }
            }
        }

        // 2. 检查普通处理器
        auto it = s_handlerMap.find(hwnd);
        if (it != s_handlerMap.end()) {
            for (auto& handler : it->second) {
                if (handler.msg == msg.msg) {
                    handler.callback(&msg);
                    if (msg.handled) { return true; OutputDebugString(L"LOL\n"); }
                }
            }
        }
        /*       // 3. 向上冒泡到父窗口
        HWND hParent = GetParent(hwnd);
        if (hParent && IsWindow(hParent)) {
            // 避免无限递归（父窗口不能是自己）
            if (hParent != hwnd) {
                return DispatchMessageToControl(hParent, msg);
            }
        }
        */
        return false;
    }

    // ========== 原有绘图辅助函数 ==========
    void PremultiplyAlphaBits(void* bits, int width, int height) {
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
            }
        }
    }

    // ========== 控件 ID 管理 ==========
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
        : m_layoutType(LayoutType::None), m_hwnd(nullptr), m_id(-1), m_parent(parent),
        m_isCustomDraw(false), m_useD2D(false) {
        m_layoutParam[0] = m_layoutParam[1] = m_layoutParam[2] = m_layoutParam[3] = 0;
    }

    zhuziControl::~zhuziControl() {
        destroy();
    }

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

    BOOL zhuziControl::postMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
        if (m_hwnd) return PostMessageW(m_hwnd, uMsg, wParam, lParam);
        return FALSE;
    }

    LONG zhuziControl::getWindowLong(int nIndex) {
        if (m_hwnd) return GetWindowLongW(m_hwnd, nIndex);
        return 0;
    }

    LONG_PTR zhuziControl::getWindowLongPtr(int nIndex) {
        if (m_hwnd) return GetWindowLongPtrW(m_hwnd, nIndex);
        return 0;
    }

    LONG zhuziControl::setWindowLong(int nIndex, LONG dwNewLong) {
        if (m_hwnd) return SetWindowLongW(m_hwnd, nIndex, dwNewLong);
        return 0;
    }

    LONG_PTR zhuziControl::setWindowLongPtr(int nIndex, LONG_PTR dwNewLong) {
        if (m_hwnd) return SetWindowLongPtrW(m_hwnd, nIndex, dwNewLong);
        return 0;
    }

    BOOL zhuziControl::setWindowPos(HWND hWndInsertAfter,
        int x, int y, int cx, int cy, UINT _uFlags) {
        return ::SetWindowPos(m_hwnd, hWndInsertAfter, x, y, cx, cy, _uFlags);
    }

    BOOL zhuziControl::setWindowPos(int x, int y, int cx, int cy) {
        return SetWindowPos(m_hwnd, nullptr, x, y, cx, cy, SWP_NOZORDER);
    }

    bool zhuziControl::createControl(const wchar_t* className, int x, int y,
        int w, int h, DWORD style, DWORD exStyle, bool doSubClass) {
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
        if (doSubClass) SetWindowSubclass(m_hwnd, ControlWndProc, 0, (DWORD_PTR)this);
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
        if (m_hwnd && IsWindow(m_hwnd)) {
            RemoveWindowSubclass(m_hwnd, ControlWndProc, 0);
            DestroyWindow(m_hwnd);
        }
        m_hwnd = nullptr;
        if (m_id >= 2000 && m_id <= 2999) releaseId(m_id);
        m_id = -1;
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

    zhuziFont zhuziControl::getFont() const {
        if (!m_hwnd) return zhuziFont(L"Microsoft YaHei", 16);
        HFONT hFont = (HFONT)SendMessage(m_hwnd, WM_GETFONT, 0, 0);
        if (!hFont) return zhuziFont(L"Microsoft YaHei", 16);
        LOGFONTW lf = { 0 };
        GetObjectW(hFont, sizeof(LOGFONTW), &lf);
        HDC screenDC = GetDC(nullptr);
        int dpiY = GetDeviceCaps(screenDC, LOGPIXELSY);
        ReleaseDC(nullptr, screenDC);
        int pointSize = MulDiv(-lf.lfHeight, 72, dpiY);
        if (pointSize <= 0) pointSize = 16;
        bool bold = (lf.lfWeight >= FW_BOLD);
        bool italic = (lf.lfItalic != 0);
        bool underline = (lf.lfUnderline != 0);
        return zhuziFont(lf.lfFaceName, pointSize, bold, italic, underline);
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
        m_layoutParam[0] = x; m_layoutParam[1] = y;
        m_layoutParam[2] = w; m_layoutParam[3] = h;
    }

    LRESULT zhuziControl::SendWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam) const {
        return SendMessageW(m_hwnd, msg, wParam, lParam);
    }

    void zhuziControl::setWindowTheme(const zhuziString themeName) {
        if (m_hwnd) SetWindowTheme(m_hwnd, themeName.c_str(), NULL);
    }

    void zhuziControl::setWindowTheme(const zhuziString themeName, const zhuziString subIdList) {
        if (m_hwnd) SetWindowTheme(m_hwnd, themeName.c_str(), subIdList.c_str());
    }

    void zhuziControl::setFocus() {
        if (m_hwnd && IsWindow(m_hwnd)) ::SetFocus(m_hwnd);
    }

    // ========== ControlWndProc（使用无冒泡分发）==========
    LRESULT CALLBACK zhuziControl::ControlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        zhuziControl* pThis = (zhuziControl*)dwRefData;
        if (!pThis || pThis->m_hwnd != hwnd) {
            RemoveWindowSubclass(hwnd, ControlWndProc, uIdSubclass);
            return DefSubclassProc(hwnd, msg, wParam, lParam);
        }
        if (msg == WM_NCDESTROY) {
            RemoveWindowSubclass(hwnd, ControlWndProc, uIdSubclass);
            pThis->m_hwnd = nullptr;
            return DefSubclassProc(hwnd, msg, wParam, lParam);
        }

        zhuziMsg zmsg{ msg, wParam, lParam, 0, false };
        if (DispatchMessageToControl(hwnd, zmsg)) {
            return zmsg.result;
        }

        if (msg == WM_KEYDOWN) {
            SendMessage(GetParent(hwnd), msg, wParam, lParam);
        }

        if (!(pThis->m_isCustomDraw)) {
            return DefSubclassProc(hwnd, msg, wParam, lParam);
        }

        switch (msg) {
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (hdc) {
                RECT rcClient;
                GetClientRect(hwnd, &rcClient);
                int width = rcClient.right - rcClient.left;
                int height = rcClient.bottom - rcClient.top;
                if (width > 0 && height > 0) {
                    if (pThis->isUsingD2D()) {
                        zhuziD2DRenderTarget d2d(hwnd);
                        if (d2d.beginDraw()) {
                            d2d.clear(0x00000000);
                            pThis->onPaintD2D(d2d);
                            d2d.endDraw();
                        }
                    }
                    else {
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
                                memset(pvBits, 0, width * height * 4);
                                zhuziPaint paint(memDC, rcClient);
                                pThis->onPaint(paint);
                                BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
                                AlphaBlend(hdc, 0, 0, width, height, memDC, 0, 0, width, height, blend);
                                SelectObject(memDC, oldBitmap);
                                DeleteObject(memBitmap);
                            }
                            DeleteDC(memDC);
                        }
                        else {
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
        case WM_MOUSEMOVE:
            pThis->onMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_MOUSELEAVE:
            pThis->onMouseLeave();
            return 0;
        }
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    // ========== zhuziWindow 实现 ==========
    const wchar_t* zhuziWindow::WINDOW_CLASS_NAME = L"zhuziWindowClass";
    bool zhuziWindow::s_classRegistered = false;

    zhuziWindow::zhuziWindow() : zhuziControl(nullptr), m_hBgBrush(nullptr), m_minWidth(0), m_minHeight(0), m_maxWidth(0), m_maxHeight(0) {}
    zhuziWindow::~zhuziWindow() { if (m_hBgBrush) DeleteObject(m_hBgBrush); destroy(); }

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
        m_hwnd = CreateWindowExW(0, WINDOW_CLASS_NAME, m_windowTitle.c_str(), style,
            m_layoutParam[0], m_layoutParam[1], m_layoutParam[2], m_layoutParam[3],
            nullptr, nullptr, zhuziInstance::getHandle(), this);
        if (!m_hwnd) return false;
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
        UpdateWindow(m_hwnd);
        zhuziInstance::registerTopLevelWindow(m_hwnd);
        return true;
    }

    void zhuziWindow::destroy() {
        if (m_hwnd && IsWindow(m_hwnd)) {
            zhuziInstance::unregisterTopLevelWindow(m_hwnd);
            DestroyWindow(m_hwnd);
        }
        m_hwnd = nullptr;
        if (m_hBgBrush) DeleteObject(m_hBgBrush);
        m_hBgBrush = nullptr;
    }

    void zhuziWindow::show(int cmdShow) { if (m_hwnd) ShowWindow(m_hwnd, cmdShow); }
    void zhuziWindow::update() { if (m_hwnd) UpdateWindow(m_hwnd); }
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
    void zhuziWindow::setBgColor(COLORREF color) {
        if (m_hBgBrush) DeleteObject(m_hBgBrush);
        m_hBgBrush = CreateSolidBrush(color);
        if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
    }
    void zhuziWindow::setWindowRgn(zhuziRgn& rgn, bool bRedraw) { if (m_hwnd) ::SetWindowRgn(m_hwnd, rgn.release(), bRedraw ? TRUE : FALSE); }
    void zhuziWindow::setWindowRgn(HRGN hRgn, bool bRedraw) { if (m_hwnd && hRgn) ::SetWindowRgn(m_hwnd, hRgn, bRedraw ? TRUE : FALSE); }
    void zhuziWindow::clearWindowRgn(bool bRedraw) { if (m_hwnd) ::SetWindowRgn(m_hwnd, nullptr, bRedraw ? TRUE : FALSE); }

    void zhuziWindow::onParentResize(int parentWidth, int parentHeight) {
        if (m_hwnd) {
            HWND child = GetWindow(m_hwnd, GW_CHILD);
            while (child) {
                zhuziControl* ctrl = (zhuziControl*)GetWindowLongPtrW(child, GWLP_USERDATA);
                if (ctrl) {
                    RECT rc; GetClientRect(m_hwnd, &rc);
                    ctrl->onParentResize(rc.right - rc.left, rc.bottom - rc.top);
                }
                child = GetWindow(child, GW_HWNDNEXT);
            }
        }
    }

    LRESULT CALLBACK zhuziWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        zhuziWindow* pThis = nullptr;
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
        // 先尝试派发消息到全局绑定（冒泡）
        zhuziMsg zmsg{ msg, wParam, lParam, 0, false };
        if (DispatchMessageToControl(m_hwnd, zmsg)) {
            return zmsg.result;
        }

        // 原有的特殊消息处理
        if (msg == WM_ERASEBKGND) {
            if (m_hBgBrush) {
                HDC hdc = (HDC)wParam;
                RECT rc;
                GetClientRect(m_hwnd, &rc);
                FillRect(hdc, &rc, m_hBgBrush);
                return 1;
            }
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
        if (msg == WM_SIZE) {
            onParentResize(LOWORD(lParam), HIWORD(lParam));
            return 0;
        }
        if (msg == WM_NCDESTROY) {
            zhuziInstance::unregisterTopLevelWindow(m_hwnd);
            m_hwnd = nullptr;
            return 0;
        }
        return -1;
    }

    zhuziWindow* findParentWindow(zhuziControl* ctrl) {
        while (ctrl) {
            if (auto* w = dynamic_cast<zhuziWindow*>(ctrl)) return w;
            ctrl = ctrl->getParent();
        }
        return nullptr;
    }

    zhuziControl* GetControlFromHWND(HWND hwnd) {
        return reinterpret_cast<zhuziControl*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }
} // namespace zhuzi