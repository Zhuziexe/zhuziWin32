#pragma once
#include "zhuziControl.h"

namespace zhuzi {

    class zhuziToolTip : public zhuziControl {
    public:
        zhuziToolTip(zhuziControl* parent = nullptr);
        virtual ~zhuziToolTip();

        bool create();
        virtual bool onCreate(DWORD style) override;

        // 添加工具提示（支持多行文本，使用 \n）
        void addTool(HWND hwndTool, const zhuziString& text, bool bSubclass = true);
        void addTool(zhuziControl* ctrl, const zhuziString& text, bool bSubclass = true);

        // 移除工具提示
        void delTool(HWND hwndTool);
        void delTool(zhuziControl* ctrl);

        // 更新提示文本
        void updateToolText(HWND hwndTool, const zhuziString& text);
        void updateToolText(zhuziControl* ctrl, const zhuziString& text);

        // 设置延迟时间（毫秒）
        void setDelayTime(int initial = 500, int reshow = 100, int autoPop = 3000);

        // 设置最大宽度（用于换行）
        void setMaxTipWidth(int width = 300);

        // 启用圆角气泡样式
        void setBalloon(bool enable);

        // 设置图标和标题（标题会显示在图标右侧）
        void setIcon(int iconType, const zhuziString& title = L"");

        // 激活/停用
        void activate(bool active);

    private:
        HWND m_hToolTip;
        zhuziString m_title;
    };

} // namespace zhuzi