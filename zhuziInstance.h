#pragma once
#include <windows.h>
#include <commctrl.h>

namespace zhuzi {
    class zhuziWindow;

    class zhuziInstance {
    public:
        zhuziInstance(HINSTANCE hInstance);
        ~zhuziInstance();

        int run(zhuziWindow& mainWindow);
        static HINSTANCE getHandle();
        void quit(int exitCode = 0);

        static void registerTopLevelWindow();
        static void unregisterTopLevelWindow();

    private:
        HINSTANCE m_hInstance;
        bool m_quitting;
        int m_exitCode;
        static zhuziInstance* s_pInstance;
        static int s_topLevelWindowCount;

        void initCommonControls();
        void initGdiplus();
        void shutdownGdiplus();

        ULONG_PTR m_gdiplusToken;
    };
}