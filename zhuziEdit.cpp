#include "zhuziEdit.h"
#include "zhuziInstance.h"
#include <windowsx.h>

namespace zhuzi {
    std::shared_ptr<zhuziFont> zhuziEdit::font_tahoma = nullptr;

    // ==================== zhuziEdit ====================
    zhuziEdit::zhuziEdit(zhuziControl* parent)
        : zhuziControl(parent), m_onEnter(nullptr), m_isPassword(false), m_isOnlyNumber(false) {
        if (!font_tahoma) font_tahoma = std::make_shared<zhuziFont>(L"Tahoma", 16);
    }
    zhuziEdit::~zhuziEdit() { destroy(); }

    bool zhuziEdit::onCreate(DWORD style) {
        DWORD finalStyle = style | WS_BORDER | ES_LEFT | ES_AUTOHSCROLL;
        if (m_isPassword) finalStyle |= ES_PASSWORD;
        if (m_isOnlyNumber) finalStyle |= ES_NUMBER;
        if (!createControl(L"EDIT", 0, 0, 0, 0, finalStyle)) return false;
        SetWindowSubclass(m_hwnd, EditProc, 0, (DWORD_PTR)this);
        if (m_isPassword) setFont(*font_tahoma);
        return true;
    }

    void zhuziEdit::setText(const zhuziString& text) { if (m_hwnd) SetWindowTextW(m_hwnd, text.c_str()); }
    zhuziString zhuziEdit::getText() const {
        if (!m_hwnd) return L"";
        int len = GetWindowTextLengthW(m_hwnd);
        zhuziString s(len, L'\0');
        GetWindowTextW(m_hwnd, &s[0], len + 1);
        return s;
    }

    void zhuziEdit::setPassword(bool enable) {
        if (m_isPassword == enable) return;
        m_isPassword = enable;
        if (!m_hwnd) return;
        SendMessageW(m_hwnd, EM_SETPASSWORDCHAR, enable ? L'¡ñ' : 0, 0);
        LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
        enable ? style |= ES_PASSWORD : style &= ~ES_PASSWORD;
        SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);
        setFont(*font_tahoma);
        InvalidateRect(m_hwnd, nullptr, TRUE);
        UpdateWindow(m_hwnd);
    }

    void zhuziEdit::setOnlyNumber(bool enable) {
        if (m_isOnlyNumber == enable) return;
        m_isOnlyNumber = enable;
        if (!m_hwnd) return;
        LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
        enable ? style |= ES_NUMBER : style &= ~ES_NUMBER;
        SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void zhuziEdit::setOnEnter(std::function<void()> callback) { m_onEnter = callback; }
    void zhuziEdit::onEnter() { if (m_onEnter) m_onEnter(); }

    LRESULT CALLBACK zhuziEdit::EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        zhuziEdit* pThis = (zhuziEdit*)dwRefData;
        if (!pThis) return DefSubclassProc(hwnd, msg, wParam, lParam);
        if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
            if (pThis->m_onEnter) {
                pThis->onEnter();
                return 0;
            }
        }
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    // ==================== zhuziText ====================
    zhuziText::zhuziText(zhuziControl* parent) : zhuziControl(parent), m_onTextChange(nullptr) {}
    zhuziText::~zhuziText() { destroy(); }

    bool zhuziText::onCreate(DWORD style) {
        DWORD editStyle = style | ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL | ES_AUTOVSCROLL | WS_BORDER;
        if (!createControl(L"EDIT", 0, 0, 0, 0, editStyle)) return false;
        SetWindowSubclass(m_hwnd, TextProc, 0, (DWORD_PTR)this);
        return true;
    }

    void zhuziText::setText(const zhuziString& text) { if (m_hwnd) SetWindowTextW(m_hwnd, text.c_str()); }
    zhuziString zhuziText::getText() const {
        if (!m_hwnd) return L"";
        int len = GetWindowTextLengthW(m_hwnd);
        zhuziString s(len, L'\0');
        GetWindowTextW(m_hwnd, &s[0], len + 1);
        return s;
    }
    void zhuziText::appendText(const zhuziString& text) {
        if (!m_hwnd) return;
        zhuziString old = getText();
        setText(old + text);
    }
    void zhuziText::clear() { if (m_hwnd) SetWindowTextW(m_hwnd, L""); }
    void zhuziText::scrollToTop() { if (m_hwnd) SendMessageW(m_hwnd, EM_SETSEL, 0, 0); }
    void zhuziText::scrollToBottom() {
        if (m_hwnd) {
            int len = GetWindowTextLengthW(m_hwnd);
            SendMessageW(m_hwnd, EM_SETSEL, len, len);
            SendMessageW(m_hwnd, EM_SCROLLCARET, 0, 0);
        }
    }
    void zhuziText::scrollToLine(int lineIndex) {
        if (m_hwnd) {
            int idx = (int)SendMessageW(m_hwnd, EM_LINEINDEX, lineIndex, 0);
            if (idx >= 0) { SendMessageW(m_hwnd, EM_SETSEL, idx, idx); SendMessageW(m_hwnd, EM_SCROLLCARET, 0, 0); }
        }
    }
    void zhuziText::selectAll() { if (m_hwnd) SendMessageW(m_hwnd, EM_SETSEL, 0, -1); }
    void zhuziText::setSelection(int start, int end) { if (m_hwnd) SendMessageW(m_hwnd, EM_SETSEL, start, end); }
    void zhuziText::getSelection(int& start, int& end) const {
        if (m_hwnd) {
            DWORD sel = (DWORD)SendMessageW(m_hwnd, EM_GETSEL, 0, 0);
            start = LOWORD(sel); end = HIWORD(sel);
        }
        else start = end = 0;
    }
    void zhuziText::setReadOnly(bool readOnly) { if (m_hwnd) SendMessageW(m_hwnd, EM_SETREADONLY, readOnly ? TRUE : FALSE, 0); }
    bool zhuziText::isReadOnly() const { return m_hwnd ? (GetWindowLongPtrW(m_hwnd, GWL_STYLE) & ES_READONLY) != 0 : false; }
    void zhuziText::setOnTextChange(std::function<void()> callback) { m_onTextChange = callback; }
    void zhuziText::onTextChanged() { if (m_onTextChange) m_onTextChange(); }

    LRESULT CALLBACK zhuziText::TextProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        zhuziText* pThis = (zhuziText*)dwRefData;
        if (!pThis) return DefSubclassProc(hwnd, msg, wParam, lParam);
        if (msg == WM_COMMAND && HIWORD(wParam) == EN_CHANGE) {
            pThis->onTextChanged();
            return 0;
        }
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

} // namespace zhuzi