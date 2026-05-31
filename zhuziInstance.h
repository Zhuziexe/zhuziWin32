#pragma once
#include <windows.h>
#include <functional>

namespace zhuzi {

    class zhuziWindow;

    class zhuziInstance {
    public:
        zhuziInstance(HINSTANCE hInstance);
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

} // namespace zhuzi