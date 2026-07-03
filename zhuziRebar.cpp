// zhuziRebar.cpp
#include "zhuziRebar.h"
#include "zhuziInstance.h"
#include "zhuziCommctrl.h"
#include <windowsx.h>

#pragma comment(lib, "comctl32.lib")

namespace zhuzi {

    zhuziRebar::zhuziRebar(zhuziControl* parent)
        : zhuziControl(parent), m_hBackgroundBrush(nullptr), m_bUseCustomBg(false)
    {
    }

    zhuziRebar::~zhuziRebar() {
        if (m_hBackgroundBrush) DeleteObject(m_hBackgroundBrush);
        destroy();
    }

    bool zhuziRebar::onCreate(DWORD style) {
        DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | CCS_TOP | RBS_VARHEIGHT | RBS_BANDBORDERS;
        if (style) dwStyle |= style;
        if (!createControl(REBARCLASSNAME, 0, 0, 0, 0, dwStyle, WS_EX_CLIENTEDGE, true))
            return false;
        SetWindowSubclass(m_hwnd, RebarSubclassProc, 0, (DWORD_PTR)this);
        // 主动进行一次布局，避免高度为0
        if (m_parent) {
            RECT rc;
            GetClientRect(m_parent->getHandle(), &rc);
            onParentResize(rc.right - rc.left, rc.bottom - rc.top);
        }
        return true;
    }

    int zhuziRebar::AddBand(const RebarBandInfo& bandInfo, bool bRedraw) {
        REBARBANDINFOW rbbi = { 0 };
        rbbi.cbSize = sizeof(REBARBANDINFOW);

        if (bandInfo.hwndChild && IsWindow(bandInfo.hwndChild)) {
            rbbi.hwndChild = bandInfo.hwndChild;
            rbbi.fMask = RBBIM_CHILD | RBBIM_STYLE | RBBIM_SIZE;
            if (bandInfo.cx > 0) {
                rbbi.cx = bandInfo.cx;
                rbbi.fMask |= RBBIM_SIZE;
            }
            else {
                RECT rc;
                GetWindowRect(bandInfo.hwndChild, &rc);
                rbbi.cx = rc.right - rc.left;
                rbbi.fMask |= RBBIM_SIZE;
            }

            // 设置子窗口高度和最小高度
            if (bandInfo.cyChild > 0) {
                rbbi.cyChild = bandInfo.cyChild;
                rbbi.cyMinChild = bandInfo.cyMinChild > 0 ? bandInfo.cyMinChild : bandInfo.cyChild;
                rbbi.fMask |= RBBIM_CHILDSIZE;
            }
            else {
                RECT rc;
                GetWindowRect(bandInfo.hwndChild, &rc);
                int height = rc.bottom - rc.top;
                rbbi.cyChild = height;
                rbbi.cyMinChild = height;
                rbbi.fMask |= RBBIM_CHILDSIZE;
            }

            ShowWindow(bandInfo.hwndChild, SW_SHOW);
        }
        else {
            rbbi.lpText = const_cast<wchar_t*>(bandInfo.szText.c_str());
            rbbi.fMask = RBBIM_TEXT | RBBIM_STYLE | RBBIM_SIZE;
            SIZE sz;
            HDC hdc = GetDC(m_hwnd);
            HFONT hFont = (HFONT)SendMessage(m_hwnd, WM_GETFONT, 0, 0);
            if (hFont) {
                HGDIOBJ oldFont = SelectObject(hdc, hFont);
                GetTextExtentPoint32W(hdc, bandInfo.szText.c_str(), (int)bandInfo.szText.length(), &sz);
                SelectObject(hdc, oldFont);
            }
            else {
                sz.cx = 100; sz.cy = 20;
            }
            ReleaseDC(m_hwnd, hdc);
            rbbi.cx = bandInfo.cx > 0 ? bandInfo.cx : sz.cx + 20;
        }

        // 关键：确保每个 Band 都有非空标题（至少一个空格）
        static wchar_t defaultTitle[] = L" ";
        wchar_t titleBuffer[64] = { 0 };
        if (bandInfo.hwndChild) {
            if (!bandInfo.szText.empty()) {
                wcscpy_s(titleBuffer, bandInfo.szText.c_str());
                rbbi.lpText = titleBuffer;
                rbbi.fMask |= RBBIM_TEXT;
            }
            else {
                rbbi.lpText = defaultTitle;
                rbbi.fMask |= RBBIM_TEXT;
            }
        }
        else if (!bandInfo.szText.empty()) {
            wcscpy_s(titleBuffer, bandInfo.szText.c_str());
            rbbi.lpText = titleBuffer;
            rbbi.fMask |= RBBIM_TEXT;
        }

        rbbi.fStyle = bandInfo.fStyle;
        if (bandInfo.cxMinChild > 0) {
            rbbi.cxMinChild = bandInfo.cxMinChild;
            rbbi.fMask |= RBBIM_CHILDSIZE;
        }
        if (bandInfo.cxIdeal > 0) {
            rbbi.cxIdeal = bandInfo.cxIdeal;
            rbbi.fMask |= RBBIM_IDEALSIZE;
        }
        if (bandInfo.cxHeader > 0) {
            rbbi.cxHeader = bandInfo.cxHeader;
            rbbi.fMask |= RBBIM_HEADERSIZE;
        }
        if (bandInfo.iId != -1) {
            rbbi.wID = bandInfo.iId;
            rbbi.fMask |= RBBIM_ID;
        }
        else {
            static int s_nextId = 1000;
            rbbi.wID = s_nextId++;
            rbbi.fMask |= RBBIM_ID;
        }

        int iBand = (int)SendMessage(m_hwnd, RB_INSERTBANDW, (WPARAM)-1, (LPARAM)&rbbi);
        if (iBand != -1) {
            m_bandIds.push_back(rbbi.wID);
            if (bRedraw) InvalidateRect(m_hwnd, nullptr, TRUE);
        }
        return iBand;
    }

    int zhuziRebar::AddBand(const zhuziString& text, DWORD style, int minWidth, int idealWidth) {
        RebarBandInfo info = { 0 };
        info.szText = text;
        info.fStyle = style;
        info.cxMinChild = minWidth;
        info.cxIdeal = idealWidth;
        return AddBand(info);
    }

    int zhuziRebar::AddBand(zhuziControl* childControl, DWORD style, int minWidth, int idealWidth, int fixedHeight) {
        if (!childControl || !childControl->getHandle()) return -1;
        // 重要：子控件的父窗口应该是 Rebar 的父窗口（而不是 Rebar 本身），否则拖拽会消失
        HWND hParentRebar = GetParent(m_hwnd);
        if (GetParent(childControl->getHandle()) != hParentRebar) {
            SetParent(childControl->getHandle(), hParentRebar);
        }
        RebarBandInfo info = { 0 };
        info.hwndChild = childControl->getHandle();
        info.fStyle = style;
        info.cxMinChild = minWidth;
        info.cxIdeal = idealWidth;
        info.cyChild = fixedHeight;
        info.cyMinChild = fixedHeight;          // 最小高度等于固定高度
        info.szText = L" ";                     // 必须有标题（空格也可）
        return AddBand(info);
    }

    int zhuziRebar::AddToolbarBand(zhuziToolBar* toolBar, DWORD style,
        int minWidth, int idealWidth,
        int fixedHeight, const zhuziString& title)
    {
        if (!toolBar || !toolBar->getHandle()) return -1;

        // 关键：将工具栏的父窗口设置为 Rebar 的父窗口（否则拖拽带区时工具栏会消失）
        HWND hParentRebar = GetParent(m_hwnd);
        //if (GetParent(toolBar->getHandle()) != hParentRebar) {
        //    SetParent(toolBar->getHandle(), hParentRebar);
        //}

        toolBar->setAutoTopDock(false);

        // 构建带区信息
        RebarBandInfo info = { 0 };
        info.hwndChild = toolBar->getHandle();
        info.fStyle = style;
        info.cxMinChild = minWidth;
        info.cxIdeal = idealWidth;
        info.cyChild = fixedHeight;
        info.cyMinChild = fixedHeight;    // 最小高度等于固定高度
        info.szText = title.empty() ? L" " : title;  // 必须有标题（空格也可）

        // 自动计算理想宽度（如果未指定）
        if (idealWidth <= 0) {
            RECT rc;
            GetWindowRect(toolBar->getHandle(), &rc);
            info.cxIdeal = rc.right - rc.left;
        }

        return AddBand(info);
    }

    bool zhuziRebar::DeleteBand(int iBand) {
        BOOL ret = (BOOL)SendMessage(m_hwnd, RB_DELETEBAND, iBand, 0);
        if (ret && iBand >= 0 && iBand < (int)m_bandIds.size())
            m_bandIds.erase(m_bandIds.begin() + iBand);
        return ret != 0;
    }

    int zhuziRebar::GetBandCount() const {
        return (int)SendMessage(m_hwnd, RB_GETBANDCOUNT, 0, 0);
    }

    bool zhuziRebar::GetBandInfo(int iBand, REBARBANDINFOW& rbbi) const {
        rbbi.cbSize = sizeof(REBARBANDINFOW);
        return SendMessage(m_hwnd, RB_GETBANDINFOW, iBand, (LPARAM)&rbbi) != 0;
    }

    bool zhuziRebar::SetBandInfo(int iBand, const REBARBANDINFOW& rbbi) {
        REBARBANDINFOW tmp = rbbi;
        tmp.cbSize = sizeof(REBARBANDINFOW);
        return SendMessage(m_hwnd, RB_SETBANDINFOW, iBand, (LPARAM)&tmp) != 0;
    }

    void zhuziRebar::onParentResize(int parentWidth, int parentHeight) {
        if (!m_hwnd) return;
        HWND hParent = m_parent ? m_parent->getHandle() : GetParent(m_hwnd);
        if (!hParent) return;
        RECT rcParent;
        GetClientRect(hParent, &rcParent);
        int width = rcParent.right - rcParent.left;
        int height = (int)SendMessage(m_hwnd, RB_GETBARHEIGHT, 0, 0);
        SetWindowPos(m_hwnd, nullptr, 0, 0, width, height, SWP_NOZORDER);
        SendMessage(m_hwnd, WM_SIZE, 0, MAKELPARAM(width, height));
        zhuziControl::onParentResize(parentWidth, parentHeight);
    }

    void zhuziRebar::SetBackgroundColor(COLORREF clr) {
        if (m_hBackgroundBrush) DeleteObject(m_hBackgroundBrush);
        m_hBackgroundBrush = CreateSolidBrush(clr);
        m_bUseCustomBg = true;
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void zhuziRebar::SetControlStyle(DWORD dwStyle, bool bSet) {
        LONG_PTR curStyle = GetWindowLongPtr(m_hwnd, GWL_STYLE);
        if (bSet) curStyle |= dwStyle;
        else curStyle &= ~dwStyle;
        SetWindowLongPtr(m_hwnd, GWL_STYLE, curStyle);
        SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

    LRESULT CALLBACK zhuziRebar::RebarSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        zhuziRebar* pThis = (zhuziRebar*)dwRefData;
        if (!pThis) return DefSubclassProc(hwnd, msg, wParam, lParam);
        if (msg == WM_NOTIFY) {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            if (pnmh->code == RBN_HEIGHTCHANGE) {
                pThis->onParentResize(0, 0);
                return 0;
            }
            else if (pnmh->code == RBN_ENDDRAG) {
                // 拖拽结束，强制修复所有子窗口的父窗口、Z序和正确高度
                HWND hParentRebar = GetParent(hwnd);
                int bandCount = pThis->GetBandCount();
                for (int i = 0; i < bandCount; ++i) {
                    REBARBANDINFOW rbbi = { sizeof(REBARBANDINFOW) };
                    rbbi.fMask = RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_IDEALSIZE;
                    if (pThis->GetBandInfo(i, rbbi) && rbbi.hwndChild) {
                        HWND hChild = rbbi.hwndChild;
                        // 确保父窗口正确
                        if (GetParent(hChild) != hParentRebar) {
                            SetParent(hChild, hParentRebar);
                        }
                        // 强制恢复高度（防止因拖拽导致高度变为0）
                        if (rbbi.cyChild > 0) {
                            RECT rc;
                            GetWindowRect(hChild, &rc);
                            int curHeight = rc.bottom - rc.top;
                            if (curHeight != (int)rbbi.cyChild) {
                                // 通过子控件的布局系统恢复大小
                                zhuziControl* ctrl = (zhuziControl*)GetWindowLongPtrW(hChild, GWLP_USERDATA);
                                if (ctrl && ctrl->getParent()) {
                                    RECT rcParent;
                                    GetClientRect(ctrl->getParent()->getHandle(), &rcParent);
                                    ctrl->onParentResize(rcParent.right - rcParent.left,
                                        rcParent.bottom - rcParent.top);
                                }
                                else {
                                    // 回退：直接 SetWindowPos
                                    int width = (rbbi.cxIdeal > 0) ? rbbi.cxIdeal : rbbi.cx;
                                    SetWindowPos(hChild, nullptr, 0, 0,
                                        width, rbbi.cyChild,
                                        SWP_NOMOVE | SWP_NOZORDER);
                                }
                            }
                        }
                        // 将子窗口置顶，避免被 Rebar 背景遮挡
                        SetWindowPos(hChild, HWND_TOP, 0, 0, 0, 0,
                            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                    }
                }
                pThis->onParentResize(0, 0);
                return 0;
            }
        }

        if (msg == WM_ERASEBKGND && pThis->m_bUseCustomBg && pThis->m_hBackgroundBrush) {
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, pThis->m_hBackgroundBrush);
            return 1;
        }

        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    void zhuziRebar::create() {
        // 使用绝对布局创建，初始位置和大小任意（后续 onParentResize 会修正）
        zhuziControl::create(0, 0, 100, 30, WS_CHILD | WS_VISIBLE);
        // 确保立即调整到正确位置
        if (m_parent) {
            RECT rc;
            GetClientRect(m_parent->getHandle(), &rc);
            onParentResize(rc.right - rc.left, rc.bottom - rc.top);
        }
    }
} // namespace zhuzi