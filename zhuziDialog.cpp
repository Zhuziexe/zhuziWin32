#include "zhuziDialog.h"
#include "zhuziInstance.h"
#include "zhuziControl.h"   // 包含 DispatchMessageToControl 声明
#include <windowsx.h>
#include <stdexcept>

namespace zhuzi {

    // ==================== zhuziDialogWindow ====================
    zhuziDialogWindow::zhuziDialogWindow()
        : zhuziControl(nullptr), m_dialog(nullptr) {
        m_isCustomDraw = false;
        m_useD2D = false;
    }

    zhuziDialogWindow::~zhuziDialogWindow() {
        m_hwnd = nullptr;
    }

    void zhuziDialogWindow::attach(HWND hwnd) {
        if (m_hwnd) return;
        m_hwnd = hwnd;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)this);
    }

    bool zhuziDialogWindow::onCreate(DWORD /*style*/) {
        // 容器不创建窗口，由 DialogBox 管理，直接返回 true
        return true;
    }

    void zhuziDialogWindow::onParentResize(int parentWidth, int parentHeight) {
        if (!m_hwnd) return;
        HWND child = GetWindow(m_hwnd, GW_CHILD);
        while (child) {
            zhuziControl* ctrl = (zhuziControl*)GetWindowLongPtrW(child, GWLP_USERDATA);
            if (ctrl) {
                ctrl->onParentResize(parentWidth, parentHeight);
            }
            child = GetWindow(child, GW_HWNDNEXT);
        }
    }

    zhuziDialog* zhuziDialogWindow::getDialog() const {
        return m_dialog;
    }

    void zhuziDialogWindow::setDialog(zhuziDialog* dlg) {
        m_dialog = dlg;
    }

    // ==================== zhuziDialog ====================
    zhuziDialog::zhuziDialog()
        : m_initProc(nullptr), m_result(0), m_ended(false) {
        m_wnd.setDialog(this);
    }

    zhuziDialog::~zhuziDialog() {
        if (!m_ended && m_wnd.getHandle()) {
            ::EndDialog(m_wnd.getHandle(), IDCANCEL);
        }
    }

    void zhuziDialog::setInitProc(std::function<void(zhuziDialog*, zhuziDialogWindow*)> initProc) {
        m_initProc = initProc;
    }

    int zhuziDialog::createModal(zhuziControl* parent, int width, int height,\
        const zhuziString& title, DWORD dwStyle) {
        if (m_ended) {
            m_ended = false;
            m_result = 0;
        }

        HGLOBAL hTemplate = BuildDialogTemplate(width, height, title, dwStyle);
        if (!hTemplate) {
            throw std::runtime_error("Failed to build dialog template");
        }

        HWND hParent = parent ? parent->getHandle() : nullptr;

        INT_PTR ret = DialogBoxIndirectParamW(
            zhuziInstance::getHandle(),
            (LPCDLGTEMPLATE)GlobalLock(hTemplate),
            hParent,          // 父窗口句柄
            DlgProc,
            (LPARAM)this
        );
        GlobalUnlock(hTemplate);
        GlobalFree(hTemplate);

        if (ret == -1) {
            throw std::runtime_error("DialogBoxIndirectParam failed");
        }

        return (int)ret;
    }

    void zhuziDialog::endDialog(int result) {
        if (m_ended) return;
        m_result = result;
        m_ended = true;
        HWND hwnd = m_wnd.getHandle();
        if (hwnd && IsWindow(hwnd)) {
            ::EndDialog(hwnd, result);
        }
    }

    HWND zhuziDialog::getHandle() const {
        return m_wnd.getHandle();
    }

    // ==================== 静态对话框过程 ====================
    INT_PTR CALLBACK zhuziDialog::DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        zhuziDialog* pDlg = nullptr;

        if (msg == WM_INITDIALOG) {
            pDlg = (zhuziDialog*)lParam;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pDlg);
            pDlg->m_wnd.attach(hwnd);
            if (pDlg->m_initProc) {
                pDlg->m_initProc(pDlg, &pDlg->m_wnd);
            }
            return TRUE;
        }

        pDlg = (zhuziDialog*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (!pDlg) {
            return FALSE;
        }

        // 先分发到绑定处理器
        zhuziMsg zmsg{ msg, wParam, lParam, 0, false };
        if (DispatchMessageToControl(hwnd, zmsg)) {
            SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, zmsg.result);
            return TRUE;
        }

        // 默认对话框处理
        switch (msg) {
        case WM_SIZE: {
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            pDlg->m_wnd.onParentResize(w, h);
            return TRUE;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == IDOK || id == IDCANCEL) {
                pDlg->endDialog(id);
                SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, 0);
                return TRUE;
            }
            break;
        }

        case WM_CLOSE: {
            pDlg->endDialog(IDCANCEL);
            return TRUE;
        }
        }

        return FALSE;
    }

    // ==================== 构建对话框模板 ====================
    HGLOBAL zhuziDialog::BuildDialogTemplate(int width, int height,\
        const zhuziString& title, DWORD dlgStyle) {
        DWORD templateSize = sizeof(DLGTEMPLATE);
        templateSize += sizeof(WORD);  // 菜单
        templateSize += sizeof(WORD);  // 类
        int titleLen = (int)title.length();
        templateSize += (titleLen + 1) * sizeof(WCHAR);

        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, templateSize);
        if (!hMem) return nullptr;

        DLGTEMPLATE* pTemplate = (DLGTEMPLATE*)GlobalLock(hMem);
        if (!pTemplate) {
            GlobalFree(hMem);
            return nullptr;
        }

        pTemplate->style = dlgStyle;
        // WS_POPUP | WS_BORDER | WS_SYSMENU |\
        // WS_CAPTION | DS_MODALFRAME | DS_CENTER;
        pTemplate->dwExtendedStyle = 0;
        pTemplate->cdit = 0;
        pTemplate->x = 0;
        pTemplate->y = 0;
        pTemplate->cx = (short)width;
        pTemplate->cy = (short)height;

        WORD* pMenu = (WORD*)(pTemplate + 1);
        *pMenu = 0;
        WORD* pClass = pMenu + 1;
        *pClass = 0;
        WCHAR* pTitle = (WCHAR*)(pClass + 1);
        wcscpy_s(pTitle, titleLen + 1, title.c_str());
        pTitle[titleLen + 1] = L'\0';

        GlobalUnlock(hMem);
        return hMem;
    }

} // namespace zhuzi