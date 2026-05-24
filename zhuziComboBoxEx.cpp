#include "zhuziComboBoxEx.h"
#include "zhuziInstance.h"
#include <commctrl.h>
#include <vector>

#pragma comment(lib, "comctl32.lib")

namespace zhuzi {

    zhuziComboBoxEx::zhuziComboBoxEx(zhuziControl* parent)
        : zhuziControl(parent)
        , m_pImageList(nullptr)
        , m_ownImageList(nullptr)
        , m_onSelChange(nullptr) {
    }

    zhuziComboBoxEx::~zhuziComboBoxEx() {
        destroy();
    }

    bool zhuziComboBoxEx::onCreate(DWORD style) {
        if (m_hwnd) return true;

        INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_USEREX_CLASSES };
        InitCommonControlsEx(&icex);

        if (m_id == -1) {
            try { m_id = allocateId(); }
            catch (...) { return false; }
        }

        HWND hParent = m_parent ? m_parent->getHandle() : nullptr;
        DWORD comboStyle = WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL;
        m_hwnd = CreateWindowExW(0, WC_COMBOBOXEX, L"", comboStyle,
            0, 0, 100, 100,
            hParent, (HMENU)(INT_PTR)m_id,
            zhuziInstance::getHandle(), nullptr);

        if (!m_hwnd) {
            releaseId(m_id);
            m_id = -1;
            return false;
        }

        // 设置项高度
        SendMessage(m_hwnd, CB_SETITEMHEIGHT, 0, 32);
        SendMessage(m_hwnd, CB_SETITEMHEIGHT, -1, 32);

        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

        // 安装子类化过程，用于捕获 CBN_SELCHANGE 消息
        SetWindowSubclass(m_hwnd, ComboBoxExSubclassProc, 0, (DWORD_PTR)this);

        // 应用布局
        if (m_parent) {
            RECT rc;
            GetClientRect(m_parent->getHandle(), &rc);
            applyLayout(rc.right - rc.left, rc.bottom - rc.top);
        }

        return true;
    }

    void zhuziComboBoxEx::destroy() {
        if (m_ownImageList) {
            ImageList_Destroy(m_ownImageList);
            m_ownImageList = nullptr;
        }
        if (m_hwnd) {
            RemoveWindowSubclass(m_hwnd, ComboBoxExSubclassProc, 0);
        }
        zhuziControl::destroy();
    }

    void zhuziComboBoxEx::ensureImageListCreated() {
        if (m_pImageList || m_ownImageList) return;
        m_ownImageList = ImageList_Create(32, 32, ILC_COLOR32 | ILC_MASK, 4, 4);
        if (m_ownImageList && m_hwnd) {
            SendMessage(m_hwnd, CBEM_SETIMAGELIST, 0, (LPARAM)m_ownImageList);
        }
    }

    void zhuziComboBoxEx::setImageList(zhuziImageList* imageList) {
        m_pImageList = imageList;
        if (m_ownImageList) {
            ImageList_Destroy(m_ownImageList);
            m_ownImageList = nullptr;
        }
        if (m_hwnd && m_pImageList && m_pImageList->getHandle()) {
            SendMessage(m_hwnd, CBEM_SETIMAGELIST, 0, (LPARAM)m_pImageList->getHandle());
            int imgHeight = m_pImageList->getHeight();
            if (imgHeight > 0) {
                SendMessage(m_hwnd, CB_SETITEMHEIGHT, 0, imgHeight + 4);
                SendMessage(m_hwnd, CB_SETITEMHEIGHT, -1, imgHeight + 4);
            }
        }
    }

    void zhuziComboBoxEx::updateItemHeight() {
        if (!m_hwnd) return;
        int height = 32;
        if (m_pImageList && m_pImageList->getHeight() > 0)
            height = m_pImageList->getHeight() + 4;
        else if (m_ownImageList) {
            int cx, cy;
            if (ImageList_GetIconSize(m_ownImageList, &cx, &cy))
                height = cy + 4;
        }
        SendMessage(m_hwnd, CB_SETITEMHEIGHT, 0, height);
        SendMessage(m_hwnd, CB_SETITEMHEIGHT, -1, height);
    }

    int zhuziComboBoxEx::addItem(const zhuziString& text, int imageIndex) {
        if (!m_hwnd) return -1;

        if (imageIndex >= 0 && !m_pImageList && !m_ownImageList) {
            ensureImageListCreated();
        }

        int textLen = (int)text.length();
        std::vector<wchar_t> buf(textLen + 1);
        wcscpy_s(buf.data(), buf.size(), text.c_str());

        COMBOBOXEXITEM cbei = { 0 };
        cbei.mask = CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
        cbei.iItem = -1;
        cbei.pszText = buf.data();
        cbei.cchTextMax = textLen + 1;

        HIMAGELIST himl = m_pImageList ? m_pImageList->getHandle() : m_ownImageList;
        if (himl && imageIndex >= 0) {
            cbei.iImage = imageIndex;
            cbei.iSelectedImage = imageIndex;
        }
        else {
            cbei.iImage = I_IMAGECALLBACK;
            cbei.iSelectedImage = I_IMAGECALLBACK;
        }

        LRESULT result = SendMessage(m_hwnd, CBEM_INSERTITEM, 0, (LPARAM)&cbei);
        return (int)result;
    }

    void zhuziComboBoxEx::addItems(const std::vector<zhuziString>& items) {
        for (const auto& s : items) addItem(s, -1);
    }

    bool zhuziComboBoxEx::deleteItem(int index) {
        if (!m_hwnd) return false;
        return SendMessage(m_hwnd, CB_DELETESTRING, index, 0) != CB_ERR;
    }

    void zhuziComboBoxEx::clear() {
        if (m_hwnd) SendMessage(m_hwnd, CB_RESETCONTENT, 0, 0);
    }

    int zhuziComboBoxEx::getSelectedIndex() const {
        if (!m_hwnd) return -1;
        return (int)SendMessage(m_hwnd, CB_GETCURSEL, 0, 0);
    }

    zhuziString zhuziComboBoxEx::getSelectedText() const {
        int idx = getSelectedIndex();
        if (idx == -1) return L"";
        int len = (int)SendMessage(m_hwnd, CB_GETLBTEXTLEN, idx, 0);
        if (len == CB_ERR) return L"";
        zhuziString s(len, L'\0');
        SendMessage(m_hwnd, CB_GETLBTEXT, idx, (LPARAM)s.data());
        return s;
    }

    void zhuziComboBoxEx::setSelectedIndex(int index) {
        if (m_hwnd) SendMessage(m_hwnd, CB_SETCURSEL, index, 0);
    }

    int zhuziComboBoxEx::getCount() const {
        if (!m_hwnd) return 0;
        return (int)SendMessage(m_hwnd, CB_GETCOUNT, 0, 0);
    }

    void zhuziComboBoxEx::setItemData(int index, LPARAM data) {
        if (!m_hwnd || index < 0) return;
        COMBOBOXEXITEM cbei = { 0 };
        cbei.mask = CBEIF_LPARAM;
        cbei.iItem = index;
        cbei.lParam = data;
        SendMessage(m_hwnd, CBEM_SETITEM, 0, (LPARAM)&cbei);
    }

    LPARAM zhuziComboBoxEx::getItemData(int index) const {
        if (!m_hwnd || index < 0) return 0;
        COMBOBOXEXITEM cbei = { 0 };
        cbei.mask = CBEIF_LPARAM;
        cbei.iItem = index;
        SendMessage(m_hwnd, CBEM_GETITEM, 0, (LPARAM)&cbei);
        return cbei.lParam;
    }

    LPARAM zhuziComboBoxEx::getSelectedItemData() const {
        int idx = getSelectedIndex();
        return idx == -1 ? 0 : getItemData(idx);
    }

    void zhuziComboBoxEx::setOnSelChange(std::function<void()> callback) {
        m_onSelChange = callback;
    }

    void zhuziComboBoxEx::onParentResize(int parentWidth, int parentHeight) {
        applyLayout(parentWidth, parentHeight);
        updateItemHeight();
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    // 子类化过程，自动处理 CBN_SELCHANGE
    LRESULT CALLBACK zhuziComboBoxEx::ComboBoxExSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        zhuziComboBoxEx* pThis = reinterpret_cast<zhuziComboBoxEx*>(dwRefData);
        if (!pThis) return DefSubclassProc(hwnd, msg, wParam, lParam);

        if (msg == WM_COMMAND) {
            if (HIWORD(wParam) == CBN_SELCHANGE) {
                if (pThis->m_onSelChange) {
                    pThis->m_onSelChange();
                }
            }
        }
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

} // namespace zhuzi