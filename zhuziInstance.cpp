#include "zhuziInstance.h"
#include "zhuziControl.h"
#include <commctrl.h>
#include <gdiplus.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")
//#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace zhuzi {
    zhuziInstance* zhuziInstance::s_pInstance = nullptr;
    int zhuziInstance::s_topLevelWindowCount = 0;

    zhuziInstance::zhuziInstance(HINSTANCE hInstance)
        : m_hInstance(hInstance), m_quitting(false), m_exitCode(0), m_gdiplusToken(0) {
        s_pInstance = this;
        initCommonControls();
        initGdiplus();
        OleInitialize(nullptr);          // 新增
    }

    zhuziInstance::~zhuziInstance() {
        OleUninitialize();               // 新增
        shutdownGdiplus();
        s_pInstance = nullptr;
    }

    int zhuziInstance::run(zhuziWindow& mainWindow) {
        mainWindow.show();
        mainWindow.update();

        MSG msg = {};
        while (!m_quitting && GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return m_exitCode;
    }

    HINSTANCE zhuziInstance::getHandle() {
        return s_pInstance ? s_pInstance->m_hInstance : GetModuleHandle(nullptr);
    }

    void zhuziInstance::quit(int exitCode) {
        m_exitCode = exitCode;
        m_quitting = true;
        PostQuitMessage(exitCode);
    }

    void zhuziInstance::registerTopLevelWindow() {
        s_topLevelWindowCount++;
    }

    void zhuziInstance::unregisterTopLevelWindow() {
        if (--s_topLevelWindowCount == 0 && s_pInstance) {
            s_pInstance->quit(0);
        }
    }

    void zhuziInstance::initCommonControls() {
        INITCOMMONCONTROLSEX icex = {};
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_WIN95_CLASSES | ICC_USEREX_CLASSES | ICC_COOL_CLASSES 
            |ICC_BAR_CLASSES | ICC_TREEVIEW_CLASSES;
        InitCommonControlsEx(&icex);
    }

    void zhuziInstance::initGdiplus() {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);
    }

    void zhuziInstance::shutdownGdiplus() {
        if (m_gdiplusToken) {
            Gdiplus::GdiplusShutdown(m_gdiplusToken);
            m_gdiplusToken = 0;
        }
    }
}