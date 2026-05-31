#include "zhuziInstance.h"
#include <commctrl.h>
#include <gdiplus.h>
#include <objbase.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")

namespace zhuzi {

    zhuziInstance* zhuziInstance::s_pInstance = nullptr;
    int zhuziInstance::s_topLevelWindowCount = 0;

    zhuziInstance::zhuziInstance(HINSTANCE hInstance)
        : m_hInstance(hInstance), m_quitting(false), m_exitCode(0), m_running(false), m_gdiplusToken(0) {
        s_pInstance = this;
        initCommonControls();
        initGdiplus();
        OleInitialize(nullptr);
    }

    zhuziInstance::~zhuziInstance() {
        OleUninitialize();
        shutdownGdiplus();
        s_pInstance = nullptr;
    }

    int zhuziInstance::run() {
        if (m_running) return m_exitCode; // 防止重入，直接返回上次的退出码
        m_running = true;
        m_quitting = false;
        m_exitCode = 0;  // 重置退出码

        MSG msg;
        while (!m_quitting) {
            BOOL bRet = GetMessage(&msg, nullptr, 0, 0);
            if (bRet == -1) {
                // 错误，退出
                break;
            }
            if (bRet == 0) {
                // WM_QUIT 收到，退出
                m_quitting = true;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        m_running = false;
        return m_exitCode;
    }

    void zhuziInstance::quit(int exitCode) {
        m_exitCode = exitCode;
        m_quitting = true;
        PostQuitMessage(exitCode);
    }

    HINSTANCE zhuziInstance::getHandle() {
        return s_pInstance ? s_pInstance->m_hInstance : GetModuleHandle(nullptr);
    }

    void zhuziInstance::registerTopLevelWindow(HWND /*hwnd*/) {
        s_topLevelWindowCount++;
    }

    void zhuziInstance::unregisterTopLevelWindow(HWND /*hwnd*/) {
        if (s_topLevelWindowCount > 0) {
            if (--s_topLevelWindowCount == 0 && s_pInstance && s_pInstance->m_running) {
                s_pInstance->quit(0);
            }
        }
    }

    void zhuziInstance::initCommonControls() {
        INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX),
            ICC_WIN95_CLASSES | ICC_USEREX_CLASSES | ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_TREEVIEW_CLASSES };
        InitCommonControlsEx(&icex);
    }

    void zhuziInstance::initGdiplus() {
        Gdiplus::GdiplusStartupInput input;
        Gdiplus::GdiplusStartup(&m_gdiplusToken, &input, nullptr);
    }

    void zhuziInstance::shutdownGdiplus() {
        if (m_gdiplusToken) {
            Gdiplus::GdiplusShutdown(m_gdiplusToken);
            m_gdiplusToken = 0;
        }
    }

} // namespace zhuzi