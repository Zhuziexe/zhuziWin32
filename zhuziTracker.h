#pragma once
#include "zhuziControl.h"
#include <functional>

namespace zhuzi {

    class zhuziTracker : public zhuziControl {
    public:
        enum class Orientation {
            Horizontal,
            Vertical
        };

        zhuziTracker(zhuziControl* parent = nullptr);
        virtual ~zhuziTracker();

        virtual bool onCreate(DWORD style) override;

        // 设置范围
        void setRange(int min, int max);
        void getRange(int& min, int& max) const;

        // 设置当前位置
        void setPos(int pos);
        int getPos() const;

        // 设置刻度频率（仅当启用刻度时有效）
        void setTickFreq(int freq);

        // 设置刻度标记显示方式
        void setTicks(bool autoTicks = true, int freq = 1);

        // 设置方向
        void setOrientation(Orientation orient);

        // 启用/禁用选择范围（双滑块模式）
        void setSelectionRange(bool enable);
        void setSelStart(int start);
        void setSelEnd(int end);
        void getSelRange(int& start, int& end) const;

        // 设置页面步长和行步长
        void setPageSize(int pageSize);
        void setLineSize(int lineSize);

        // 清除选择范围（如果有）
        void clearSel();

        // 事件：值改变时触发
        void setOnPosChange(std::function<void(int pos)> callback);

        // 可选：自定义绘制（通过子类化高级主题，此处不实现）

    private:
        Orientation m_orientation;
        bool m_autoTicks;
        int m_tickFreq;
        std::function<void(int)> m_onPosChange;

        void updateStyle();
        static LRESULT CALLBACK TrackerSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        LRESULT handleNotify(NMHDR* pnmh);
        void onPosChanged();
    };

} // namespace zhuzi