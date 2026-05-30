#pragma once
#include "zhuziControl.h"
#include <vector>
#include <memory>

namespace zhuzi {

    // 布局容器基类
    class zhuziLayout : public zhuziControl {
    public:
        zhuziLayout(zhuziControl* parent = nullptr);
        virtual ~zhuziLayout();

        // 添加控件，stretch 为拉伸因子（0 表示与其他控件平分，或使用比例）
        void addControl(zhuziControl* control, int stretch = 1);
        void removeControl(zhuziControl* control);
        void clear();

        void setSpacing(int spacing);
        int spacing() const { return m_spacing; }

        void setContentsMargins(int left, int top, int right, int bottom);
        void getContentsMargins(int& left, int& top, int& right, int& bottom) const;

        // 设置默认控件尺寸（当控件尚未创建或无法获取大小时使用）
        void setDefaultControlSize(int width, int height);

        virtual void updateLayout() = 0;
        virtual void onParentResize(int parentWidth, int parentHeight) override;

    protected:
        struct ControlItem {
            zhuziControl* control;
            int stretch;
        };
        std::vector<ControlItem> m_items;
        int m_spacing;
        int m_marginLeft, m_marginTop, m_marginRight, m_marginBottom;
        int m_defaultWidth, m_defaultHeight;

        void ensureControlCreated(zhuziControl* control);
        void getControlSize(zhuziControl* control, int& width, int& height);
    };

    // 水平布局：子控件宽度按 stretch 分配，高度填满
    class zhuziHLayout : public zhuziLayout {
    public:
        zhuziHLayout(zhuziControl* parent = nullptr);
        virtual ~zhuziHLayout();

        virtual bool onCreate(DWORD style) override;
        virtual void updateLayout() override;
    };

    // 垂直布局：子控件高度按 stretch 分配，宽度填满
    class zhuziVLayout : public zhuziLayout {
    public:
        zhuziVLayout(zhuziControl* parent = nullptr);
        virtual ~zhuziVLayout();

        virtual bool onCreate(DWORD style) override;
        virtual void updateLayout() override;
    };

} // namespace zhuzi