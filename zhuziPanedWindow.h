#pragma once
#include "zhuziControl.h"
#include <vector>
#include <functional>

namespace zhuzi {

    // 可拖动分隔条的多窗格容器
    class zhuziPanedWindow : public zhuziControl {
    public:
        enum class Orientation {
            Horizontal, // 水平排列（左右窗格），分隔条垂直拖动
            Vertical    // 垂直排列（上下窗格），分隔条水平拖动
        };

        zhuziPanedWindow(zhuziControl* parent = nullptr);
        virtual ~zhuziPanedWindow();

        virtual bool onCreate(DWORD style) override;
        virtual void onParentResize(int parentWidth, int parentHeight) override;

        // 设置排列方向（必须在添加窗格前设置，或设置后重新布局）
        void setOrientation(Orientation orient);
        Orientation getOrientation() const { return m_orientation; }

        // 设置分隔条粗细（像素）
        void setSashSize(int size);
        int getSashSize() const { return m_sashSize; }

        // 窗格管理
        void addPane(zhuziControl* pane);
        void insertPane(int index, zhuziControl* pane);
        void removePane(int index);
        void clearPanes();
        int getPaneCount() const { return (int)m_panes.size(); }
        zhuziControl* getPane(int index) const;

        // 分隔条位置存取（像素，相对于左边缘或上边缘）
        int getSashPosition(int sashIndex) const;      // sashIndex 从0到窗格数-2
        void setSashPosition(int sashIndex, int pos, bool redraw = true);

        // 设置窗格最小尺寸（避免被拖得过小）
        void setMinPaneSize(int minSize);
        int getMinPaneSize() const { return m_minPaneSize; }

        // 强制重新布局所有窗格
        void layoutPanes();

    protected:
        static LRESULT CALLBACK PanedWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

        void updateSashPositionsFromPanes();
        void getSashValidRange(int sashIndex, int& minPos, int& maxPos) const;
        int getPosFromMouse(int x, int y) const;
        void drawSashes(HDC hdc, const RECT& clientRect);
        void invalidateSash(int sashIndex);

    private:
        Orientation m_orientation;
        std::vector<zhuziControl*> m_panes;   // 存储窗格（不负责释放，由外部管理）
        std::vector<int> m_sashPositions;     // 分隔条位置（像素），长度 = panes.size() - 1
        int m_sashSize;                       // 分隔条宽度/高度（默认5px）
        int m_minPaneSize;                    // 每个窗格的最小尺寸（默认20px）

        // 拖拽状态
        int m_dragSashIndex;
        int m_dragStartPos;
        int m_originalSashPos;

        static bool s_classRegistered;
        static const wchar_t* WINDOW_CLASS_NAME;
        static bool RegisterWindowClass();
    };

} // namespace zhuzi