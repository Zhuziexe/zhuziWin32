#include "zhuziToolBar.h"
#include "zhuziInstance.h"
#include <commctrl.h>
#include <windowsx.h>

#pragma comment(lib, "comctl32.lib")

namespace zhuzi {

    std::bitset<1000> zhuziToolBar::s_cmdIdUsed;
    const int CMD_ID_BASE = 5000;

    int zhuziToolBar::AllocateCmdIdInternal() {
        for (size_t i = 0; i < 1000; ++i) {
            if (!s_cmdIdUsed[i]) {
                s_cmdIdUsed.set(i);
                return CMD_ID_BASE + static_cast<int>(i);
            }
        }
        return -1;
    }

    void zhuziToolBar::ReleaseCmdIdInternal(int cmdId) {
        if (cmdId >= CMD_ID_BASE && cmdId < CMD_ID_BASE + 1000) {
            size_t idx = cmdId - CMD_ID_BASE;
            if (s_cmdIdUsed[idx]) s_cmdIdUsed.reset(idx);
        }
    }

    int zhuziToolBar::allocateCmdId() {
        return AllocateCmdIdInternal();
    }

    void zhuziToolBar::releaseCmdId(int cmdId) {
        ReleaseCmdIdInternal(cmdId);
    }

    zhuziToolBar::zhuziToolBar(zhuziControl* parent)
        : zhuziControl(parent), m_himl(nullptr),
        m_bgColor(GetSysColor(COLOR_BTNFACE)), m_hBgBrush(nullptr) {
        INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES };
        InitCommonControlsEx(&icex);
    }

    zhuziToolBar::~zhuziToolBar() {
        if (m_hBgBrush) DeleteObject(m_hBgBrush);
        destroy();
    }

    bool zhuziToolBar::create() {
        if (m_hwnd) return false;
        if (m_id == -1) {
            try { m_id = allocateId(); }
            catch (...) { return false; }
        }
        HWND hParent = m_parent ? m_parent->getHandle() : nullptr;
        DWORD style = WS_CHILD | WS_VISIBLE | TBSTYLE_TOOLTIPS | TBSTYLE_WRAPABLE | CCS_NODIVIDER;
        m_hwnd = CreateWindowExW(0, TOOLBARCLASSNAME, L"", style,
            0, 0, 0, 0, hParent,
            (HMENU)(INT_PTR)m_id, zhuziInstance::getHandle(), nullptr);
        if (!m_hwnd) {
            releaseId(m_id); m_id = -1;
            return false;
        }
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

        // 初始化工具栏
        SendMessage(m_hwnd, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
        SendMessage(m_hwnd, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);
        setButtonSize(0, 0);   // 自动大小

        // 子类化以处理 WM_COMMAND
        SetWindowSubclass(m_hwnd, ToolBarSubclassProc, 0, (DWORD_PTR)this);

        // 初始布局
        if (m_parent) {
            RECT rc;
            GetClientRect(m_parent->getHandle(), &rc);
            onParentResize(rc.right - rc.left, rc.bottom - rc.top);
        }
        return true;
    }

    bool zhuziToolBar::onCreate(DWORD style) {
        // 兼容基类，内部直接调用 create
        return create();
    }

    void zhuziToolBar::setImageList(zhuziImageList* imageList) {
        if (imageList) setImageList(imageList->getHandle());
    }

    void zhuziToolBar::setImageList(HIMAGELIST himl) {
        m_himl = himl;
        if (m_hwnd) SendMessage(m_hwnd, TB_SETIMAGELIST, 0, (LPARAM)m_himl);
    }

    void zhuziToolBar::addSeparator() {
        if (!m_hwnd) return;
        TBBUTTON tb = {};
        tb.iBitmap = I_IMAGENONE;
        tb.idCommand = 0;
        tb.fsState = TBSTATE_ENABLED;
        tb.fsStyle = BTNS_SEP;
        SendMessage(m_hwnd, TB_ADDBUTTONS, 1, (LPARAM)&tb);
        autoSize();
        updateLayout();
    }

    int zhuziToolBar::addButton(const zhuziString& text, int cmdId, DWORD buttonStyle, int imageIndex) {
        if (!m_hwnd) return -1;
        if (cmdId == -1) {
            cmdId = AllocateCmdIdInternal();
            if (cmdId == -1) return -1;
        }

        TBBUTTON tb = {};
        tb.iBitmap = (imageIndex >= 0) ? imageIndex : I_IMAGENONE;
        tb.idCommand = cmdId;
        tb.fsState = TBSTATE_ENABLED;
        tb.fsStyle = (buttonStyle & ~BTNS_SEP) | BTNS_AUTOSIZE;

        if (!text.empty()) {
            tb.iString = (INT_PTR)SendMessageW(m_hwnd, TB_ADDSTRINGW, 0, (LPARAM)text.c_str());
        }

        if (SendMessageW(m_hwnd, TB_ADDBUTTONS, 1, (LPARAM)&tb) == 0) {
            if (cmdId >= CMD_ID_BASE) ReleaseCmdIdInternal(cmdId);
            return -1;
        }

        autoSize();
        updateLayout();
        return cmdId;
    }

    void zhuziToolBar::addButtons(const std::vector<ToolBarButtonInfo>& buttons) {
        for (const auto& info : buttons) {
            addButton(info.text, info.cmdId, info.style, info.imageIndex);
        }
    }

    bool zhuziToolBar::enableButton(int cmdId, bool enable) {
        if (!m_hwnd) return false;
        TBBUTTONINFOW info = { sizeof(TBBUTTONINFOW) };
        info.dwMask = TBIF_STATE;
        info.fsState = enable ? TBSTATE_ENABLED : 0;
        return SendMessageW(m_hwnd, TB_SETBUTTONINFOW, cmdId, (LPARAM)&info) != 0;
    }

    bool zhuziToolBar::checkButton(int cmdId, bool check) {
        if (!m_hwnd) return false;
        TBBUTTONINFOW info = { sizeof(TBBUTTONINFOW) };
        info.dwMask = TBIF_STATE;
        info.fsState = check ? TBSTATE_CHECKED : 0;
        return SendMessageW(m_hwnd, TB_SETBUTTONINFOW, cmdId, (LPARAM)&info) != 0;
    }

    bool zhuziToolBar::setButtonText(int cmdId, const zhuziString& text) {
        if (!m_hwnd) return false;
        int idx = (int)SendMessageW(m_hwnd, TB_ADDSTRINGW, 0, (LPARAM)text.c_str());
        if (idx == -1) return false;
        TBBUTTONINFOW info = { sizeof(TBBUTTONINFOW) };
        info.dwMask = TBIF_TEXT;
        info.pszText = (LPWSTR)MAKEINTRESOURCE(idx);
        return SendMessageW(m_hwnd, TB_SETBUTTONINFOW, cmdId, (LPARAM)&info) != 0;
    }

    bool zhuziToolBar::setButtonImage(int cmdId, int imageIndex) {
        if (!m_hwnd) return false;
        TBBUTTONINFOW info = { sizeof(TBBUTTONINFOW) };
        info.dwMask = TBIF_IMAGE;
        info.iImage = (imageIndex >= 0) ? imageIndex : I_IMAGENONE;
        return SendMessageW(m_hwnd, TB_SETBUTTONINFOW, cmdId, (LPARAM)&info) != 0;
    }

    bool zhuziToolBar::isButtonChecked(int cmdId) const {
        if (!m_hwnd) return false;
        TBBUTTONINFOW info = { sizeof(TBBUTTONINFOW) };
        info.dwMask = TBIF_STATE;
        if (!SendMessageW(m_hwnd, TB_GETBUTTONINFOW, cmdId, (LPARAM)&info)) return false;
        return (info.fsState & TBSTATE_CHECKED) != 0;
    }

    bool zhuziToolBar::isButtonEnabled(int cmdId) const {
        if (!m_hwnd) return false;
        TBBUTTONINFOW info = { sizeof(TBBUTTONINFOW) };
        info.dwMask = TBIF_STATE;
        if (!SendMessageW(m_hwnd, TB_GETBUTTONINFOW, cmdId, (LPARAM)&info)) return false;
        return (info.fsState & TBSTATE_ENABLED) != 0;
    }

    void zhuziToolBar::clearButtons() {
        if (!m_hwnd) return;
        SendMessageW(m_hwnd, TB_DELETEBUTTON, 0, (LPARAM)-1);
        autoSize();
        updateLayout();
    }

    void zhuziToolBar::setOnClick(int cmdId, std::function<void()> callback) {
        zhuziWindow* parentWnd = findParentWindow(this);
        if (parentWnd) {
            parentWnd->Bind(WM_COMMAND, (WPARAM)cmdId, [callback](LPARAM) {
                if (callback) callback();
                });
        }
    }

    void zhuziToolBar::setButtonSize(int width, int height) {
        if (!m_hwnd) return;
        if (width > 0 && height > 0)
            SendMessage(m_hwnd, TB_SETBUTTONSIZE, 0, MAKELPARAM(width, height));
        else
            SendMessage(m_hwnd, TB_SETBUTTONSIZE, 0, 0);
        autoSize();
        updateLayout();
    }

    void zhuziToolBar::setBitmapSize(int width, int height) {
        if (!m_hwnd) return;
        SendMessage(m_hwnd, TB_SETBITMAPSIZE, 0, MAKELPARAM(width, height));
        autoSize();
        updateLayout();
    }

    void zhuziToolBar::setMaxTextRows(int rows) {
        if (!m_hwnd) return;
        SendMessage(m_hwnd, TB_SETMAXTEXTROWS, rows, 0);
        autoSize();
        updateLayout();
    }

    void zhuziToolBar::enableToolTips(bool enable) {
        if (!m_hwnd) return;
        LONG_PTR style = GetWindowLongPtr(m_hwnd, GWL_STYLE);
        if (enable) style |= TBSTYLE_TOOLTIPS;
        else style &= ~TBSTYLE_TOOLTIPS;
        SetWindowLongPtr(m_hwnd, GWL_STYLE, style);
    }

    void zhuziToolBar::setBackgroundColor(COLORREF color) {
        m_bgColor = color;
        if (m_hBgBrush) DeleteObject(m_hBgBrush);
        m_hBgBrush = CreateSolidBrush(m_bgColor);
        updateBackground();
        if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void zhuziToolBar::updateBackground() {
        if (!m_hwnd) return;
        SetClassLongPtr(m_hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)m_hBgBrush);
    }

    void zhuziToolBar::autoSize() {
        if (m_hwnd) SendMessage(m_hwnd, TB_AUTOSIZE, 0, 0);
    }

    void zhuziToolBar::updateLayout() {
        if (!m_hwnd || !m_parent) return;

        HWND hParent = m_parent->getHandle();
        RECT rcParent;
        GetClientRect(hParent, &rcParent);
        int parentWidth = rcParent.right - rcParent.left;

        // 获取工具栏所需高度
        RECT rcToolbar;
        GetWindowRect(m_hwnd, &rcToolbar);
        int height = rcToolbar.bottom - rcToolbar.top;
        if (height == 0) height = 28; // 保底高度

        // 放置在顶部 (y=0)，宽度填满
        SetWindowPos(m_hwnd, nullptr, 0, 0, parentWidth, height, SWP_NOZORDER);
    }

    void zhuziToolBar::onParentResize(int parentWidth, int parentHeight) {
        updateLayout();
    }

    void zhuziToolBar::setButtonTooltip(int cmdId, const zhuziString& tooltip) {
        m_tooltips[cmdId] = tooltip;
        // 工具提示不需要立即刷新，下次鼠标停留时自动显示
    }

    LRESULT CALLBACK zhuziToolBar::ToolBarSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        zhuziToolBar* pThis = reinterpret_cast<zhuziToolBar*>(dwRefData);
        if (!pThis) return DefSubclassProc(hwnd, msg, wParam, lParam);

        if (msg == WM_NOTIFY) {
            NMHDR* pnmh = reinterpret_cast<NMHDR*>(lParam);
            if (pnmh->code == TTN_NEEDTEXTW) {
                // 处理工具提示（同之前代码）
                NMTTDISPINFOW* pttdi = reinterpret_cast<NMTTDISPINFOW*>(lParam);
                int cmdId = static_cast<int>(pnmh->idFrom);
                auto it = pThis->m_tooltips.find(cmdId);
                if (it != pThis->m_tooltips.end() && !it->second.empty()) {
                    static std::wstring buffer;
                    buffer = it->second.c_str();
                    pttdi->lpszText = const_cast<LPWSTR>(buffer.c_str());
                    pttdi->hinst = nullptr;
                    pttdi->uFlags = 0;
                    return 0;
                }
            }
        }
        // 不处理 WM_COMMAND，让消息自然传递
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

} // namespace zhuzi