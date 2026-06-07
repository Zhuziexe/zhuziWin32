#define _ZHUZI_LAYOUT_NO_WARNINGS
#include "zhuziLayout.h"
#include "zhuziInstance.h"
#include <windowsx.h>
#include <numeric>
#include "zhuziCommctrl.h"

namespace zhuzi {

    // 修复：增加对 pThis 有效性的检查
    static LRESULT CALLBACK LayoutWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        zhuziControl* pThis = (zhuziControl*)dwRefData;
        if (!pThis || pThis->getHandle() != hwnd) {
            // 控件已销毁或指针无效，移除子类化
            RemoveWindowSubclass(hwnd, LayoutWndProc, uIdSubclass);
            return DefSubclassProc(hwnd, msg, wParam, lParam);
        }
        _CONTAINER_MSGHANDLER_IF_N;
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    // ==================== zhuziLayout 实现 ====================
    zhuziLayout::zhuziLayout(zhuziControl* parent)
        : zhuziControl(parent), m_spacing(0),
        m_marginLeft(0), m_marginTop(0), m_marginRight(0), m_marginBottom(0),
        m_defaultWidth(100), m_defaultHeight(30) {
    }

    zhuziLayout::~zhuziLayout() {
        destroy();  // 会调用 clear 并销毁窗口
    }

    void zhuziLayout::destroy() {
        // 清空子控件列表，避免在窗口销毁后访问
        m_items.clear();
        zhuziControl::destroy();
    }

    void zhuziLayout::addControl(zhuziControl* control, int stretch) {
        if (!control) return;
        if (stretch <= 0) stretch = 1;

        ensureControlCreated(control);
        control->setCustomLayout();

        control->setParent(this);
        if (m_hwnd && control->getHandle()) {
            SetParent(control->getHandle(), m_hwnd);
        }

        m_items.push_back({ control, stretch });
        updateLayout();
    }

    void zhuziLayout::removeControl(zhuziControl* control) {
        auto it = std::find_if(m_items.begin(), m_items.end(),
            [control](const ControlItem& item) { return item.control == control; });
        if (it != m_items.end()) {
            m_items.erase(it);
            updateLayout();
        }
    }

    void zhuziLayout::clear() {
        m_items.clear();
        updateLayout();
    }

    void zhuziLayout::setSpacing(int spacing) {
        m_spacing = spacing;
        updateLayout();
    }

    void zhuziLayout::setContentsMargins(int left, int top, int right, int bottom) {
        m_marginLeft = left;
        m_marginTop = top;
        m_marginRight = right;
        m_marginBottom = bottom;
        updateLayout();
    }

    void zhuziLayout::getContentsMargins(int& left, int& top, int& right, int& bottom) const {
        left = m_marginLeft;
        top = m_marginTop;
        right = m_marginRight;
        bottom = m_marginBottom;
    }

    void zhuziLayout::setDefaultControlSize(int width, int height) {
        if (width > 0) m_defaultWidth = width;
        if (height > 0) m_defaultHeight = height;
    }

    void zhuziLayout::ensureControlCreated(zhuziControl* control) {
        if (!control->getHandle()) {
            control->create(0, 0, m_defaultWidth, m_defaultHeight);
        }
    }

    void zhuziLayout::getControlSize(zhuziControl* control, int& width, int& height) {
        HWND hwnd = control->getHandle();
        if (hwnd) {
            RECT rc;
            GetWindowRect(hwnd, &rc);
            width = rc.right - rc.left;
            height = rc.bottom - rc.top;
            if (width <= 0) width = m_defaultWidth;
            if (height <= 0) height = m_defaultHeight;
        }
        else {
            width = m_defaultWidth;
            height = m_defaultHeight;
        }
    }

    void zhuziLayout::onParentResize(int parentWidth, int parentHeight) {
        zhuziControl::onParentResize(parentWidth, parentHeight);
        updateLayout();
    }

    // ==================== zhuziHLayout 实现 ====================
    zhuziHLayout::zhuziHLayout(zhuziControl* parent) : zhuziLayout(parent) {}
    zhuziHLayout::~zhuziHLayout() {}

    bool zhuziHLayout::onCreate(DWORD style) {
        DWORD finalStyle = style | WS_CHILD | WS_VISIBLE | SS_NOTIFY;
        if (!createControl(L"STATIC", 0, 0, 0, 0, finalStyle, WS_EX_TRANSPARENT, true))
            return false;
        SetWindowSubclass(m_hwnd, LayoutWndProc, 0, (DWORD_PTR)this);
        if (m_parent) setAnchor(0, 0, 0, 0);
        return true;
    }

    void zhuziHLayout::updateLayout() {
        if (!m_hwnd || m_items.empty()) return;

        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);
        int totalWidth = rcClient.right - rcClient.left - m_marginLeft - m_marginRight;
        int totalHeight = rcClient.bottom - rcClient.top - m_marginTop - m_marginBottom;
        int y = m_marginTop;

        int totalStretch = 0;
        for (const auto& item : m_items) totalStretch += item.stretch;

        int totalSpacing = (int)m_items.size() - 1;
        int availableWidth = totalWidth - totalSpacing * m_spacing;
        if (availableWidth < 0) availableWidth = 0;

        int x = m_marginLeft;
        for (size_t i = 0; i < m_items.size(); ++i) {
            auto& item = m_items[i];
            zhuziControl* ctrl = item.control;
            if (!ctrl || !ctrl->getHandle()) continue;   // 安全跳过

            int stretch = item.stretch;
            int width = (totalStretch > 0) ? (availableWidth * stretch) / totalStretch : availableWidth / (int)m_items.size();
            if (width < 0) width = 0;

            int height = totalHeight;
            if (height < 0) height = m_defaultHeight;

            SetWindowPos(ctrl->getHandle(), nullptr, x, y, width, height, SWP_NOZORDER);
            x += width + m_spacing;
        }
    }

    // ==================== zhuziVLayout 实现 ====================
    zhuziVLayout::zhuziVLayout(zhuziControl* parent) : zhuziLayout(parent) {}
    zhuziVLayout::~zhuziVLayout() {}

    bool zhuziVLayout::onCreate(DWORD style) {
        DWORD finalStyle = style | WS_CHILD | WS_VISIBLE | SS_NOTIFY;
        if (!createControl(L"STATIC", 0, 0, 0, 0, finalStyle, WS_EX_TRANSPARENT, true))
            return false;
        SetWindowSubclass(m_hwnd, LayoutWndProc, 0, (DWORD_PTR)this);
        if (m_parent) setAnchor(0, 0, 0, 0);
        return true;
    }

    void zhuziVLayout::updateLayout() {
        if (!m_hwnd || m_items.empty()) return;

        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);
        int totalWidth = rcClient.right - rcClient.left - m_marginLeft - m_marginRight;
        int totalHeight = rcClient.bottom - rcClient.top - m_marginTop - m_marginBottom;
        int x = m_marginLeft;

        int totalStretch = 0;
        for (const auto& item : m_items) totalStretch += item.stretch;

        int totalSpacing = (int)m_items.size() - 1;
        int availableHeight = totalHeight - totalSpacing * m_spacing;
        if (availableHeight < 0) availableHeight = 0;

        int y = m_marginTop;
        for (size_t i = 0; i < m_items.size(); ++i) {
            auto& item = m_items[i];
            zhuziControl* ctrl = item.control;
            if (!ctrl || !ctrl->getHandle()) continue;

            int stretch = item.stretch;
            int height = (totalStretch > 0) ? (availableHeight * stretch) / totalStretch : availableHeight / (int)m_items.size();
            if (height < 0) height = 0;

            int width = totalWidth;
            if (width < 0) width = m_defaultWidth;

			ctrl->setGeometry(x, y, width, height);
            y += height + m_spacing;
        }
    }

} // namespace zhuzi