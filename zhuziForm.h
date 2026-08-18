#pragma once
#include "zhuziControl.h"

namespace zhuzi {

    class zhuziForm : public zhuziControl {
    public:
        zhuziForm(zhuziControl* parent = nullptr);
        virtual ~zhuziForm();

        virtual bool onCreate(DWORD style) override;
        virtual void destroy() override;
        virtual void onParentResize(int parentWidth, int parentHeight) override;
        virtual void onPaint(zhuziPaint& paint) override;

        void addControl(zhuziControl* pCtrl, double widthPercent, int height,
            AlignHorizontal align = AlignHorizontal::Center);
        void addControl(zhuziControl* pCtrl, int width, int height,
            AlignHorizontal align = AlignHorizontal::Center);

        void setControlWidth(zhuziControl* pCtrl, double widthPercent);
        void setControlWidth(zhuziControl* pCtrl, int width);
        void setControlHeight(zhuziControl* pCtrl, int height);

        void setSpacing(int spacing) { m_spacing = spacing; layoutChildren(); }
        void destroyAllControls();
        void layoutChildren();

    protected:
        virtual void onVScroll(WPARAM wParam, LPARAM lParam);
        virtual void onMouseWheel(WPARAM wParam, LPARAM lParam);

    private:
        int m_spacing;
        int m_scrollPos;
        int m_contentHeight;
        int m_vScrollBindId;
        int m_mouseWheelBindId;

        void updateScrollInfo(int clientHeight);

        // 窗口类注册相关（改为私有静态成员函数）
        static bool RegisterFormClass();
        static LRESULT CALLBACK FormWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        static const wchar_t* FORM_CLASS_NAME;
        static bool s_classRegistered;
    };

} // namespace zhuzi