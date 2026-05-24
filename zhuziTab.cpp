#include "zhuziTab.h"
#include "zhuziInstance.h"
#include "zhuziControl.h"
#include <windowsx.h>

#pragma comment(lib, "comctl32.lib")

namespace zhuzi {

    zhuziTab::zhuziTab(zhuziControl* parent)
        : zhuziControl(parent)
        , m_hImageList(nullptr)
        , m_curSel(-1)
        , m_isSubclassed(false)
        , m_notifyRegistered(false)
        , m_originalStyle(WS_CHILD | WS_VISIBLE) {}

    zhuziTab::~zhuziTab() {
        destroy();
    }

    bool zhuziTab::onCreate(DWORD style) {
        // 保存原始样式
        m_originalStyle = style;

        INITCOMMONCONTROLSEX icex = {};
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_TAB_CLASSES;
        InitCommonControlsEx(&icex);

        HWND hParent = m_parent ? m_parent->getHandle() : nullptr;
        // 创建时使用临时位置(0,0,0,0)，基类后续会通过applyLayout调整位置和大小
        m_hwnd = CreateWindowExW(0, WC_TABCONTROLW, L"",
            style | WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0, hParent,
            (HMENU)(INT_PTR)m_id,
            zhuziInstance::getHandle(),
            nullptr);

        if (!m_hwnd) return false;

        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

        if (!m_isSubclassed) {
            SetWindowSubclass(m_hwnd, TabWndProc, 0, (DWORD_PTR)this);
            m_isSubclassed = true;
        }

        registerParentNotify();

        // 立即应用布局（因为父窗口可能已经具有大小）
        if (m_parent) {
            RECT parentRect;
            GetClientRect(m_parent->getHandle(), &parentRect);
            applyLayout(parentRect.right - parentRect.left, parentRect.bottom - parentRect.top);
        }

        return true;
    }

    void zhuziTab::updateTabStyle() {
        if (!m_hwnd) return;

        LONG_PTR currentStyle = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
        LONG_PTR baseStyle = currentStyle & (WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN);
        LONG_PTR newStyle = baseStyle | (m_originalStyle & ~(WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN));

        if (currentStyle != newStyle) {
            SetWindowLongPtrW(m_hwnd, GWL_STYLE, newStyle);
            SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            InvalidateRect(m_hwnd, nullptr, TRUE);
        }
    }

    DWORD zhuziTab::getTabStyle() const {
        return m_originalStyle;
    }

    void zhuziTab::setFlatButtons(bool flat) {
        if (flat) {
            m_originalStyle |= TCS_FLATBUTTONS;
        }
        else {
            m_originalStyle &= ~TCS_FLATBUTTONS;
        }
        updateTabStyle();
    }

    void zhuziTab::setBottom(bool bottom) {
        if (bottom) {
            m_originalStyle |= TCS_BOTTOM;
        }
        else {
            m_originalStyle &= ~TCS_BOTTOM;
        }
        updateTabStyle();
        updateCurrentPageLayout();
    }

    void zhuziTab::setMultiLine(bool multiLine) {
        if (multiLine) {
            m_originalStyle |= TCS_MULTILINE;
        }
        else {
            m_originalStyle &= ~TCS_MULTILINE;
        }
        updateTabStyle();
        updateCurrentPageLayout();
    }

    void zhuziTab::setFixedWidth(bool fixed, int width) {
        if (fixed) {
            m_originalStyle |= TCS_FIXEDWIDTH;
            if (width > 0 && m_hwnd) {
                SendMessageW(m_hwnd, TCM_SETITEMSIZE, 0, MAKELPARAM(width, 0));
            }
        }
        else {
            m_originalStyle &= ~TCS_FIXEDWIDTH;
        }
        updateTabStyle();
    }

    void zhuziTab::setButtonMode(bool buttonMode) {
        if (buttonMode) {
            m_originalStyle |= TCS_BUTTONS;
        }
        else {
            m_originalStyle &= ~TCS_BUTTONS;
        }
        updateTabStyle();
    }

    void zhuziTab::setFocusOnSelChange(bool focus) {
        if (focus) {
            m_originalStyle &= ~TCS_FOCUSNEVER;
            m_originalStyle |= TCS_FOCUSONBUTTONDOWN;
        }
        else {
            m_originalStyle |= TCS_FOCUSNEVER;
        }
        updateTabStyle();
    }

    void zhuziTab::registerParentNotify() {
        if (m_notifyRegistered) return;

        zhuziWindow* parentWnd = nullptr;
        zhuziControl* p = m_parent;
        while (p) {
            parentWnd = dynamic_cast<zhuziWindow*>(p);
            if (parentWnd) break;
            p = p->getParent();
        }

        if (parentWnd) {
            parentWnd->BindChain(WM_NOTIFY, [this](WPARAM, LPARAM lParam) -> bool {
                NMHDR* pnmh = reinterpret_cast<NMHDR*>(lParam);
                if (pnmh->hwndFrom == m_hwnd) {
                    return handleNotifyFromParent(pnmh);
                }
                return false;
                });
            m_notifyRegistered = true;
        }
    }

    bool zhuziTab::handleNotifyFromParent(NMHDR* pnmh) {
        if (pnmh->code == TCN_SELCHANGE) {
            int newSel = getCurSel();
            if (newSel != m_curSel) {
                int oldSel = m_curSel;
                m_curSel = newSel;
                updatePagesVisibility();
                updateCurrentPageLayout();
                if (m_onSelChange) {
                    m_onSelChange(oldSel, newSel);
                }
            }
            return true;
        }
        return false;
    }

    void zhuziTab::destroy() {
        if (m_hwnd) {
            if (m_isSubclassed) {
                RemoveWindowSubclass(m_hwnd, TabWndProc, 0);
                m_isSubclassed = false;
            }
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }

        m_pages.clear();
        m_curSel = -1;

        if (m_hImageList) {
            ImageList_Destroy(m_hImageList);
            m_hImageList = nullptr;
        }

        if (m_id != -1) {
            releaseId(m_id);
            m_id = -1;
        }
    }

    int zhuziTab::addPage(const zhuziString& text, zhuziControl* pageControl, int imageIndex) {
        return insertPage((int)m_pages.size(), text, pageControl, imageIndex);
    }

    int zhuziTab::insertPage(int index, const zhuziString& text, zhuziControl* pageControl, int imageIndex) {
        if (!pageControl || !m_hwnd) return -1;

        if (!pageControl->getHandle()) {
            if (!pageControl->create(0, 0, 0, 0, WS_CHILD | WS_VISIBLE)) {
                return -1;
            }
        }
        SetParent(pageControl->getHandle(), m_hwnd);

        RECT contentRect = getContentRect();
        int width = contentRect.right - contentRect.left;
        int height = contentRect.bottom - contentRect.top;
        if (width > 0 && height > 0) {
            MoveWindow(pageControl->getHandle(),
                contentRect.left, contentRect.top,
                width, height, FALSE);
        }

        pageControl->show(false);

        if (index < 0 || index >(int)m_pages.size())
            index = (int)m_pages.size();

        TCITEMW tcItem = {};
        tcItem.mask = TCIF_TEXT;
        if (imageIndex >= 0) {
            tcItem.mask |= TCIF_IMAGE;
            tcItem.iImage = imageIndex;
        }
        zhuziString textCopy = text;
        tcItem.pszText = const_cast<wchar_t*>(textCopy.c_str());

        if (SendMessageW(m_hwnd, TCM_INSERTITEMW, index, (LPARAM)&tcItem) == -1)
            return -1;

        TabPageInfo info;
        info.text = text;
        info.control = pageControl;
        info.userData = 0;
        info.imageIndex = imageIndex;
        m_pages.insert(m_pages.begin() + index, info);

        if (m_curSel == -1) {
            setCurSel(index);
        }
        else {
            updateCurrentPageLayout();
        }

        return index;
    }

    void zhuziTab::removePage(int index, bool destroyControl) {
        if (!m_hwnd || index < 0 || index >= (int)m_pages.size()) return;

        if (index == m_curSel) {
            if (m_pages[index].control)
                m_pages[index].control->show(false);
            m_curSel = -1;
        }

        SendMessageW(m_hwnd, TCM_DELETEITEM, index, 0);

        if (destroyControl && m_pages[index].control)
            delete m_pages[index].control;

        m_pages.erase(m_pages.begin() + index);

        if (m_pages.empty()) {
            m_curSel = -1;
        }
        else if (m_curSel >= index) {
            int newSel = (index > 0) ? index - 1 : 0;
            setCurSel(newSel);
        }
        else {
            updateCurrentPageLayout();
        }
    }

    void zhuziTab::clearPages(bool destroyControls) {
        if (!m_hwnd) return;
        for (auto& page : m_pages) {
            if (page.control)
                page.control->show(false);
            if (destroyControls && page.control)
                delete page.control;
        }
        SendMessageW(m_hwnd, TCM_DELETEALLITEMS, 0, 0);
        m_pages.clear();
        m_curSel = -1;
    }

    zhuziString zhuziTab::getPageText(int index) const {
        if (index < 0 || index >= (int)m_pages.size()) return L"";
        return m_pages[index].text;
    }

    void zhuziTab::setPageText(int index, const zhuziString& text) {
        if (!m_hwnd || index < 0 || index >= (int)m_pages.size()) return;
        TCITEMW tcItem = {};
        tcItem.mask = TCIF_TEXT;
        zhuziString textCopy = text;
        tcItem.pszText = const_cast<wchar_t*>(textCopy.c_str());
        SendMessageW(m_hwnd, TCM_SETITEMW, index, (LPARAM)&tcItem);
        m_pages[index].text = text;
    }

    zhuziControl* zhuziTab::getPageControl(int index) const {
        if (index < 0 || index >= (int)m_pages.size()) return nullptr;
        return m_pages[index].control;
    }

    void zhuziTab::setPageControl(int index, zhuziControl* newControl, bool destroyOld) {
        if (!m_hwnd || index < 0 || index >= (int)m_pages.size()) return;
        if (destroyOld && m_pages[index].control)
            delete m_pages[index].control;
        m_pages[index].control = newControl;
        if (newControl) {
            SetParent(newControl->getHandle(), m_hwnd);
            newControl->show(index == m_curSel);
        }
        if (index == m_curSel)
            updateCurrentPageLayout();
    }

    LPARAM zhuziTab::getPageUserData(int index) const {
        if (index < 0 || index >= (int)m_pages.size()) return 0;
        return m_pages[index].userData;
    }

    void zhuziTab::setPageUserData(int index, LPARAM data) {
        if (index < 0 || index >= (int)m_pages.size()) return;
        m_pages[index].userData = data;
    }

    int zhuziTab::getPageImageIndex(int index) const {
        if (index < 0 || index >= (int)m_pages.size()) return -1;
        return m_pages[index].imageIndex;
    }

    void zhuziTab::setPageImageIndex(int index, int imageIndex) {
        if (!m_hwnd || index < 0 || index >= (int)m_pages.size()) return;
        TCITEMW tcItem = {};
        tcItem.mask = TCIF_IMAGE;
        tcItem.iImage = imageIndex;
        SendMessageW(m_hwnd, TCM_SETITEMW, index, (LPARAM)&tcItem);
        m_pages[index].imageIndex = imageIndex;
    }

    int zhuziTab::getCurSel() const {
        if (!m_hwnd) return -1;
        return (int)SendMessageW(m_hwnd, TCM_GETCURSEL, 0, 0);
    }

    bool zhuziTab::setCurSel(int index) {
        if (!m_hwnd || index < 0 || index >= (int)m_pages.size())
            return false;

        if (SendMessageW(m_hwnd, TCM_SETCURSEL, index, 0) == -1)
            return false;

        int oldSel = m_curSel;
        m_curSel = index;
        updatePagesVisibility();
        updateCurrentPageLayout();

        if (m_onSelChange)
            m_onSelChange(oldSel, m_curSel);

        return true;
    }

    int zhuziTab::getPageCount() const {
        return (int)m_pages.size();
    }

    void zhuziTab::setImageList(zhuziImageList iml) {
        if (m_hwnd) {
            // 假设 zhuziImageList 提供了 getHandle() 方法返回 HIMAGELIST
            TabCtrl_SetImageList(m_hwnd, iml.getHandle());
        }
    }

    void zhuziTab::setImageList(HIMAGELIST himl) {
        if (m_hwnd && himl) {
            SendMessageW(m_hwnd, TCM_SETIMAGELIST, 0, (LPARAM)himl);
            if (m_hImageList && m_hImageList != himl)
                ImageList_Destroy(m_hImageList);
            m_hImageList = himl;
        }
    }

    HIMAGELIST zhuziTab::getImageList() const {
        if (m_hwnd)
            return (HIMAGELIST)SendMessageW(m_hwnd, TCM_GETIMAGELIST, 0, 0);
        return m_hImageList;
    }

    void zhuziTab::setOnSelChange(std::function<void(int, int)> callback) {
        m_onSelChange = callback;
    }

    RECT zhuziTab::getContentRect() const {
        RECT rc = { 0 };
        if (m_hwnd) {
            GetClientRect(m_hwnd, &rc);
            TabCtrl_AdjustRect(m_hwnd, FALSE, &rc);
        }
        return rc;
    }

    void zhuziTab::layoutCurrentPage() {
        if (!m_hwnd || m_curSel < 0 || m_curSel >= (int)m_pages.size())
            return;

        zhuziControl* ctrl = m_pages[m_curSel].control;
        if (!ctrl || !ctrl->getHandle())
            return;

        RECT contentRect = getContentRect();
        int width = contentRect.right - contentRect.left;
        int height = contentRect.bottom - contentRect.top;

        MoveWindow(ctrl->getHandle(), contentRect.left, contentRect.top, width, height, TRUE);

        HWND child = GetWindow(ctrl->getHandle(), GW_CHILD);
        while (child) {
            zhuziControl* subCtrl = (zhuziControl*)GetWindowLongPtrW(child, GWLP_USERDATA);
            if (subCtrl) {
                RECT rcClient;
                GetClientRect(ctrl->getHandle(), &rcClient);
                subCtrl->onParentResize(rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
            }
            child = GetWindow(child, GW_HWNDNEXT);
        }
    }

    void zhuziTab::updateCurrentPageLayout() {
        layoutCurrentPage();
    }

    void zhuziTab::updatePagesVisibility() {
        for (int i = 0; i < (int)m_pages.size(); ++i) {
            if (m_pages[i].control) {
                m_pages[i].control->show(i == m_curSel);
            }
        }
    }

    void zhuziTab::onParentResize(int parentWidth, int parentHeight) {
        applyLayout(parentWidth, parentHeight);
        updateCurrentPageLayout();
    }

    LRESULT CALLBACK zhuziTab::TabWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        zhuziTab* pTab = reinterpret_cast<zhuziTab*>(dwRefData);
        if (!pTab || pTab->m_hwnd != hwnd)
            return DefSubclassProc(hwnd, msg, wParam, lParam);

        _CONTAINER_MSGHANDLER_IF
        LRESULT result = pTab->handleSubclassMessage(msg, wParam, lParam);
        if (result != -1)
            return result;

        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    LRESULT zhuziTab::handleSubclassMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        if(msg == WM_SIZE)
        {
            LRESULT res = DefSubclassProc(m_hwnd, msg, wParam, lParam);
            updateCurrentPageLayout();
            return res;
        }
        return -1;
    }
} // namespace zhuzi