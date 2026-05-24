#include "zhuziToolTip.h"
#include "zhuziInstance.h"
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

#ifndef TTS_BALLOON
#define TTS_BALLOON 0x0040
#endif

namespace zhuzi {

    zhuziToolTip::zhuziToolTip(zhuziControl* parent)
        : zhuziControl(parent), m_hToolTip(nullptr) {
        m_flag1 = false;
        m_flag2 = 0;
    }

    zhuziToolTip::~zhuziToolTip() {
        if (m_hToolTip) DestroyWindow(m_hToolTip);
    }

    bool zhuziToolTip::create() {
        return zhuziControl::create(0, 0, 0, 0, WS_CHILD | WS_VISIBLE);
    }

    bool zhuziToolTip::onCreate(DWORD style) {
        DWORD dwStyle = WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP;
        if (m_flag1) dwStyle |= TTS_BALLOON;
        HWND hParent = m_parent ? m_parent->getHandle() : nullptr;
        m_hToolTip = CreateWindowExW(0, TOOLTIPS_CLASSW, nullptr, dwStyle,
            0, 0, 0, 0, hParent, nullptr,
            zhuziInstance::getHandle(), nullptr);
        if (!m_hToolTip) return false;

        SendMessage(m_hToolTip, TTM_SETMAXTIPWIDTH, 0, 300);
        SendMessage(m_hToolTip, TTM_SETDELAYTIME, TTDT_INITIAL, 500);
        SendMessage(m_hToolTip, TTM_SETDELAYTIME, TTDT_RESHOW, 100);
        SendMessage(m_hToolTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 3000);

        if (m_flag2 != 0) {
            SendMessage(m_hToolTip, TTM_SETTITLE, (WPARAM)m_flag2, (LPARAM)m_title.c_str());
        }
        return true;
    }

    void zhuziToolTip::addTool(HWND hwndTool, const zhuziString& text, bool bSubclass) {
        if (!m_hToolTip || !hwndTool || !IsWindow(hwndTool)) return;
        HWND hParent = GetParent(hwndTool);
        if (!hParent) hParent = hwndTool;
        TOOLINFOW ti = { 0 };
        ti.cbSize = sizeof(TOOLINFOW);
        ti.hwnd = hParent;
        ti.uId = (UINT_PTR)hwndTool;
        ti.uFlags = TTF_IDISHWND;
        if (bSubclass) ti.uFlags |= TTF_SUBCLASS;
        ti.lpszText = const_cast<LPWSTR>(text.c_str());
        SendMessage(m_hToolTip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
    }

    void zhuziToolTip::addTool(zhuziControl* ctrl, const zhuziString& text, bool bSubclass) {
        if (ctrl) addTool(ctrl->getHandle(), text, bSubclass);
    }

    void zhuziToolTip::delTool(HWND hwndTool) {
        if (!m_hToolTip || !hwndTool) return;
        HWND hParent = GetParent(hwndTool);
        if (!hParent) hParent = hwndTool;
        TOOLINFOW ti = { 0 };
        ti.cbSize = sizeof(TOOLINFOW);
        ti.hwnd = hParent;
        ti.uId = (UINT_PTR)hwndTool;
        SendMessage(m_hToolTip, TTM_DELTOOLW, 0, (LPARAM)&ti);
    }

    void zhuziToolTip::delTool(zhuziControl* ctrl) {
        if (ctrl) delTool(ctrl->getHandle());
    }

    void zhuziToolTip::updateToolText(HWND hwndTool, const zhuziString& text) {
        if (!m_hToolTip || !hwndTool) return;
        HWND hParent = GetParent(hwndTool);
        if (!hParent) hParent = hwndTool;
        TOOLINFOW ti = { 0 };
        ti.cbSize = sizeof(TOOLINFOW);
        ti.hwnd = hParent;
        ti.uId = (UINT_PTR)hwndTool;
        ti.lpszText = const_cast<LPWSTR>(text.c_str());
        SendMessage(m_hToolTip, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);
    }

    void zhuziToolTip::updateToolText(zhuziControl* ctrl, const zhuziString& text) {
        if (ctrl) updateToolText(ctrl->getHandle(), text);
    }

    void zhuziToolTip::setDelayTime(int initial, int reshow, int autoPop) {
        if (!m_hToolTip) return;
        SendMessage(m_hToolTip, TTM_SETDELAYTIME, TTDT_INITIAL, initial);
        SendMessage(m_hToolTip, TTM_SETDELAYTIME, TTDT_RESHOW, reshow);
        SendMessage(m_hToolTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, autoPop);
    }

    void zhuziToolTip::setMaxTipWidth(int width) {
        if (m_hToolTip) SendMessage(m_hToolTip, TTM_SETMAXTIPWIDTH, 0, width);
    }

    void zhuziToolTip::setBalloon(bool enable) {
        m_flag1 = enable;
        if (m_hToolTip) {
            LONG_PTR style = GetWindowLongPtr(m_hToolTip, GWL_STYLE);
            if (enable)
                style |= TTS_BALLOON;
            else
                style &= ~TTS_BALLOON;
            SetWindowLongPtr(m_hToolTip, GWL_STYLE, style);
            SetWindowPos(m_hToolTip, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }

    void zhuziToolTip::setIcon(int iconType, const zhuziString& title) {
        m_flag2 = iconType;
        m_title = title;
        if (m_hToolTip) {
            SendMessage(m_hToolTip, TTM_SETTITLE, (WPARAM)m_flag2, (LPARAM)m_title.c_str());
        }
    }

    void zhuziToolTip::activate(bool active) {
        if (m_hToolTip) SendMessage(m_hToolTip, TTM_ACTIVATE, active ? TRUE : FALSE, 0);
    }

} // namespace zhuzi