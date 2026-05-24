#pragma once
#include "zhuziControl.h"
#include "zhuziCommctrl.h"

namespace zhuzi {

    class zhuziScrollArea : public zhuziControl {
    public:
        zhuziScrollArea(zhuziControl* parent = nullptr);
        virtual ~zhuziScrollArea();

        virtual bool onCreate(DWORD style) override;

        zhuziFrame* getContentFrame() const { return m_contentFrame; }

        void setContentSize(int width, int height);
        void autoFitContent(bool adjustChildPositions = true);

        int getScrollX() const { return m_scrollX; }
        int getScrollY() const { return m_scrollY; }
        void setScrollOffset(int x, int y);

        enum ScrollBarPolicy { AsNeeded, AlwaysOff, AlwaysOn };
        void setHorizontalScrollBarPolicy(ScrollBarPolicy policy);
        void setVerticalScrollBarPolicy(ScrollBarPolicy policy);

    protected:
        void updateScrollBars();
        void updateContentPosition();

    private:
        zhuziFrame* m_contentFrame;
        int m_scrollX, m_scrollY;
        int m_contentWidth, m_contentHeight;
        ScrollBarPolicy m_hPolicy, m_vPolicy;

        static bool RegisterScrollAreaClass();
        static const wchar_t* SCROLL_AREA_CLASS_NAME;
        static bool s_classRegistered;

        static LRESULT CALLBACK ScrollAreaWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        LRESULT handleScrollMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    };

} // namespace zhuzi