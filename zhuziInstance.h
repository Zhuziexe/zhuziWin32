#pragma once
#include <windows.h>
#include <functional>
#include "zhuziString.h"

namespace zhuzi {

    class zhuziWindow;
    class zhuziInstance {
    public:
        zhuziInstance(HINSTANCE hInstance,
            DPI_AWARENESS_CONTEXT dpicontext = DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED);
        ~zhuziInstance();

        // 运行消息循环，返回退出码（由 quit 设置，或最后一个窗口关闭时自动设为 0）
        int run();

        // 退出消息循环，设置退出码（默认为 0）
        void quit(int exitCode = 0);

        static HINSTANCE getHandle();

        static void registerTopLevelWindow(HWND hwnd);
        static void unregisterTopLevelWindow(HWND hwnd);

        bool isRunning() const { return m_running; }

    private:
        HINSTANCE m_hInstance;
        bool m_quitting;
        int m_exitCode;
        bool m_running;
        ULONG_PTR m_gdiplusToken;

        static zhuziInstance* s_pInstance;
        static int s_topLevelWindowCount;

        void initCommonControls();
        void initGdiplus();
        void shutdownGdiplus();
    };

    /**
     * @brief 从RCDATA加载字符串资源
     * @param resourceId 资源ID 
     * @param codePage 资源编码(如CP_UTF8)
     * @return 获取的资源内容  
     * @return 失败时返回空字符串
    */
    zhuziString LoadTextFromRCDATA(int resourceId, UINT codePage = CP_UTF8);
    
} // namespace zhuzi