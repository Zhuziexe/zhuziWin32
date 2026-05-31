#include "CmEdit.h"
#include "zhuziInstance.h"
#include "zhuziControl.h"
#include <windowsx.h>
#include <richedit.h>

#pragma comment(lib, "comctl32.lib")

namespace zhuzi {

    bool CmEdit::s_richeditLoaded = false;

    // 回调辅助函数（保留原有）
    template<typename T>
    inline void setCallbackFunc(std::vector<void*>& arr, int idx, T&& func) {
        if (idx >= (int)arr.size()) return;
        if (arr[idx]) delete reinterpret_cast<T*>(arr[idx]);
        if (func) arr[idx] = new T(std::move(func));
    }

    template<typename T>
    inline T* getCallbackFunc(void* ptr) {
        return ptr ? reinterpret_cast<T*>(ptr) : nullptr;
    }

    // ------------------------------------------------------------------
    CmEdit::CmEdit(zhuziControl* parent)
        : zhuziControl(parent)
        , m_isNumberOnly(0)
        , m_isMultiLine(0)
        , m_isPassword(0)
        , m_isReadOnly(0)
        , m_fontSet(0)
        , m_isSubclassed(0)
        , m_maxLength(0)
        , m_alignment(Alignment::Left)
        , m_bgColor(COLOR_WINDOW)
        , m_textColor(COLOR_WINDOWTEXT)
        , m_hBgBrush(nullptr)
        , m_defaultFont(L"Microsoft YaHei", 12)
        , m_colorHandlerId(-1)
    {
        m_callbacks.resize(EDCB_MAX, nullptr);
        m_isCustomDraw = false;
        ensureRichEditLoaded();
    }

    CmEdit::~CmEdit() {
        for (void* ptr : m_callbacks) delete ptr;
        destroy();
    }

    void CmEdit::ensureRichEditLoaded() {
        if (!s_richeditLoaded) {
            LoadLibraryW(L"Msftedit.dll");
            s_richeditLoaded = true;
        }
    }

    bool CmEdit::recreateWithCurrentSettings() {
        if (!m_hwnd) return true;
        zhuziString oldText = getText();
        auto sel = getSelection();
        bool wasVisible = (GetWindowLongPtrW(m_hwnd, GWL_STYLE) & WS_VISIBLE) != 0;

        HWND oldHwnd = m_hwnd;
        m_hwnd = nullptr;
        DestroyWindow(oldHwnd);
        m_isSubclassed = 0;

        if (!onCreate(0)) return false;
        setText(oldText);
        setSelection(sel.first, sel.second);
        if (wasVisible) ShowWindow(m_hwnd, SW_SHOW);
        return true;
    }

    bool CmEdit::onCreate(DWORD style) {
        DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP;
        if (m_isMultiLine) {
            dwStyle |= ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_WANTRETURN;
        }
        if (m_isReadOnly) dwStyle |= ES_READONLY;
        if (m_isPassword) dwStyle |= ES_PASSWORD;
        if (m_maxLength > 0) dwStyle |= ES_AUTOHSCROLL;

        if (!createControl(L"RichEdit50W", 0, 0, 0, 0, dwStyle | style, 0, false))
            return false;

        if (m_maxLength > 0)
            SendMessageW(m_hwnd, EM_LIMITTEXT, m_maxLength, 0);

        if (m_hBgBrush) DeleteObject(m_hBgBrush);
        m_hBgBrush = CreateSolidBrush(m_bgColor.toCOLORREF());

        if (m_fontSet && m_defaultFont.getHandle()) {
            SendMessageW(m_hwnd, WM_SETFONT, (WPARAM)m_defaultFont.getHandle(), TRUE);
        }

        applyDefaultFormat();
        updateAlignment();

        if (!m_isSubclassed && m_hwnd) {
            SetWindowSubclass(m_hwnd, EditSubclassProc, 0, (DWORD_PTR)this);
            m_isSubclassed = 1;
        }

        // 安装自动颜色处理器
        installAutoColorHandler();

        return true;
    }

    void CmEdit::destroy() {
        // 先注销颜色处理器
        uninstallAutoColorHandler();

        if (m_isSubclassed && m_hwnd) {
            RemoveWindowSubclass(m_hwnd, EditSubclassProc, 0);
            m_isSubclassed = 0;
        }
        if (m_hBgBrush) {
            DeleteObject(m_hBgBrush);
            m_hBgBrush = nullptr;
        }
        zhuziControl::destroy();
    }

    // ---------- 自动颜色处理器 ----------
    void CmEdit::installAutoColorHandler() {
        zhuziWindow* parentWnd = findParentWindow(this);
        if (!parentWnd) return;

        m_colorHandlerId = parentWnd->Bind(WM_CTLCOLOREDIT, [this](zhuziMessage& msg) -> bool {
            HWND hChild = (HWND)msg.lParam;
            if (hChild == m_hwnd) {
                HDC hdc = (HDC)msg.wParam;
                SetTextColor(hdc, m_textColor.toCOLORREF());
                SetBkColor(hdc, m_bgColor.toCOLORREF());
                SetBkMode(hdc, OPAQUE);
                if (m_hBgBrush) {
                    msg.result = (LRESULT)m_hBgBrush;
                    return true;
                }
            }
            return false;
            });
    }

    void CmEdit::uninstallAutoColorHandler() {
        if (m_colorHandlerId != -1) {
            zhuziWindow* parentWnd = findParentWindow(this);
            if (parentWnd) {
                parentWnd->Unbind(m_colorHandlerId);
            }
            m_colorHandlerId = -1;
        }
    }

    // ---------- 基本属性 ----------
    void CmEdit::setReadOnly(bool readOnly) {
        if (m_isReadOnly == readOnly) return;
        m_isReadOnly = readOnly ? 1 : 0;
        if (m_hwnd) SendMessageW(m_hwnd, EM_SETREADONLY, readOnly ? TRUE : FALSE, 0);
    }
    bool CmEdit::isReadOnly() const { return m_isReadOnly != 0; }

    void CmEdit::setPasswordChar(wchar_t ch) {
        m_isPassword = 1;
        if (m_hwnd) {
            SendMessageW(m_hwnd, EM_SETPASSWORDCHAR, (WPARAM)ch, 0);
            InvalidateRect(m_hwnd, nullptr, TRUE);
        }
    }
    void CmEdit::disablePassword() {
        m_isPassword = 0;
        if (m_hwnd) SendMessageW(m_hwnd, EM_SETPASSWORDCHAR, 0, 0);
    }

    void CmEdit::setNumberOnly(bool enable) { m_isNumberOnly = enable ? 1 : 0; }
    bool CmEdit::isNumberOnly() const { return m_isNumberOnly != 0; }

    void CmEdit::setMultiLine(bool enable) {
        if (m_isMultiLine == enable) return;
        m_isMultiLine = enable ? 1 : 0;
        if (m_hwnd) recreateWithCurrentSettings();
    }
    bool CmEdit::isMultiLine() const { return m_isMultiLine != 0; }

    void CmEdit::setMaxLength(int max) {
        m_maxLength = max;
        if (m_hwnd) SendMessageW(m_hwnd, EM_LIMITTEXT, max, 0);
    }
    int CmEdit::getMaxLength() const { return m_maxLength; }

    // ---------- 颜色 ----------
    void CmEdit::setBackgroundColor(const zhuziColor& color) {
        m_bgColor = color;
        if (m_hBgBrush) DeleteObject(m_hBgBrush);
        m_hBgBrush = CreateSolidBrush(color.toCOLORREF());
        if (m_hwnd) {
            InvalidateRect(m_hwnd, nullptr, TRUE);
            // 强制重绘，触发父窗口的 WM_CTLCOLOREDIT
            RedrawWindow(m_hwnd, nullptr, nullptr, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
        }
    }

    void CmEdit::setTextColor(const zhuziColor& color) {
        m_textColor = color;
        applyDefaultFormat();
        if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void CmEdit::resetColors() {
        m_bgColor = zhuziColor(COLOR_WINDOW);
        m_textColor = zhuziColor(COLOR_WINDOWTEXT);
        if (m_hBgBrush) DeleteObject(m_hBgBrush);
        m_hBgBrush = CreateSolidBrush(m_bgColor.toCOLORREF());
        applyDefaultFormat();
        if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    // ---------- 对齐 ----------
    void CmEdit::setAlignment(Alignment align) {
        m_alignment = align;
        updateAlignment();
    }
    CmEdit::Alignment CmEdit::getAlignment() const { return m_alignment; }

    void CmEdit::updateAlignment() {
        if (!m_hwnd) return;
        PARAFORMAT2 pf;
        ZeroMemory(&pf, sizeof(pf));
        pf.cbSize = sizeof(PARAFORMAT2);
        pf.dwMask = PFM_ALIGNMENT;
        switch (m_alignment) {
        case Alignment::Left:   pf.wAlignment = PFA_LEFT; break;
        case Alignment::Center: pf.wAlignment = PFA_CENTER; break;
        case Alignment::Right:  pf.wAlignment = PFA_RIGHT; break;
        }
        SendMessageW(m_hwnd, EM_SETPARAFORMAT, 0, (LPARAM)&pf);
    }

    // ---------- 文本操作 ----------
    void CmEdit::appendText(const zhuziString& text) {
        if (!m_hwnd) return;
        int len = GetWindowTextLengthW(m_hwnd);
        setSelection(len, len);
        replaceSelection(text);
    }
    void CmEdit::clear() { setText(L""); }
    void CmEdit::cut() { SendMessageW(m_hwnd, WM_CUT, 0, 0); }
    void CmEdit::copy() { SendMessageW(m_hwnd, WM_COPY, 0, 0); }
    void CmEdit::paste() { SendMessageW(m_hwnd, WM_PASTE, 0, 0); }
    bool CmEdit::canUndo() const { return SendMessageW(m_hwnd, EM_CANUNDO, 0, 0) != 0; }
    void CmEdit::undo() { SendMessageW(m_hwnd, EM_UNDO, 0, 0); }
    bool CmEdit::canRedo() const { return SendMessageW(m_hwnd, EM_CANREDO, 0, 0) != 0; }
    void CmEdit::redo() { SendMessageW(m_hwnd, EM_REDO, 0, 0); }

    // ---------- 选择操作 ----------
    void CmEdit::setSelection(int start, int end) {
        SendMessageW(m_hwnd, EM_SETSEL, start, end);
    }
    void CmEdit::setSelectionAll() { setSelection(0, -1); }
    void CmEdit::setCursorPos(int pos) { setSelection(pos, pos); }

    std::pair<int, int> CmEdit::getSelection() const {
        DWORD start, end;
        SendMessageW(m_hwnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
        return { (int)start, (int)end };
    }

    zhuziString CmEdit::getSelectedText() const {
        auto sel = getSelection();
        if (sel.first == sel.second) return L"";
        int len = sel.second - sel.first;
        zhuziString buf(len, L'\0');
        SendMessageW(m_hwnd, EM_GETSELTEXT, 0, (LPARAM)buf.data());
        return buf;
    }

    void CmEdit::replaceSelection(const zhuziString& text) {
        SendMessageW(m_hwnd, EM_REPLACESEL, TRUE, (LPARAM)text.c_str());
    }

    // ---------- 指定范围操作 ----------
    void CmEdit::setRangeText(int start, int end, const zhuziString& text) {
        if (!m_hwnd) return;
        auto oldSel = getSelection();
        setSelection(start, end);
        replaceSelection(text);
        setSelection(oldSel.first, oldSel.second);
    }

    void CmEdit::setRangeColor(int start, int end, const zhuziColor& color) {
        if (!m_hwnd) return;
        auto oldSel = getSelection();
        setSelection(start, end);
        setSelectionColor(color);
        setSelection(oldSel.first, oldSel.second);
    }

    void CmEdit::setRangeFont(int start, int end, const zhuziFont& font) {
        if (!m_hwnd) return;
        auto oldSel = getSelection();
        setSelection(start, end);
        setSelectionFont(font);
        setSelection(oldSel.first, oldSel.second);
    }

    // ---------- 多行扩展 ----------
    int CmEdit::getLineCount() const {
        return (int)SendMessageW(m_hwnd, EM_GETLINECOUNT, 0, 0);
    }
    int CmEdit::getLineIndex(int line) const {
        return (int)SendMessageW(m_hwnd, EM_LINEINDEX, line, 0);
    }
    int CmEdit::getLineLength(int line) const {
        return (int)SendMessageW(m_hwnd, EM_LINELENGTH, getLineIndex(line), 0);
    }
    zhuziString CmEdit::getLineText(int line) const {
        int idx = getLineIndex(line);
        if (idx == -1) return L"";
        int len = getLineLength(line);
        if (len <= 0) return L"";
        zhuziString buffer(len + 2, L'\0');
        ((WORD*)buffer.data())[0] = (WORD)(len + 1);
        int copied = (int)SendMessageW(m_hwnd, EM_GETLINE, line, (LPARAM)buffer.data());
        if (copied > 0) buffer.resize(copied);
        else buffer.clear();
        return buffer;
    }
    int CmEdit::getCurrentLine() const {
        int selStart = getSelection().first;
        return (int)SendMessageW(m_hwnd, EM_LINEFROMCHAR, selStart, 0);
    }
    void CmEdit::scrollToLine(int line) {
        SendMessageW(m_hwnd, EM_LINESCROLL, 0, line - getCurrentLine());
    }
    void CmEdit::scrollToCaret() {
        SendMessageW(m_hwnd, EM_SCROLLCARET, 0, 0);
    }

    // ---------- 富文本扩展 ----------
    void CmEdit::setFont(const zhuziFont& font) {
        m_defaultFont = font;
        m_fontSet = 1;
        if (m_hwnd && font.getHandle()) {
            SendMessageW(m_hwnd, WM_SETFONT, (WPARAM)font.getHandle(), TRUE);
            applyDefaultFormat();
        }
    }

    void CmEdit::setSelectionFont(const zhuziFont& font) {
        if (!m_hwnd || !font.getHandle()) return;
        CHARFORMAT2W cf;
        ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(CHARFORMAT2W);
        cf.dwMask = CFM_FACE | CFM_SIZE | CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;
        cf.dwEffects = 0;
        if (font.isbold())   cf.dwEffects |= CFE_BOLD;
        if (font.isitalic()) cf.dwEffects |= CFE_ITALIC;
        if (font.isunderline()) cf.dwEffects |= CFE_UNDERLINE;
        cf.yHeight = font.getSize() * 20;
        wcscpy_s(cf.szFaceName, font.getFontFamily());
        SendMessageW(m_hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    }

    void CmEdit::setSelectionColor(const zhuziColor& color) {
        if (!m_hwnd) return;
        CHARFORMAT2W cf;
        ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(CHARFORMAT2W);
        cf.dwMask = CFM_COLOR;
        cf.crTextColor = color.toCOLORREF();
        SendMessageW(m_hwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
    }

    void CmEdit::applyDefaultFormat() {
        if (!m_hwnd) return;
        CHARFORMAT2W cf;
        ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(CHARFORMAT2W);
        cf.dwMask = CFM_COLOR | CFM_FACE | CFM_SIZE;
        cf.crTextColor = m_textColor.toCOLORREF();
        cf.yHeight = m_defaultFont.getSize() * 20;
        wcscpy_s(cf.szFaceName, m_defaultFont.getFontFamily());
        SendMessageW(m_hwnd, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM)&cf);
        SendMessageW(m_hwnd, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    }

    // ---------- 事件回调 ----------
    void CmEdit::setOnTextChanged(std::function<void()> callback) {
        setCallbackFunc(m_callbacks, EDCB_TEXTCHANGED, std::move(callback));
    }
    void CmEdit::setOnReturnPressed(std::function<void()> callback) {
        setCallbackFunc(m_callbacks, EDCB_RETURNPRESSED, std::move(callback));
    }
    void CmEdit::setOnFocus(std::function<void(bool)> callback) {
        setCallbackFunc(m_callbacks, EDCB_FOCUS, std::move(callback));
    }
    void CmEdit::setOnCharInput(std::function<bool(wchar_t)> callback) {
        setCallbackFunc(m_callbacks, EDCB_CHARINPUT, std::move(callback));
    }

    // ---------- 子类化消息处理 ----------
    LRESULT CALLBACK CmEdit::EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        CmEdit* pThis = reinterpret_cast<CmEdit*>(dwRefData);
        if (!pThis || pThis->m_hwnd != hwnd)
            return DefSubclassProc(hwnd, msg, wParam, lParam);
        return pThis->handleSubclassMessage(msg, wParam, lParam);
    }

    LRESULT CmEdit::handleSubclassMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_SETFOCUS:
            if (auto* cb = getCallbackFunc<std::function<void(bool)>>(m_callbacks[EDCB_FOCUS]))
                (*cb)(true);
            break;
        case WM_KILLFOCUS:
            if (auto* cb = getCallbackFunc<std::function<void(bool)>>(m_callbacks[EDCB_FOCUS]))
                (*cb)(false);
            break;
        case WM_CHAR:
            if (m_isNumberOnly) {
                wchar_t ch = (wchar_t)wParam;
                if (ch >= L'0' && ch <= L'9') break;
                if (ch == L'\b' || ch == L'\r' || ch == 27) break;
                return 0;
            }
            if (auto* cb = getCallbackFunc<std::function<bool(wchar_t)>>(m_callbacks[EDCB_CHARINPUT])) {
                wchar_t ch = (wchar_t)wParam;
                if (!(*cb)(ch)) return 0;
            }
            if (wParam == VK_RETURN) {
                bool isCtrlPressed = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                bool isSingleLine = !m_isMultiLine;
                if (isSingleLine || (m_isMultiLine && isCtrlPressed)) {
                    if (auto* cb = getCallbackFunc<std::function<void()>>(m_callbacks[EDCB_RETURNPRESSED]))
                        (*cb)();
                    return 0;
                }
            }
            break;
        case WM_KEYDOWN:
            if (m_isMultiLine && wParam == VK_RETURN && (GetKeyState(VK_CONTROL) & 0x8000)) {
                if (auto* cb = getCallbackFunc<std::function<void()>>(m_callbacks[EDCB_RETURNPRESSED]))
                    (*cb)();
                return 0;
            }
            break;
        case WM_COMMAND:
            if (HIWORD(wParam) == EN_CHANGE) {
                if (auto* cb = getCallbackFunc<std::function<void()>>(m_callbacks[EDCB_TEXTCHANGED]))
                    (*cb)();
            }
            break;
        }
        return DefSubclassProc(m_hwnd, msg, wParam, lParam);
    }

    // ---------- RTF 文件流回调 ----------
    static DWORD CALLBACK StreamInCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb) {
        HANDLE hFile = reinterpret_cast<HANDLE>(dwCookie);
        if (ReadFile(hFile, pbBuff, cb, reinterpret_cast<DWORD*>(pcb), nullptr)) {
            return 0;
        }
        return 1;  // 错误
    }

    static DWORD CALLBACK StreamOutCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb) {
        HANDLE hFile = reinterpret_cast<HANDLE>(dwCookie);
        if (WriteFile(hFile, pbBuff, cb, reinterpret_cast<DWORD*>(pcb), nullptr)) {
            return 0;
        }
        return 1;  // 错误
    }

    bool CmEdit::loadRTF(const zhuziString& filePath) {
        if (!m_hwnd) return false;

        HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        EDITSTREAM es = { 0 };
        es.dwCookie = reinterpret_cast<DWORD_PTR>(hFile);
        es.pfnCallback = StreamInCallback;

        // 发送 EM_STREAMIN 消息，使用 SF_RTF 标志
        LRESULT result = SendMessageW(m_hwnd, EM_STREAMIN, SF_RTF, reinterpret_cast<LPARAM>(&es));
        CloseHandle(hFile);

        // 强制重绘
        InvalidateRect(m_hwnd, nullptr, TRUE);
        return (result != 0);
    }

    bool CmEdit::saveRTF(const zhuziString& filePath) {
        if (!m_hwnd) return false;

        HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_WRITE, 0,
            nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        EDITSTREAM es = { 0 };
        es.dwCookie = reinterpret_cast<DWORD_PTR>(hFile);
        es.pfnCallback = StreamOutCallback;

        LRESULT result = SendMessageW(m_hwnd, EM_STREAMOUT, SF_RTF, reinterpret_cast<LPARAM>(&es));
        CloseHandle(hFile);

        return (result != 0);
    }

    // 辅助：将 CHARFORMAT2W 转换为 zhuziFont
    static zhuziFont CharFormatToFont(const CHARFORMAT2W& cf) {
        zhuziString faceName = cf.szFaceName;
        int pointSize = cf.yHeight / 20;   // twips 转 point
        if (pointSize <= 0) pointSize = 12;
        bool bold = (cf.dwEffects & CFE_BOLD) != 0;
        bool italic = (cf.dwEffects & CFE_ITALIC) != 0;
        bool underline = (cf.dwEffects & CFE_UNDERLINE) != 0;
        return zhuziFont(faceName, pointSize, bold, italic, underline);
    }

    // 辅助：将 CHARFORMAT2W 转换为 zhuziColor
    static zhuziColor CharFormatToColor(const CHARFORMAT2W& cf) {
        if (cf.dwMask & CFM_COLOR) {
            return zhuziColor(cf.crTextColor);
        }
        return zhuziColor(0, 0, 0);
    }

    zhuziFont CmEdit::getSelectionFont() const {
        if (!m_hwnd) return m_defaultFont;
        CHARFORMAT2W cf;
        ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(CHARFORMAT2W);
        cf.dwMask = CFM_FACE | CFM_SIZE | CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;
        SendMessageW(m_hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
        if (cf.dwMask & (CFM_FACE | CFM_SIZE)) {
            return CharFormatToFont(cf);
        }
        return m_defaultFont;
    }

    zhuziFont CmEdit::getRangeFont(int start, int end) const {
        if (!m_hwnd || start < 0 || end <= start) return m_defaultFont;
        auto oldSel = getSelection();
        const_cast<CmEdit*>(this)->setSelection(start, end);
        zhuziFont result = getSelectionFont();
        const_cast<CmEdit*>(this)->setSelection(oldSel.first, oldSel.second);
        return result;
    }

    zhuziColor CmEdit::getSelectionColor() const {
        if (!m_hwnd) return zhuziColor(0, 0, 0);
        CHARFORMAT2W cf;
        ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(CHARFORMAT2W);
        cf.dwMask = CFM_COLOR;
        SendMessageW(m_hwnd, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
        return CharFormatToColor(cf);
    }

    zhuziColor CmEdit::getRangeColor(int start, int end) const {
        if (!m_hwnd || start < 0 || end <= start) return zhuziColor(0, 0, 0);
        auto oldSel = getSelection();
        const_cast<CmEdit*>(this)->setSelection(start, end);
        zhuziColor result = getSelectionColor();
        const_cast<CmEdit*>(this)->setSelection(oldSel.first, oldSel.second);
        return result;
    }

} // namespace zhuzi