#include "zhuziCommctrl.h"
#include "zhuziInstance.h"
#include <windowsx.h>

namespace zhuzi {

    // ==================== zhuziButton ====================
    zhuziButton::zhuziButton(zhuziControl* parent) : zhuziControl(parent) {}
    zhuziButton::~zhuziButton() { destroy(); }

    bool zhuziButton::onCreate(DWORD style) {
        return createControl(L"BUTTON", 0, 0, 0, 0, style | BS_PUSHBUTTON);
    }

    void zhuziButton::setOnClick(std::function<void()> callback) {
        HWND hParent = GetParent(m_hwnd);
        if (!hParent) return;
        zhuziControl* pParent = reinterpret_cast<zhuziControl*>(GetWindowLongPtrW(hParent, GWLP_USERDATA));
        if (!pParent) return;
        Bind(pParent, WM_COMMAND, MAKEWPARAM(m_id, BN_CLICKED), [callback](zhuziMsg* msg) {
            if (callback) callback();
            msg->handled = true;
            });
    }

    // ==================== zhuziLabel ====================
    zhuziLabel::zhuziLabel(zhuziControl* parent)
        : zhuziControl(parent), m_hAlign(AlignHorizontal::Left), m_vAlign(AlignVertical::Top), m_baseStyle(0)
    {
        m_isCustomDraw = 1;
    }

    zhuziLabel::~zhuziLabel() { destroy(); }

    bool zhuziLabel::onCreate(DWORD style) {
        // ���������ʽ��ȥ���������λ��
        m_baseStyle = style | WS_CHILD | WS_VISIBLE | SS_LEFTNOWORDWRAP | WS_CLIPSIBLINGS;
        m_baseStyle &= ~(SS_CENTER | SS_RIGHT | SS_CENTERIMAGE);
        if (!createControl(L"STATIC", 0, 0, 0, 0, m_baseStyle, WS_EX_TRANSPARENT)) return false;
        updateStyle(); // Ӧ�õ�ǰ��������
        return true;
    }

    void zhuziLabel::setTextAlign(AlignHorizontal align) {
        m_hAlign = align;
        if (m_hwnd) updateStyle();
    }

    void zhuziLabel::setVerticalAlign(AlignVertical align) {
        m_vAlign = align;
        if (m_hwnd) updateStyle();
    }

    void zhuziLabel::setSingleLine(bool enable)
    {
        m_singleline = enable;
        if (m_hwnd) updateStyle();
    }

    void zhuziLabel::updateStyle() {
        if (!m_hwnd) return;

        // ��ȡ��ǰ��ʽ���������ˮƽ����λ�ʹ�ֱ���ж���λ
        LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
        style &= ~(SS_LEFT | SS_CENTER | SS_RIGHT | SS_LEFTNOWORDWRAP | SS_CENTERIMAGE | 0x00001000L);

        // ����ˮƽ����
        switch (m_hAlign) {
        case AlignHorizontal::Left:   style |= SS_LEFT; break;
        case AlignHorizontal::Center: style |= SS_CENTER; break;
        case AlignHorizontal::Right:  style |= SS_RIGHT; break;
        }
        if (m_singleline) style |= 0x00001000L;

        // ���ô�ֱ����
        if (m_vAlign == AlignVertical::Center) {
            style |= SS_CENTERIMAGE;
        }

        SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);
        // ǿ���ػ�
        InvalidateRect(m_hwnd, nullptr, TRUE);
        UpdateWindow(m_hwnd);
    }

    void zhuziLabel::onPaint(zhuziPaint& paint) {
        RECT rct;
        GetClientRect(m_hwnd, &rct);
        if (rct.right <= rct.left || rct.bottom <= rct.top) return;

        zhuziString text = getText();
        if (text.empty()) return;

        zhuziBrush brush(RGB(0, 0, 0));
        paint.drawText(getText(), getFont(), brush, rct, ((DWORD)(m_hAlign) |
            ((m_vAlign == AlignVertical::Center ? DT_VCENTER : 0) |
                (m_singleline ? DT_SINGLELINE : 0))));
    }

    // ==================== zhuziListBox ====================
    zhuziListBox::zhuziListBox(zhuziControl* parent) : zhuziControl(parent) {}
    zhuziListBox::~zhuziListBox() { destroy(); }

    bool zhuziListBox::onCreate(DWORD style) {
        return createControl(L"LISTBOX", 0, 0, 0, 0, style | WS_BORDER | WS_VSCROLL | LBS_NOTIFY);
    }

    void zhuziListBox::addItem(const zhuziString& item) {
        SendMessageW(m_hwnd, LB_ADDSTRING, 0, (LPARAM)item.c_str());
    }

    void zhuziListBox::addItems(const std::vector<zhuziString>& items) {
        for (auto& i : items) addItem(i);
    }

    void zhuziListBox::deleteItem(int index) {
        SendMessageW(m_hwnd, LB_DELETESTRING, index, 0);
    }

    void zhuziListBox::clear() {
        SendMessageW(m_hwnd, LB_RESETCONTENT, 0, 0);
    }

    int zhuziListBox::getSelectedIndex() const {
        return (int)SendMessageW(m_hwnd, LB_GETCURSEL, 0, 0);
    }

    zhuziString zhuziListBox::getSelectedText() const {
        int idx = getSelectedIndex();
        return idx == -1 ? L"" : getItemText(idx);
    }

    void zhuziListBox::setSelectedIndex(int index) {
        SendMessageW(m_hwnd, LB_SETCURSEL, index, 0);
    }

    int zhuziListBox::getCount() const {
        return (int)SendMessageW(m_hwnd, LB_GETCOUNT, 0, 0);
    }

    zhuziString zhuziListBox::getItemText(int index) const {
        int len = (int)SendMessageW(m_hwnd, LB_GETTEXTLEN, index, 0);
        if (len == LB_ERR) return L"";
        zhuziString s(len, L'\0');
        SendMessageW(m_hwnd, LB_GETTEXT, index, (LPARAM)&s[0]);
        return s;
    }

    void zhuziListBox::setItemText(int index, const zhuziString& text) {
        if (index >= 0 && index < getCount()) {
            deleteItem(index);
            SendMessageW(m_hwnd, LB_INSERTSTRING, index, (LPARAM)text.c_str());
        }
    }

    void zhuziListBox::setOnSelChange(std::function<void()> callback) {
        HWND hParent = GetParent(m_hwnd);
        if (!hParent) return;
        zhuziControl* pParent = reinterpret_cast<zhuziControl*>(GetWindowLongPtrW(hParent, GWLP_USERDATA));
        if (!pParent) return;
        Bind(pParent, WM_COMMAND, MAKEWPARAM(m_id, LBN_SELCHANGE), [callback](zhuziMsg* msg) {
            if (callback) callback();
            msg->handled = true;
            });
    }

    void zhuziListBox::setOnDoubleClick(std::function<void()> callback) {
        HWND hParent = GetParent(m_hwnd);
        if (!hParent) return;
        zhuziControl* pParent = reinterpret_cast<zhuziControl*>(GetWindowLongPtrW(hParent, GWLP_USERDATA));
        if (!pParent) return;
        Bind(pParent, WM_COMMAND, MAKEWPARAM(m_id, LBN_DBLCLK), [callback](zhuziMsg* msg) {
            if (callback) callback();
            msg->handled = true;
            });
    }

    // ==================== zhuziComboBox ====================
    zhuziComboBox::zhuziComboBox(zhuziControl* parent)
        : zhuziControl(parent), m_isDropdownList(true) {
    }

    zhuziComboBox::~zhuziComboBox() { destroy(); }

    bool zhuziComboBox::onCreate(DWORD style) {
        DWORD comboStyle = style | WS_VSCROLL | CBS_HASSTRINGS;
        comboStyle |= m_isDropdownList ? CBS_DROPDOWNLIST : CBS_DROPDOWN;
        return createControl(L"COMBOBOX", 0, 0, 0, 0, comboStyle);
    }

    void zhuziComboBox::setDropdownList(bool dropdownList) {
        m_isDropdownList = dropdownList;
        // ����Ѿ����������Զ�̬�޸���ʽ�������ṩ�˴��ھ����
        if (m_hwnd) {
            LONG_PTR curStyle = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
            curStyle &= ~(CBS_DROPDOWN | CBS_DROPDOWNLIST);
            curStyle |= dropdownList ? CBS_DROPDOWNLIST : CBS_DROPDOWN;
            SetWindowLongPtrW(m_hwnd, GWL_STYLE, curStyle);
            SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }

    void zhuziComboBox::setEditable(bool editable) {
        setDropdownList(!editable);
    }

    void zhuziComboBox::addItem(const zhuziString& item) {
        SendMessageW(m_hwnd, CB_ADDSTRING, 0, (LPARAM)item.c_str());
    }

    void zhuziComboBox::addItems(const std::vector<zhuziString>& items) {
        for (auto& i : items) addItem(i);
    }

    void zhuziComboBox::deleteItem(int index) {
        SendMessageW(m_hwnd, CB_DELETESTRING, index, 0);
    }

    void zhuziComboBox::clear() {
        SendMessageW(m_hwnd, CB_RESETCONTENT, 0, 0);
    }

    int zhuziComboBox::getSelectedIndex() const {
        return (int)SendMessageW(m_hwnd, CB_GETCURSEL, 0, 0);
    }

    zhuziString zhuziComboBox::getSelectedText() const {
        int idx = getSelectedIndex();
        if (idx == -1) return L"";
        int len = (int)SendMessageW(m_hwnd, CB_GETLBTEXTLEN, idx, 0);
        if (len == CB_ERR) return L"";
        zhuziString s(len, L'\0');
        SendMessageW(m_hwnd, CB_GETLBTEXT, idx, (LPARAM)&s[0]);
        return s;
    }

    void zhuziComboBox::setSelectedIndex(int index) {
        SendMessageW(m_hwnd, CB_SETCURSEL, index, 0);
    }

    int zhuziComboBox::getCount() const {
        return (int)SendMessageW(m_hwnd, CB_GETCOUNT, 0, 0);
    }

    zhuziString zhuziComboBox::getItemText(int index) const {
        int len = (int)SendMessageW(m_hwnd, CB_GETLBTEXTLEN, index, 0);
        if (len == CB_ERR) return L"";
        zhuziString s(len, L'\0');
        SendMessageW(m_hwnd, CB_GETLBTEXT, index, (LPARAM)&s[0]);
        return s;
    }

    void zhuziComboBox::setItemText(int index, const zhuziString& text) {
        if (index >= 0 && index < getCount()) {
            deleteItem(index);
            SendMessageW(m_hwnd, CB_INSERTSTRING, index, (LPARAM)text.c_str());
        }
    }

    void zhuziComboBox::setOnSelChange(std::function<void()> callback) {
        HWND hParent = GetParent(m_hwnd);
        if (!hParent) return;
        zhuziControl* pParent = reinterpret_cast<zhuziControl*>(GetWindowLongPtrW(hParent, GWLP_USERDATA));
        if (!pParent) return;
        Bind(pParent, WM_COMMAND, MAKEWPARAM(m_id, CBN_SELCHANGE), [callback](zhuziMsg* msg) {
            if (callback) callback();
            msg->handled = true;
            });
    }

    void zhuziComboBox::setOnDoubleClick(std::function<void()> callback) {
        HWND hParent = GetParent(m_hwnd);
        if (!hParent) return;
        zhuziControl* pParent = reinterpret_cast<zhuziControl*>(GetWindowLongPtrW(hParent, GWLP_USERDATA));
        if (!pParent) return;
        Bind(pParent, WM_COMMAND, MAKEWPARAM(m_id, CBN_DBLCLK), [callback](zhuziMsg* msg) {
            if (callback) callback();
            msg->handled = true;
            });
    }

    // ==================== zhuziCheckButton ====================
    zhuziCheckButton::zhuziCheckButton(zhuziControl* parent) : zhuziControl(parent) {}
    zhuziCheckButton::~zhuziCheckButton() { destroy(); }

    bool zhuziCheckButton::onCreate(DWORD style) {
        return createControl(L"BUTTON", 0, 0, 0, 0, style | BS_AUTOCHECKBOX);
    }

    void zhuziCheckButton::setChecked(bool checked) {
        SendMessageW(m_hwnd, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
    }

    bool zhuziCheckButton::isChecked() const {
        return SendMessageW(m_hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
    }

    void zhuziCheckButton::setOnCheck(std::function<void(bool)> callback) {
        HWND hParent = GetParent(m_hwnd);
        if (!hParent) return;
        zhuziControl* pParent = reinterpret_cast<zhuziControl*>(GetWindowLongPtrW(hParent, GWLP_USERDATA));
        if (!pParent) return;
        Bind(pParent, WM_COMMAND, MAKEWPARAM(m_id, BN_CLICKED), [this, callback](zhuziMsg* msg) {
            if (callback) callback(isChecked());
            msg->handled = true;
            });
    }

    // ==================== zhuziRadioButton ====================
    zhuziRadioButton::zhuziRadioButton(zhuziControl* parent) : zhuziControl(parent) {}
    zhuziRadioButton::~zhuziRadioButton() { destroy(); }

    bool zhuziRadioButton::onCreate(DWORD style) {
        return createControl(L"BUTTON", 0, 0, 0, 0, style | BS_AUTORADIOBUTTON);
    }

    void zhuziRadioButton::setChecked(bool checked) {
        SendMessageW(m_hwnd, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
    }

    bool zhuziRadioButton::isChecked() const {
        return SendMessageW(m_hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
    }

    void zhuziRadioButton::setOnCheck(std::function<void(bool)> callback) {
        HWND hParent = GetParent(m_hwnd);
        if (!hParent) return;
        zhuziControl* pParent = reinterpret_cast<zhuziControl*>(GetWindowLongPtrW(hParent, GWLP_USERDATA));
        if (!pParent) return;
        Bind(pParent, WM_COMMAND, MAKEWPARAM(m_id, BN_CLICKED), [this, callback](zhuziMsg* msg) {
            if (callback) callback(isChecked());
            msg->handled = true;
            });
    }

    // ==================== zhuziFrame ====================
    static LRESULT CALLBACK FrameWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        zhuziFrame* pFrame = (zhuziFrame*)dwRefData;
        if (!pFrame) return DefSubclassProc(hwnd, msg, wParam, lParam);

        if (msg == WM_CTLCOLORSTATIC) {
            HDC hdc = (HDC)wParam;
            SetBkMode(hdc, TRANSPARENT);
            HBRUSH hBrush = pFrame->getBackgroundBrush();
            if (hBrush) return (LRESULT)hBrush;
            else return (LRESULT)GetStockObject(NULL_BRUSH);
        }
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    zhuziFrame::zhuziFrame(zhuziControl* parent)
        : zhuziControl(parent), m_hBrush(nullptr) {
        m_isCustomDraw = true;
    }

    zhuziFrame::~zhuziFrame() {
        if (m_hBrush) DeleteObject(m_hBrush);
        destroy();
    }

    bool zhuziFrame::onCreate(DWORD style) {
        if (!createControl(L"STATIC", 0, 0, 0, 0, style | SS_NOTIFY))
            return false;
        SetWindowSubclass(m_hwnd, FrameWndProc, 0, (DWORD_PTR)this);
        setBackgroundSysColor(COLOR_WINDOW);
        setBorderStyle(None);
        return true;
    }

    void zhuziFrame::setBackgroundColor(COLORREF color) {
        if (m_hBrush) DeleteObject(m_hBrush);
        m_hBrush = CreateSolidBrush(color);
        if (m_hwnd) {
            InvalidateRect(m_hwnd, nullptr, TRUE);
            UpdateWindow(m_hwnd);
        }
    }

    void zhuziFrame::setBackgroundSysColor(int sysColorIndex) {
        setBackgroundColor(GetSysColor(sysColorIndex));
    }

    void zhuziFrame::setBorderStyle(BorderStyle style) {
        if (!m_hwnd) return;
        LONG_PTR exStyle = GetWindowLongPtrW(m_hwnd, GWL_EXSTYLE);
        exStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
        LONG_PTR dwStyle = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
        if (style == Simple) dwStyle |= WS_BORDER;
        else dwStyle &= ~WS_BORDER;
        if (style == Sunken) exStyle |= WS_EX_CLIENTEDGE;
        else if (style == Raised) exStyle |= WS_EX_STATICEDGE;
        SetWindowLongPtrW(m_hwnd, GWL_STYLE, dwStyle);
        SetWindowLongPtrW(m_hwnd, GWL_EXSTYLE, exStyle);
        SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    void zhuziFrame::enable(bool enabled) {
        if (m_hwnd) {
            EnableWindow(m_hwnd, enabled);
            HWND child = GetWindow(m_hwnd, GW_CHILD);
            while (child) {
                EnableWindow(child, enabled);
                child = GetWindow(child, GW_HWNDNEXT);
            }
        }
    }

    void zhuziFrame::onParentResize(int parentWidth, int parentHeight) {
        if (m_hwnd) {
            applyLayout(parentWidth, parentHeight);
        }
        if (m_hwnd) {
            HWND child = GetWindow(m_hwnd, GW_CHILD);
            while (child) {
                zhuziControl* ctrl = (zhuziControl*)GetWindowLongPtrW(child, GWLP_USERDATA);
                if (ctrl) {
                    RECT rc;
                    GetClientRect(m_hwnd, &rc);
                    ctrl->onParentResize(rc.right - rc.left, rc.bottom - rc.top);
                }
                child = GetWindow(child, GW_HWNDNEXT);
            }
        }
    }

    HBRUSH zhuziFrame::getBackgroundBrush() const {
        return m_hBrush;
    }

    void zhuziFrame::onPaint(zhuziPaint& paint) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        if (m_hBrush) {
            paint.fillRect(0, 0, rc.right - rc.left, rc.bottom - rc.top, zhuziBrush(m_hBrush));
        }
    }

} // namespace zhuzi