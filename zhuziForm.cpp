#include "zhuziForm.h"
#include "zhuziInstance.h"
#include <windowsx.h>
#include <CommCtrl.h>

namespace zhuzi {

    // ---------- 静态成员定义 ----------
    const wchar_t* zhuziForm::FORM_CLASS_NAME = L"zhuziFormClass";
    bool zhuziForm::s_classRegistered = false;

    // ---------- 注册窗口类（静态成员函数） ----------
    bool zhuziForm::RegisterFormClass() {
        if (s_classRegistered) return true;
        WNDCLASSW wc = {};
        wc.lpfnWndProc = zhuziForm::FormWndProc;
        wc.hInstance = zhuziInstance::getHandle();
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = FORM_CLASS_NAME;
        s_classRegistered = (RegisterClassW(&wc) != 0);
        return s_classRegistered;
    }

    // ---------- 静态窗口过程 ----------
    LRESULT CALLBACK zhuziForm::FormWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    // ---------- 构造/析构 ----------
    zhuziForm::zhuziForm(zhuziControl* parent)
        : zhuziControl(parent)
        , m_spacing(0)
        , m_scrollPos(0)
        , m_contentHeight(0)
        , m_vScrollBindId(-1)
        , m_mouseWheelBindId(-1) {
        m_isCustomDraw = true;
    }

    zhuziForm::~zhuziForm() {
        destroy();
    }

    // ---------- 创建窗口 ----------
    bool zhuziForm::onCreate(DWORD style) {
        if (m_hwnd) return true;

        // 注册自定义窗口类
        if (!RegisterFormClass()) return false;

        // 分配控件 ID
        if (m_id == -1) {
            try { m_id = allocateId(); }
            catch (...) { return false; }
        }

        HWND hParent = m_parent ? m_parent->getHandle() : nullptr;
        // 使用自定义类创建窗口，样式添加垂直滚动条
        DWORD dwStyle = style | WS_CHILD | WS_VISIBLE | WS_VSCROLL;
        m_hwnd = CreateWindowExW(0, FORM_CLASS_NAME, L"", dwStyle,
            0, 0, 0, 0, hParent, (HMENU)(INT_PTR)m_id,
            zhuziInstance::getHandle(), nullptr);

        if (!m_hwnd) {
            releaseId(m_id);
            m_id = -1;
            return false;
        }

        // 设置用户数据并子类化
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
        SetWindowSubclass(m_hwnd, ControlWndProc, 0, (DWORD_PTR)this);

        // 应用布局
        if (m_parent) {
            RECT rc;
            GetClientRect(m_parent->getHandle(), &rc);
            applyLayout(rc.right - rc.left, rc.bottom - rc.top);
        }

        // 绑定滚动消息
        m_vScrollBindId = Bind(this, WM_VSCROLL, [this](zhuziMsg* msg) {
            this->onVScroll(msg->wParam, msg->lParam);
            msg->handled = true;
            });

        // 绑定鼠标滚轮消息
        m_mouseWheelBindId = Bind(this, WM_MOUSEWHEEL, [this](zhuziMsg* msg) {
            this->onMouseWheel(msg->wParam, msg->lParam);
            msg->handled = true;
            });

        return true;
    }

    void zhuziForm::destroy() {
        if (m_vScrollBindId != -1) {
            Unbind(m_vScrollBindId);
            m_vScrollBindId = -1;
        }
        if (m_mouseWheelBindId != -1) {
            Unbind(m_mouseWheelBindId);
            m_mouseWheelBindId = -1;
        }
        zhuziControl::destroy();
    }

    // ---------- 绘制背景 ----------
    void zhuziForm::onPaint(zhuziPaint& paint) {
        RECT client;
        GetClientRect(m_hwnd, &client);
        int width = client.right - client.left;
        int height = client.bottom - client.top;
        if (width <= 0 || height <= 0) return;

        zhuziBrush bgBrush(RGB(255, 255, 255));  // 白色背景
        paint.fillRect(0, 0, width, height, bgBrush);
    }

    // ---------- 销毁所有子控件 ----------
    void zhuziForm::destroyAllControls() {
        if (!m_hwnd) return;
        HWND child = GetWindow(m_hwnd, GW_CHILD);
        while (child) {
            HWND next = GetWindow(child, GW_HWNDNEXT);
            zhuziControl* ctrl = GetControlFromHWND(child);
            if (ctrl) {
                ctrl->destroy();
            }
            child = next;
        }
        m_contentHeight = 0;
        m_scrollPos = 0;
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    // ---------- 添加/修改子控件 ----------
    void zhuziForm::addControl(zhuziControl* pCtrl, double widthPercent, int height, AlignHorizontal align) {
        if (!pCtrl) return;
        if (!pCtrl->getHandle()) {
            pCtrl->create(0, 0, 0, 0, WS_CHILD | WS_VISIBLE);
            pCtrl->setCustomLayout();
        }
        pCtrl->setLayoutParamOnly((int)align, 1, height, (int)(widthPercent * 10000));
        layoutChildren();
    }

    void zhuziForm::addControl(zhuziControl* pCtrl, int width, int height, AlignHorizontal align) {
        if (!pCtrl) return;
        if (!pCtrl->getHandle()) {
            pCtrl->create(0, 0, 0, 0, WS_CHILD | WS_VISIBLE);
            pCtrl->setCustomLayout();
        }
        pCtrl->setLayoutParamOnly((int)align, 0, height, width);
        layoutChildren();
    }

    void zhuziForm::setControlWidth(zhuziControl* pCtrl, double widthPercent) {
        if (!pCtrl) return;
        const int* lp = pCtrl->getLayoutParamPtr();
        if (!lp) return;
        pCtrl->setLayoutParamOnly(lp[0], 1, lp[2], (int)(widthPercent * 10000));
        layoutChildren();
    }

    void zhuziForm::setControlWidth(zhuziControl* pCtrl, int width) {
        if (!pCtrl) return;
        const int* lp = pCtrl->getLayoutParamPtr();
        if (!lp) return;
        pCtrl->setLayoutParamOnly(lp[0], 0, lp[2], width);
        layoutChildren();
    }

    void zhuziForm::setControlHeight(zhuziControl* pCtrl, int height) {
        if (!pCtrl) return;
        const int* lp = pCtrl->getLayoutParamPtr();
        if (!lp) return;
        pCtrl->setLayoutParamOnly(lp[0], lp[1], height, lp[3]);
        layoutChildren();
    }

    // ---------- 父窗口尺寸变化 ----------
    void zhuziForm::onParentResize(int parentWidth, int parentHeight) {
        layoutChildren();
        zhuziControl::onParentResize(parentWidth, parentHeight);
    }

    // ---------- 核心布局 ----------
    void zhuziForm::layoutChildren() {
        if (!m_hwnd) return;

        RECT client;
        GetClientRect(m_hwnd, &client);
        int clientWidth = client.right - client.left;
        int clientHeight = client.bottom - client.top;
        if (clientWidth <= 0 || clientHeight <= 0) return;

        HWND child = GetWindow(m_hwnd, GW_CHILD);
        std::vector<HWND> children;
        while (child) {
            children.push_back(child);
            child = GetWindow(child, GW_HWNDNEXT);
        }

        struct ChildRect { int x, y, w, h; };
        std::vector<ChildRect> rects;
        rects.reserve(children.size());

        int y = 0;
        for (HWND hwndChild : children) {
            zhuziControl* ctrl = GetControlFromHWND(hwndChild);
            if (!ctrl) continue;
            const int* lp = ctrl->getLayoutParamPtr();
            if (!lp) continue;

            AlignHorizontal align = (AlignHorizontal)lp[0];
            int widthMode = lp[1];
            int height = lp[2];
            int widthVal = lp[3];

            int width;
            if (widthMode == 1) {
                width = (int)(clientWidth * (widthVal / 10000.0));
            }
            else {
                width = widthVal;
            }
            if (width > clientWidth) width = clientWidth;

            int x;
            switch (align) {
            case AlignHorizontal::Left:   x = 0; break;
            case AlignHorizontal::Center: x = (clientWidth - width) / 2; break;
            case AlignHorizontal::Right:  x = clientWidth - width; break;
            default: x = 0;
            }
            if (x < 0) x = 0;

            rects.push_back({ x, y, width, height });
            y += height + m_spacing;
        }

        // 修正：总高度为最后一个控件底部，保留末尾间距（不减去）
        int totalHeight = 0;
        if (!rects.empty()) {
            totalHeight = rects.back().y + rects.back().h;  // 不再减去 m_spacing
        }
        m_contentHeight = totalHeight;

        updateScrollInfo(clientHeight);

        int offset = m_scrollPos;
        for (size_t i = 0; i < rects.size(); ++i) {
            HWND hwndChild = children[i];
            zhuziControl* ctrl = GetControlFromHWND(hwndChild);
            if (!ctrl) continue;
            const ChildRect& rc = rects[i];
            int top = rc.y - offset;
            SetWindowPos(ctrl->getHandle(), nullptr,
                rc.x, top, rc.w, rc.h,
                SWP_NOZORDER | SWP_NOACTIVATE);
        }

        InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    // ---------- 更新滚动条 ----------
    void zhuziForm::updateScrollInfo(int clientHeight) {
        SCROLLINFO si = { sizeof(SCROLLINFO) };
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = 0;
        si.nMax = m_contentHeight;
        si.nPage = clientHeight;
        si.nPos = m_scrollPos;
        int maxPos = si.nMax - (int)si.nPage;
        if (maxPos < 0) maxPos = 0;
        if (si.nPos > maxPos) si.nPos = maxPos;
        SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
        m_scrollPos = si.nPos;
    }

    // ---------- 垂直滚动 ----------
    void zhuziForm::onVScroll(WPARAM wParam, LPARAM lParam) {
        SCROLLINFO si = { sizeof(SCROLLINFO) };
        si.fMask = SIF_ALL;
        GetScrollInfo(m_hwnd, SB_VERT, &si);

        int newPos = si.nPos;
        switch (LOWORD(wParam)) {
        case SB_LINEUP:      newPos -= 10; break;
        case SB_LINEDOWN:    newPos += 10; break;
        case SB_PAGEUP:      newPos -= si.nPage; break;
        case SB_PAGEDOWN:    newPos += si.nPage; break;
        case SB_THUMBTRACK:  newPos = si.nTrackPos; break;
        case SB_TOP:         newPos = 0; break;
        case SB_BOTTOM:      newPos = si.nMax - si.nPage; break;
        default: return;
        }
        if (newPos < 0) newPos = 0;
        int maxPos = si.nMax - (int)si.nPage;
        if (maxPos < 0) maxPos = 0;
        if (newPos > maxPos) newPos = maxPos;

        if (newPos != m_scrollPos) {
            m_scrollPos = newPos;
            si.fMask = SIF_POS;
            si.nPos = m_scrollPos;
            SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
            layoutChildren();
        }
    }

    // ---------- 鼠标滚轮 ----------
    void zhuziForm::onMouseWheel(WPARAM wParam, LPARAM lParam) {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        int lines = delta / WHEEL_DELTA;  // 每次滚动一行
        // 转换为滚动条移动
        SCROLLINFO si = { sizeof(SCROLLINFO) };
        si.fMask = SIF_ALL;
        GetScrollInfo(m_hwnd, SB_VERT, &si);

        int newPos = si.nPos - lines * 10; // 每次滚动10像素
        if (newPos < 0) newPos = 0;
        int maxPos = si.nMax - (int)si.nPage;
        if (maxPos < 0) maxPos = 0;
        if (newPos > maxPos) newPos = maxPos;

        if (newPos != m_scrollPos) {
            m_scrollPos = newPos;
            si.fMask = SIF_POS;
            si.nPos = m_scrollPos;
            SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
            layoutChildren();
        }
    }

} // namespace zhuzi