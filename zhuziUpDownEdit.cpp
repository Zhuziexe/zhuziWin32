#include "zhuziUpDownEdit.h"
#include "zhuziInstance.h"
#include <windowsx.h>
#include <commctrl.h>
#include <string>
#include <algorithm>

#pragma comment(lib, "comctl32.lib")

namespace zhuzi {

    zhuziUpDownEdit::zhuziUpDownEdit(zhuziControl* parent)
        : zhuziControl(parent)
        , m_hwndEdit(nullptr)
        , m_hwndUpDown(nullptr)
        , m_value(0)
        , m_min(0)
        , m_max(100)
        , m_step(1)
        , m_hexMode(false)
        , m_thousandsSep(false)
        , m_isSubclassed(false)
        , m_onValueChanged(nullptr) {}

    zhuziUpDownEdit::~zhuziUpDownEdit() {
        destroy();
    }

    bool zhuziUpDownEdit::onCreate(DWORD style) {
        return createControls();
    }

    bool zhuziUpDownEdit::createControls() {
        if (m_hwnd) return false;

        HWND hParent = m_parent ? m_parent->getHandle() : nullptr;

        INITCOMMONCONTROLSEX icex = {};
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_UPDOWN_CLASS;
        InitCommonControlsEx(&icex);

        // 创建编辑框（位置大小暂时为0）
        m_hwndEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"0",
            WS_CHILD | WS_VISIBLE | ES_LEFT | ES_NUMBER,
            0, 0, 0, 0,
            hParent,
            (HMENU)(INT_PTR)m_id,
            zhuziInstance::getHandle(),
            nullptr
        );
        if (!m_hwndEdit) return false;

        // 创建 Up-Down 控件
        m_hwndUpDown = CreateWindowW(
            UPDOWN_CLASSW,
            nullptr,
            WS_CHILD | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_SETBUDDYINT,
            0, 0, 0, 0,
            hParent,
            nullptr,
            zhuziInstance::getHandle(),
            nullptr
        );
        if (!m_hwndUpDown) {
            DestroyWindow(m_hwndEdit);
            m_hwndEdit = nullptr;
            return false;
        }

        // 设置伙伴窗口
        SendMessageW(m_hwndUpDown, UDM_SETBUDDY, (WPARAM)m_hwndEdit, 0);
        SendMessageW(m_hwndUpDown, UDM_SETRANGE32, m_min, m_max);
        SendMessageW(m_hwndUpDown, UDM_SETPOS32, 0, m_value);
        const UINT UDM_SETSTEP_MSG = (WM_USER + 102);
        SendMessageW(m_hwndUpDown, UDM_SETSTEP_MSG, (WPARAM)m_step, 0);

        SetWindowLongPtrW(m_hwndEdit, GWLP_USERDATA, (LONG_PTR)this);
        if (!m_isSubclassed) {
            SetWindowSubclass(m_hwndEdit, EditWndProc, 0, (DWORD_PTR)this);
            m_isSubclassed = true;
        }
        updateEditText();
        m_hwnd = m_hwndEdit;   // 主窗口句柄为编辑框

        // 初始布局：立即调用一次onParentResize，让父窗口布局本控件（但父窗口可能还没准备好）
        // 可以延迟到父窗口大小确定后，但为了保证初次显示，可以在create之后由用户显式调用update或等消息。
        // 基类create后会自动调用applyLayout，但这里我们的m_hwnd是编辑框，基类applyLayout会移动编辑框，
        // 而上下控件不会自动移动，所以我们需要在onParentResize中处理。
        // 现在手动调用一次onParentResize，但需要父窗口的客户区大小。
        if (m_parent) {
            RECT rc;
            GetClientRect(m_parent->getHandle(), &rc);
            onParentResize(rc.right - rc.left, rc.bottom - rc.top);
        }
        return true;
    }

    void zhuziUpDownEdit::onParentResize(int parentWidth, int parentHeight) {
        // 先让基类移动编辑框（m_hwnd）
        applyLayout(parentWidth, parentHeight);
        if (!m_hwndEdit || !m_hwndUpDown) return;

        // 获取编辑框的当前位置和大小
        RECT rc;
        GetWindowRect(m_hwndEdit, &rc);
        POINT pt = { rc.left, rc.top };
        ScreenToClient(GetParent(m_hwndEdit), &pt);
        int editWidth = rc.right - rc.left;
        int editHeight = rc.bottom - rc.top;

        int upDownWidth = GetSystemMetrics(SM_CXVSCROLL);
        // 调整编辑框宽度，为上下按钮留出空间
        int newEditWidth = editWidth - upDownWidth;
        if (newEditWidth < 0) newEditWidth = editWidth / 2;
        SetWindowPos(m_hwndEdit, nullptr, pt.x, pt.y, newEditWidth, editHeight, SWP_NOZORDER);
        SetWindowPos(m_hwndUpDown, nullptr, pt.x + newEditWidth, pt.y, upDownWidth, editHeight, SWP_NOZORDER);
    }

    void zhuziUpDownEdit::destroy() {
        if (m_isSubclassed && m_hwndEdit) {
            RemoveWindowSubclass(m_hwndEdit, EditWndProc, 0);
            m_isSubclassed = false;
        }

        if (m_hwndUpDown) {
            DestroyWindow(m_hwndUpDown);
            m_hwndUpDown = nullptr;
        }

        if (m_hwndEdit) {
            DestroyWindow(m_hwndEdit);
            m_hwndEdit = nullptr;
        }

        m_hwnd = nullptr;

        if (m_id != -1) {
            releaseId(m_id);
            m_id = -1;
        }
    }

    void zhuziUpDownEdit::setValue(int value) {
        if (value < m_min) value = m_min;
        if (value > m_max) value = m_max;

        if (m_value != value) {
            m_value = value;
            if (m_hwndUpDown) {
                SendMessageW(m_hwndUpDown, UDM_SETPOS32, 0, m_value);
            }
            updateEditText();

            if (m_onValueChanged) {
                m_onValueChanged(m_value);
            }
        }
    }

    int zhuziUpDownEdit::getValue() const {
        if (m_hwndUpDown) {
            return (int)SendMessageW(m_hwndUpDown, UDM_GETPOS32, 0, 0);
        }
        return m_value;
    }

    void zhuziUpDownEdit::setRange(int min, int max) {
        m_min = min;
        m_max = max;
        if (m_hwndUpDown) {
            SendMessageW(m_hwndUpDown, UDM_SETRANGE32, m_min, m_max);
        }
        setValue(m_value);
    }

    void zhuziUpDownEdit::getRange(int& min, int& max) const {
        min = m_min;
        max = m_max;
    }

    void zhuziUpDownEdit::setStep(int step) {
        m_step = step;
        if (m_hwndUpDown) {
            // 方法1：使用 UDACCEL 结构设置步进
            UDACCEL accel = { 0, (UINT)m_step };
            SendMessageW(m_hwndUpDown, UDM_SETACCEL, 1, (LPARAM)&accel);
        }
    }

    int zhuziUpDownEdit::getStep() const {
        return m_step;
    }

    void zhuziUpDownEdit::setHexMode(bool hex) {
        m_hexMode = hex;

        LONG_PTR style = GetWindowLongPtrW(m_hwndEdit, GWL_STYLE);
        if (m_hexMode) {
            style &= ~ES_NUMBER;
        }
        else {
            style |= ES_NUMBER;
        }
        SetWindowLongPtrW(m_hwndEdit, GWL_STYLE, style);

        updateEditText();
    }

    bool zhuziUpDownEdit::isHexMode() const {
        return m_hexMode;
    }

    void zhuziUpDownEdit::setThousandsSeparator(bool enable) {
        m_thousandsSep = enable;
        updateEditText();
    }

    bool zhuziUpDownEdit::hasThousandsSeparator() const {
        return m_thousandsSep;
    }

    void zhuziUpDownEdit::setReadOnly(bool readOnly) {
        if (m_hwndEdit) {
            SendMessageW(m_hwndEdit, EM_SETREADONLY, readOnly ? TRUE : FALSE, 0);
        }
    }

    void zhuziUpDownEdit::setOnValueChanged(std::function<void(int)> callback) {
        m_onValueChanged = callback;
    }

    void zhuziUpDownEdit::setControlText(const zhuziString& text) {
        std::wstring wstr = text.c_str();
        int value = 0;
        try {
            if (m_hexMode) {
                value = std::stoi(wstr, nullptr, 16);
            }
            else {
                wstr.erase(std::remove(wstr.begin(), wstr.end(), L','), wstr.end());
                if (!wstr.empty()) {
                    value = std::stoi(wstr);
                }
            }
        }
        catch (...) {
            value = m_min;
        }
        setValue(value);
    }

    zhuziString zhuziUpDownEdit::getControlText() const {
        wchar_t buffer[64] = { 0 };
        if (m_hwndEdit) {
            GetWindowTextW(m_hwndEdit, buffer, 64);
        }
        return zhuziString(buffer);
    }

    void zhuziUpDownEdit::enable(bool enabled) {
        if (m_hwndEdit) {
            EnableWindow(m_hwndEdit, enabled);
        }
        if (m_hwndUpDown) {
            EnableWindow(m_hwndUpDown, enabled);
        }
    }

    void zhuziUpDownEdit::updateEditText() {
        if (!m_hwndEdit) return;

        wchar_t buffer[64] = { 0 };
        if (m_hexMode) {
            swprintf_s(buffer, L"%X", m_value);
        }
        else {
            if (m_thousandsSep) {
                wchar_t temp[64];
                swprintf_s(temp, L"%d", m_value);
                std::wstring str(temp);
                int len = (int)str.length();
                for (int i = len - 3; i > 0; i -= 3) {
                    str.insert(i, L",");
                }
                wcscpy_s(buffer, str.c_str());
            }
            else {
                swprintf_s(buffer, L"%d", m_value);
            }
        }
        SetWindowTextW(m_hwndEdit, buffer);
    }

    void zhuziUpDownEdit::updateValueFromEdit() {
        wchar_t buffer[64] = { 0 };
        GetWindowTextW(m_hwndEdit, buffer, 64);

        int newValue = 0;
        try {
            std::wstring text(buffer);
            if (m_hexMode) {
                newValue = std::stoi(text, nullptr, 16);
            }
            else {
                text.erase(std::remove(text.begin(), text.end(), L','), text.end());
                if (text.empty()) text = L"0";
                newValue = std::stoi(text);
            }
        }
        catch (...) {
            newValue = m_min;
        }

        if (newValue < m_min) newValue = m_min;
        if (newValue > m_max) newValue = m_max;

        if (newValue != m_value) {
            setValue(newValue);
        }
    }

    bool zhuziUpDownEdit::isAllowedChar(wchar_t ch) const {
        if (m_hexMode) {
            return (ch >= L'0' && ch <= L'9') ||
                (ch >= L'A' && ch <= L'F') ||
                (ch >= L'a' && ch <= L'f') ||
                ch == L'\b';
        }
        else {
            return (ch >= L'0' && ch <= L'9') || ch == L'\b';
        }
    }

    LRESULT CALLBACK zhuziUpDownEdit::EditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        zhuziUpDownEdit* pThis = reinterpret_cast<zhuziUpDownEdit*>(dwRefData);
        if (!pThis || pThis->m_hwndEdit != hwnd) {
            return DefSubclassProc(hwnd, msg, wParam, lParam);
        }
        return pThis->handleEditMessage(msg, wParam, lParam);
    }

    LRESULT zhuziUpDownEdit::handleEditMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CHAR:
        {
            wchar_t ch = (wchar_t)wParam;
            if (!isAllowedChar(ch) && ch != VK_RETURN && ch != VK_TAB) {
                return 0;
            }
            break;
        }
        case WM_KEYDOWN:
        {
            if (wParam == VK_RETURN) {
                updateValueFromEdit();
                return 0;
            }
            break;
        }
        case WM_KILLFOCUS:
        {
            updateValueFromEdit();
            break;
        }
        }
        return DefSubclassProc(m_hwndEdit, msg, wParam, lParam);
    }

} // namespace zhuzi