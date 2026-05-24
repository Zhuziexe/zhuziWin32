#pragma once
#include "zhuziControl.h"

namespace zhuzi {

    class zhuziGroupBox : public zhuziControl {
    public:
        zhuziGroupBox(zhuziControl* parent = nullptr);
        ~zhuziGroupBox();

        virtual bool onCreate(DWORD style) override;

        void setTextAlignLeft();
        void setTextAlignCenter();
        void setTextAlignRight();

        void setBackgroundColor(COLORREF color);
        void setBackgroundSysColor(int sysColorIndex);
        void clearBackgroundColor();

        void enable(bool enabled = true) override;
        void setText(const zhuziString& text);

    private:
        int         m_textAlign;
        COLORREF    m_bgColor;
        bool        m_hasBgColor;
        HBRUSH      m_hBgBrush;
        zhuziString m_pendingText;

        void applyStyles();
        void updateBackgroundBrush();

        static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    };

}