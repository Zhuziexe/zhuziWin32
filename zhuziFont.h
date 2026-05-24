#pragma once
#include "zhuziString.h"
#include <windows.h>

namespace zhuzi {
    class zhuziControl;  // 前置声明，避免循环包含

    class zhuziFont {
    public:
        zhuziFont(const zhuziString& faceName, int pointSize, bool bold = false, bool italic = false, bool underline = false);
        ~zhuziFont();

        // zhuziFont.h 中添加
        zhuziFont(const zhuziFont& other);
        zhuziFont& operator=(const zhuziFont& other);
        zhuziFont(zhuziFont&& other) noexcept;
        zhuziFont& operator=(zhuziFont&& other) noexcept;

        // 直接操作 HWND 的重载（可内联）
        void applyTo(HWND hwnd) const {
            if (hwnd && m_hFont) SendMessageW(hwnd, WM_SETFONT, (WPARAM)m_hFont, TRUE);
        }

        // 操作 zhuziControl 的重载（定义移至 cpp）
        void applyTo(zhuziControl* ctrl) const;

        HFONT getHandle() const { return m_hFont; }

        const wchar_t* getFontFamily() const { return m_faceName.c_str(); }
        int getSize() const { return m_pointSize; }
        int getStyle() const;  // 返回 GDI+ 样式标志

    private:
        HFONT m_hFont;
        zhuziString m_faceName;
        int m_pointSize;
        bool m_bold;
        bool m_italic;
        bool m_underline;
    };
}