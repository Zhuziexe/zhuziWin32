// zhuziRebar.h
#pragma once
#include "zhuziControl.h"
#include <commctrl.h>
#include <vector>

namespace zhuzi {

    // Rebar Band 信息结构体（不含图像列表）
    struct RebarBandInfo {
        int iId;                    // Band ID (可自定义)
        HWND hwndChild;             // 子窗口句柄（可为 NULL）
        zhuziString szText;         // 显示文本（若无子窗口则显示）
        DWORD fStyle;               // RBBS_* 样式
        DWORD cxMinChild;           // 最小宽度（像素）
        DWORD cxIdeal;              // 理想宽度
        DWORD cx;                   // 当前宽度（0 自动）
        DWORD cyChild;              // 子窗口高度（默认取子窗口高度）
        DWORD cyMinChild;           // 子窗口最小高度（默认为 cyChild）
        DWORD cxHeader;             // 标题栏宽度（默认 0）
    };

    class zhuziRebar : public zhuziControl {
    public:
        zhuziRebar(zhuziControl* parent = nullptr);
        virtual ~zhuziRebar();

        virtual bool onCreate(DWORD style) override;

        // 添加 Band
        int AddBand(const RebarBandInfo& bandInfo, bool bRedraw = true);
        // 简便方法：文本 Band
        int AddBand(const zhuziString& text, DWORD style = RBBS_BREAK | RBBS_GRIPPERALWAYS,
            int minWidth = 50, int idealWidth = 100);
        // 简便方法：子控件 Band
        int AddBand(zhuziControl* childControl, DWORD style = RBBS_GRIPPERALWAYS,
            int minWidth = 50, int idealWidth = 100, int fixedHeight = 0);

        bool DeleteBand(int iBand);
        int GetBandCount() const;
        bool GetBandInfo(int iBand, REBARBANDINFOW& rbbi) const;
        bool SetBandInfo(int iBand, const REBARBANDINFOW& rbbi);

        virtual void onParentResize(int parentWidth, int parentHeight) override;

        void SetBackgroundColor(COLORREF clr);
        void SetControlStyle(DWORD dwStyle, bool bSet = true);

    protected:
        static LRESULT CALLBACK RebarSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    private:
        std::vector<int> m_bandIds;
        HBRUSH m_hBackgroundBrush;
        bool m_bUseCustomBg;
    };

} // namespace zhuzi