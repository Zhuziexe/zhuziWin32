#pragma once
#include <windows.h>
#include <map>
#include <vector>
#include <functional>
#include <memory>
#include <stdexcept>
#include <bitset>
#include "zhuziString.h"
#include "zhuziFont.h"
#include "zhuziPaint.h"
#include "zhuziRgn.h"
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")
/*
* zhuziControl.h
* 定义控件基类和窗口类
*/

//===============================================
#define _CONTAINER_MSGHANDLER_IF \
if (msg == WM_COMMAND || msg == WM_NOTIFY) {\
HWND hParent = GetParent(hwnd);\
if (hParent) {\
SendMessageW(hParent, msg, wParam, lParam);\
return 0;}\
}\
else if (msg == WM_CTLCOLORSTATIC) {\
HDC hdc = (HDC)wParam;\
HWND hChild = (HWND)lParam;\
zhuziControl* pChildCtrl = (zhuziControl*)GetWindowLongPtrW(hChild, GWLP_USERDATA);\
if (pChildCtrl) {\
    if (auto* pFrame = dynamic_cast<zhuziFrame*>(pChildCtrl)) {\
        HBRUSH hBrush = pFrame->getBackgroundBrush();\
        if (hBrush) {\
            SetBkMode(hdc, TRANSPARENT);\
            return (LRESULT)hBrush;\
        }\
    }\
}\
SetBkMode(hdc, TRANSPARENT);\
return (LRESULT)GetStockObject(NULL_BRUSH);\
}

#define _CONTAINER_MSGHANDLER_IF_N \
if (msg == WM_COMMAND || msg == WM_NOTIFY) {\
HWND hParent = GetParent(hwnd);\
if (hParent) {\
SendMessageW(hParent, msg, wParam, lParam);}\
}\
else if (msg == WM_CTLCOLORSTATIC) {\
HDC hdc = (HDC)wParam;\
HWND hChild = (HWND)lParam;\
zhuziControl* pChildCtrl = (zhuziControl*)GetWindowLongPtrW(hChild, GWLP_USERDATA);\
if (pChildCtrl) {\
    if (auto* pFrame = dynamic_cast<zhuziFrame*>(pChildCtrl)) {\
        HBRUSH hBrush = pFrame->getBackgroundBrush();\
        if (hBrush) {\
            SetBkMode(hdc, TRANSPARENT);\
            return (LRESULT)hBrush;\
        }\
    }\
}\
SetBkMode(hdc, TRANSPARENT);\
return (LRESULT)GetStockObject(NULL_BRUSH);\
}
//===============================================

namespace zhuzi {
    class zhuziWindow;

    class zhuziControl {
    public:
        zhuziControl(zhuziControl* parent = nullptr);
        virtual ~zhuziControl();

        // 三种创建方式（内部记录布局参数，然后调用 onCreate）
        bool create(int x, int y, int width, int height, DWORD style = WS_CHILD | WS_VISIBLE);
        bool create(double xPercent, double yPercent, double widthPercent, double heightPercent, DWORD style = WS_CHILD | WS_VISIBLE);
        bool create(UINT toLeft, UINT toTop, UINT toRight, UINT toBottom, DWORD style = WS_CHILD | WS_VISIBLE);

        virtual void destroy();

        // 子类必须实现此方法，在其中创建实际窗口
        virtual bool onCreate(DWORD style) = 0;

        void setAnchor(UINT left, UINT top, UINT right, UINT bottom);

        HWND getHandle() const { return m_hwnd; }
        //在创建前调用
        void setId(int _id) { m_id = _id; }
        int getId() const { return m_id; }
        void setText(const zhuziString& text);
        zhuziString getText() const;
        void setRect(int x, int y, int width, int height);
        virtual void show(bool visible = true);
        void hide() { show(false); }
        virtual void enable(bool enabled = true);
        void disable() { enable(false); }
        void setFont(const zhuziFont& font);
        zhuziFont getFont() const;
        zhuziControl* getParent() const { return m_parent; }
        void setParent(zhuziControl* ctrl) { m_parent = ctrl; }

        virtual void onParentResize(int parentWidth, int parentHeight);

        void setGeometry(int x, int y, int width, int height);
        void setGeometry(double xPercent, double yPercent, double widthPercent, double heightPercent);
        void setFocus();

        // 仅更新布局参数（不移动窗口），用于高级控件过程中同步参数避免复位
        void setLayoutParamOnly(int x, int y, int w, int h);

        LRESULT SendWindowMessage(UINT msg, WPARAM wParam = NULL, LPARAM lParam = NULL) const;

		void setWindowTheme(const zhuziString themeName);

        // 绘图与鼠标事件（可被子类重写）
        virtual void onPaint(zhuziPaint& paint) {}
        virtual void onLButtonUp(int x, int y) {}
        virtual void onRButtonUp(int x, int y) {}
        virtual void onMouseMove(int x, int y) {}
        virtual void onMouseLeave() {}
            
        void Invalidate(bool bErase = 1) {
            if (m_hwnd) {
                InvalidateRect(m_hwnd, nullptr, bErase);
            }
        }

        // 禁止拷贝
        zhuziControl(const zhuziControl&) = delete;
        zhuziControl& operator=(const zhuziControl&) = delete;
        // 保留移动操作（可选）
        zhuziControl(zhuziControl&&) = default;
        zhuziControl& operator=(zhuziControl&&) = default;

        void setCustomLayout() {
            m_layoutType = LayoutType::Custom;
        }
    protected:
        bool createControl(const wchar_t* className, int x, int y, int width, int height, DWORD style, DWORD exStyle = 0, bool doSubClass = true);
        void applyLayout(int parentWidth, int parentHeight);
        static int allocateId();
        static void releaseId(int id);

        enum class LayoutType : char { None, Absolute, Percent, Anchor, Custom };
        int m_layoutParam[4];

        HWND m_hwnd;
        zhuziControl* m_parent;
        friend class zhuziWindow;

        short int m_id;
        uint8_t m_isCustomDraw = false;
        LayoutType m_layoutType;
        bool m_flag1 = 0;
        int8_t m_flag2 = 0;
        int16_t m_flag3 = 0;
        static LRESULT CALLBACK ControlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

        virtual bool getTransparent() const { return false; }

    private:
        static std::bitset<1000> s_idUsed;
        void initCreate(LayoutType type);

    };

    // zhuziWindow 声明
    class zhuziWindow : public zhuziControl {
    public:
        zhuziWindow(zhuziWindow* parent = nullptr);
        virtual ~zhuziWindow();

        // 保留原有的 create 方法（带标题）
        bool create(const zhuziString& title, int x, int y, int width, int height, DWORD style = WS_OVERLAPPEDWINDOW);
        virtual bool onCreate(DWORD style) override;
        virtual void destroy() override;

        void show(int cmdShow = SW_SHOW);
        void update();

        void Bind(UINT uMsg, std::function<void(WPARAM, LPARAM)> callback);
        void Bind(UINT uMsg, WPARAM wParam, std::function<void(LPARAM)> callback);
        int BindChain(UINT uMsg, std::function<bool(WPARAM, LPARAM)> callback);
        void Unbind(UINT uMsg);
        void Unbind(UINT uMsg, WPARAM wParam);
        int getNextControlId();
        virtual void onParentResize(int parentWidth, int parentHeight) override;

        // 设置窗口图标
        void setIcon(const zhuziString& filename);
        void setIcon(int resourceId);

        void setBgColor(COLORREF color);

        void setMinWidth(int width) { m_minWidth = (short)width; }
        void setMinHeight(int height) { m_minHeight = (short)height; }
        void setMaxWidth(int width) { m_maxWidth = (short)width; }
        void setMaxHeight(int height) { m_maxHeight = (short)height; }

        void setWindowRgn(zhuziRgn& rgn, bool bRedraw = true) {
            if (m_hwnd) {
                ::SetWindowRgn(m_hwnd, rgn.release(), bRedraw ? TRUE : FALSE);
            }
        }

        // 直接设置 HRGN（同样接管所有权）
        void setWindowRgn(HRGN hRgn, bool bRedraw = true) {
            if (m_hwnd && hRgn) {
                ::SetWindowRgn(m_hwnd, hRgn, bRedraw ? TRUE : FALSE);
            }
        }

        // 清除区域
        void clearWindowRgn(bool bRedraw = true) {
            if (m_hwnd) {
                ::SetWindowRgn(m_hwnd, nullptr, bRedraw ? TRUE : FALSE);
            }
        }
    private:
        static bool RegisterWindowClass();
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

        std::map<UINT, std::function<void(WPARAM, LPARAM)>> m_msgHandlers;
        std::map<std::pair<UINT, WPARAM>, std::function<void(LPARAM)>> m_msgWParamHandlers;
        std::map<UINT, std::vector<std::function<bool(WPARAM, LPARAM)>>> m_msgChainHandlers;

        static const wchar_t* WINDOW_CLASS_NAME;
        static bool s_classRegistered;
        zhuziString m_windowTitle;
        HBRUSH m_hBgBrush;                 // 背景画刷

        short m_minWidth = 0;
        short m_minHeight = 0;
        short m_maxWidth = 0;
        short m_maxHeight = 0;
    };
        inline zhuziWindow* findParentWindow(zhuziControl* ctrl) {
            while (ctrl) {
                if (auto* w = dynamic_cast<zhuziWindow*>(ctrl)) return w;
                ctrl = ctrl->getParent();
            }
            return nullptr;
        }
}