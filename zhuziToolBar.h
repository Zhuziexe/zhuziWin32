#pragma once
#include "zhuziControl.h"
#include "zhuziImageList.h"
#include <vector>
#include <functional>
#include <unordered_map>
#include <bitset>

namespace zhuzi {

    struct ToolBarButtonInfo {
        int         cmdId = -1;
        zhuziString text;
        int         imageIndex = -1;
        DWORD       style = 0;
        zhuziString tooltip;
    };

    class zhuziToolBar : public zhuziControl {
    public:
        zhuziToolBar(zhuziControl* parent = nullptr);
        virtual ~zhuziToolBar();

        // 简单创建（类似 StatusBar 的 create()）
        bool create();
        virtual bool onCreate(DWORD style) override;   // 为兼容基类，但不推荐直接使用

        void setImageList(zhuziImageList* imageList);
        void setImageList(HIMAGELIST himl);

        void addSeparator();
        int addButton(const zhuziString& text, int cmdId = -1,
            DWORD buttonStyle = 0, int imageIndex = -1);
        void addButtons(const std::vector<ToolBarButtonInfo>& buttons);

        bool enableButton(int cmdId, bool enable);
        bool checkButton(int cmdId, bool check);
        bool setButtonText(int cmdId, const zhuziString& text);
        bool setButtonImage(int cmdId, int imageIndex);
        bool isButtonChecked(int cmdId) const;
        bool isButtonEnabled(int cmdId) const;
        void clearButtons();

        void setOnClick(int cmdId, std::function<void()> callback);
        void removeOnClick(int cmdId);

        void setButtonSize(int width, int height);
        void setBitmapSize(int width, int height);
        void setMaxTextRows(int rows);
        void enableToolTips(bool enable);
        void setBackgroundColor(COLORREF color);   // 可选，可能不生效

        virtual void onParentResize(int parentWidth, int parentHeight) override;

        static int allocateCmdId();
        static void releaseCmdId(int cmdId);

        void setButtonTooltip(int cmdId, const zhuziString& tooltip);

        void setAutoTopDock(bool enable) { m_autoTopDock = enable; }
    private:
        HIMAGELIST m_himl;
        COLORREF   m_bgColor;
        bool m_autoTopDock;
        HBRUSH     m_hBgBrush;

        static std::bitset<1000> s_cmdIdUsed;
        static int AllocateCmdIdInternal();
        static void ReleaseCmdIdInternal(int cmdId);

        void autoSize();
        void autoSizeF();
        void updateLayout();               // 更新位置和大小
        void updateBackground();

        std::unordered_map<int, zhuziString> m_tooltips;

        static LRESULT CALLBACK ToolBarSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    };

} // namespace zhuzi