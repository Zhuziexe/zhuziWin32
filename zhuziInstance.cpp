#include "zhuziInstance.h"
#include <commctrl.h>
#include <gdiplus.h>
#include <objbase.h>
#include <windows.h>
#include <shellscalingapi.h>   // 添加这一行

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "ole32.lib")

namespace zhuzi {

    zhuziInstance* zhuziInstance::s_pInstance = nullptr;
    int zhuziInstance::s_topLevelWindowCount = 0;

    zhuziInstance::zhuziInstance(HINSTANCE hInstance,DPI_AWARENESS_CONTEXT dpicontext)
        : m_hInstance(hInstance), m_quitting(false), m_exitCode(0), m_running(false), m_gdiplusToken(0) {
        s_pInstance = this;
        initCommonControls();
        initGdiplus();
        OleInitialize(nullptr);
        SetProcessDpiAwarenessContext(dpicontext);
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

    zhuziString LoadTextFromRCDATA(int resourceId, UINT codePage) {
        HINSTANCE hInst = zhuziInstance::getHandle();
        if (!hInst) return zhuziString();

        HRSRC hRes = FindResourceW(hInst, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
        
        if (!hRes) return zhuziString();
        

        HGLOBAL hData = LoadResource(hInst, hRes);
        if (!hData) return zhuziString();

        DWORD size = SizeofResource(hInst, hRes);
        if (size == 0) return zhuziString();

        const BYTE* pData = (const BYTE*)LockResource(hData);
        if (!pData) return zhuziString();

        // 处理 UTF-8 BOM（EF BB BF）
        const BYTE* pStart = pData;
        DWORD dataSize = size;
        if (dataSize >= 3 && pData[0] == 0xEF && pData[1] == 0xBB && pData[2] == 0xBF) {
            pStart = pData + 3;
            dataSize = size - 3;
        }

        // 尝试用指定编码转换
        int wideLen = MultiByteToWideChar(codePage, 0, (const char*)pStart, dataSize, nullptr, 0);
        if (wideLen == 0) {
            // 回退到系统默认编码
            wideLen = MultiByteToWideChar(CP_ACP, 0, (const char*)pStart, dataSize, nullptr, 0);
            if (wideLen == 0) {
                return zhuziString(); // 完全失败
            }
            wchar_t* wbuf = new wchar_t[wideLen + 1];
            if (!wbuf) return zhuziString();
            MultiByteToWideChar(CP_ACP, 0, (const char*)pStart, dataSize, wbuf, wideLen);
            wbuf[wideLen] = L'\0';
            zhuziString result(wbuf);
            delete[] wbuf;
            return result;
        }

        wchar_t* wbuf = new wchar_t[wideLen + 1];
        if (!wbuf) return zhuziString();
        int converted = MultiByteToWideChar(codePage, 0, (const char*)pStart, dataSize, wbuf, wideLen);
        if (converted == 0) {
            delete[] wbuf;
            return zhuziString();
        }
        wbuf[wideLen] = L'\0';
        zhuziString result(wbuf);
        delete[] wbuf;
        return result;
    }

} // namespace zhuzi