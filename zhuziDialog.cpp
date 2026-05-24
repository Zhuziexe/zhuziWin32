#include "zhuziDialog.h"
#include "zhuziInstance.h"
#include <commctrl.h>

namespace zhuzi {

    zhuziDialog::zhuziDialog(zhuziControl* parent)
        : m_parent(parent)
        , m_hwnd(nullptr)
        , m_resourceId(0)
        , m_modal(false) {
    }

    zhuziDialog::~zhuziDialog() {
        closeDialog();
    }

    bool zhuziDialog::loadDialog(int resourceId, bool modal) {
        m_resourceId = resourceId;
        m_modal = modal;
        HINSTANCE hInst = zhuziInstance::getHandle();
        HWND hParent = m_parent ? m_parent->getHandle() : nullptr;

        if (modal) {
            // 模态对话框
            INT_PTR result = DialogBoxParamW(hInst, MAKEINTRESOURCEW(resourceId), hParent, DialogProc, (LPARAM)this);
            return result != -1;
        }
        else {
            // 非模态对话框
            m_hwnd = CreateDialogParamW(hInst, MAKEINTRESOURCEW(resourceId), hParent, DialogProc, (LPARAM)this);
            if (m_hwnd) {
                ShowWindow(m_hwnd, SW_SHOW);
                UpdateWindow(m_hwnd);
                centerWindow();
                return true;
            }
            return false;
        }
    }

    void zhuziDialog::closeDialog() {
        if (m_hwnd && !m_modal) {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }
    }

    void zhuziDialog::setTitle(const zhuziString& title) {
        if (m_hwnd && !m_modal) {
            SetWindowTextW(m_hwnd, title.c_str());
        }
    }

    void zhuziDialog::setOnClick(int controlId, std::function<void()> callback) {
        m_clickHandlers[controlId] = callback;
    }

    HWND zhuziDialog::getControlHandle(int controlId) const {
        if (!m_hwnd) return nullptr;
        return GetDlgItem(m_hwnd, controlId);
    }

    void zhuziDialog::setControlText(int controlId, const zhuziString& text) {
        HWND ctrl = getControlHandle(controlId);
        if (ctrl) SetWindowTextW(ctrl, text.c_str());
    }

    zhuziString zhuziDialog::getControlText(int controlId) const {
        HWND ctrl = getControlHandle(controlId);
        if (!ctrl) return L"";
        int len = GetWindowTextLengthW(ctrl);
        zhuziString text(len, L'\0');
        GetWindowTextW(ctrl, &text[0], len + 1);
        return text;
    }

    void zhuziDialog::endDialog(int result) {
        if (m_hwnd && m_modal) {
            EndDialog(m_hwnd, result);
            m_hwnd = nullptr;
        }
    }

    INT_PTR CALLBACK zhuziDialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        zhuziDialog* pThis = nullptr;
        if (msg == WM_INITDIALOG) {
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)lParam);
            pThis = reinterpret_cast<zhuziDialog*>(lParam);
            pThis->m_hwnd = hwnd;
        }
        else {
            pThis = reinterpret_cast<zhuziDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        }
        if (pThis) {
            return pThis->handleMessage(msg, wParam, lParam);
        }
        return FALSE;
    }

    INT_PTR zhuziDialog::handleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_COMMAND:
            onCommand(LOWORD(wParam));
            return TRUE;
        case WM_CLOSE:
            if (m_modal)
                EndDialog(m_hwnd, IDCANCEL);
            else
                DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
            return TRUE;
        }
        return FALSE;
    }

    void zhuziDialog::onCommand(WORD ctrlId) {
        auto it = m_clickHandlers.find(ctrlId);
        if (it != m_clickHandlers.end() && it->second) {
            it->second();
        }
    }

    void zhuziDialog::centerWindow() {
        if (!m_hwnd) return;
        HWND hParent = GetParent(m_hwnd);
        if (!hParent) hParent = GetDesktopWindow();
        RECT rcParent, rcDialog;
        GetWindowRect(hParent, &rcParent);
        GetWindowRect(m_hwnd, &rcDialog);
        int x = (rcParent.left + rcParent.right - (rcDialog.right - rcDialog.left)) / 2;
        int y = (rcParent.top + rcParent.bottom - (rcDialog.bottom - rcDialog.top)) / 2;
        SetWindowPos(m_hwnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

} // namespace zhuzi