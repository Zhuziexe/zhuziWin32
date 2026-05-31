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

    // 前向声明
    class zhuziWindow;

    // 消息结构体
    struct zhuziMessage {
        UINT msg;
        WPARAM wParam;
        LPARAM lParam;
        LRESULT result;     // 输出结果
        bool handled;       // 内部使用，暂不暴露
    };

    // ==================== zhuziControl 基类 ====================
    class zhuziControl {
    public:
        zhuziControl(zhuziControl* parent = nullptr);
        virtual ~zhuziControl();

        bool create(int x, int y, int width, int height, DWORD style = WS_CHILD | WS_VISIBLE);
        bool create(double xPercent, double yPercent, double widthPercent, double heightPercent, DWORD style = WS_CHILD | WS_VISIBLE);
        bool create(UINT toLeft, UINT toTop, UINT toRight, UINT toBottom, DWORD style = WS_CHILD | WS_VISIBLE);
        virtual void destroy();
        virtual bool onCreate(DWORD style) = 0;

        void setAnchor(UINT left, UINT top, UINT right, UINT bottom);
        HWND getHandle() const { return m_hwnd; }
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
        void setLayoutParamOnly(int x, int y, int w, int h);
        LRESULT SendWindowMessage(UINT msg, WPARAM wParam = NULL, LPARAM lParam = NULL) const;
        void setWindowTheme(const zhuziString themeName);
        virtual void onPaint(zhuziPaint& paint) {}
        virtual void onLButtonUp(int x, int y) {}
        virtual void onRButtonUp(int x, int y) {}
        virtual void onMouseMove(int x, int y) {}
        virtual void onMouseLeave() {}
        void Invalidate(bool bErase = 1) { if (m_hwnd) InvalidateRect(m_hwnd, nullptr, bErase); }

        zhuziControl(const zhuziControl&) = delete;
        zhuziControl& operator=(const zhuziControl&) = delete;
        zhuziControl(zhuziControl&&) = default;
        zhuziControl& operator=(zhuziControl&&) = default;

        void setCustomLayout() { m_layoutType = LayoutType::Custom; }

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

        static LRESULT CALLBACK ControlWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        virtual bool getTransparent() const { return false; }

    private:
        static std::bitset<1000> s_idUsed;
        void initCreate(LayoutType type);
    };

    // ==================== zhuziWindow 类（重构后） ====================
    class zhuziWindow : public zhuziControl {
    public:
        zhuziWindow();
        virtual ~zhuziWindow();

        bool create(const zhuziString& title, int x, int y, int width, int height, DWORD style = WS_OVERLAPPEDWINDOW);
        virtual bool onCreate(DWORD style) override;
        virtual void destroy() override;

        void show(int cmdShow = SW_SHOW);
        void update();
        void setIcon(const zhuziString& filename);
        void setIcon(int resourceId);
        void setBgColor(COLORREF color);
        void setMinWidth(int width) { m_minWidth = (short)width; }
        void setMinHeight(int height) { m_minHeight = (short)height; }
        void setMaxWidth(int width) { m_maxWidth = (short)width; }
        void setMaxHeight(int height) { m_maxHeight = (short)height; }
        void setWindowRgn(zhuziRgn& rgn, bool bRedraw = true);
        void setWindowRgn(HRGN hRgn, bool bRedraw = true);
        void clearWindowRgn(bool bRedraw = true);
        virtual void onParentResize(int parentWidth, int parentHeight) override;

        // ----- 新的事件绑定接口（Bind/Unbind）-----
        // 添加普通消息处理器（链式），返回 handler ID
        int Bind(UINT msg, std::function<bool(zhuziMessage&)> handler);
        // 添加精确匹配（按 wParam 过滤）的处理器（只能有一个，会覆盖之前的）
        void Bind(UINT msg, WPARAM wParam, std::function<bool(zhuziMessage&)> handler);
        // 移除指定 ID 的处理器
        void Unbind(int handlerId);
        // 移除指定消息的所有普通处理器（不包括精确匹配）
        void Unbind(UINT msg);

    private:
        static bool RegisterWindowClass();
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

        static const wchar_t* WINDOW_CLASS_NAME;
        static bool s_classRegistered;

        zhuziString m_windowTitle;
        HBRUSH m_hBgBrush;
        short m_minWidth, m_minHeight, m_maxWidth, m_maxHeight;

        // 事件存储
        struct Handler {
            int id;
            std::function<bool(zhuziMessage&)> func;
        };
        std::map<UINT, std::vector<Handler>> m_handlers;                    // 普通链式处理器
        std::map<std::pair<UINT, WPARAM>, Handler> m_exactHandlers;        // 精确处理器
        int m_nextHandlerId;      // 用于生成唯一 ID
    };

    // 辅助函数：查找祖先窗口中的 zhuziWindow
    inline zhuziWindow* findParentWindow(zhuziControl* ctrl) {
        while (ctrl) {
            if (auto* w = dynamic_cast<zhuziWindow*>(ctrl)) return w;
            ctrl = ctrl->getParent();
        }
        return nullptr;
    }

} // namespace zhuzi