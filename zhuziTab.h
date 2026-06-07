#pragma once
#include "zhuziControl.h"
#include <commctrl.h>
#include <vector>
#include <functional>
#include "zhuziImageList.h"

namespace zhuzi {

    struct TabPageInfo {
        zhuziString text;
        zhuziControl* control;
        LPARAM userData;
        int imageIndex;
    };

    class zhuziTab : public zhuziControl {
    public:
        zhuziTab(zhuziControl* parent = nullptr);
        virtual ~zhuziTab();

        virtual bool onCreate(DWORD style) override;
        virtual void destroy() override;

        // 样式设置函数（可在创建后随时调用）
        void setFlatButtons(bool flat);
        void setBottom(bool bottom);
        void setMultiLine(bool multiLine);
        void setFixedWidth(bool fixed, int width = 0);  // 固定标签宽度，width=0时自动计算
        void setButtonMode(bool buttonMode);            // 按钮模式（标签看起来像按钮）
        void setFocusOnSelChange(bool focus);           // 切换时是否聚焦到标签本身

        // 获取当前样式
        DWORD getTabStyle() const;

        // 页面管理
        int addPage(const zhuziString& text, zhuziControl* pageControl, int imageIndex = -1);
        int insertPage(int index, const zhuziString& text, zhuziControl* pageControl, int imageIndex = -1);
        void removePage(int index, bool destroyControl = false);
        void clearPages(bool destroyControls = false);

        zhuziString getPageText(int index) const;
        void setPageText(int index, const zhuziString& text);

        zhuziControl* getPageControl(int index) const;
        void setPageControl(int index, zhuziControl* newControl, bool destroyOld = false);

        LPARAM getPageUserData(int index) const;
        void setPageUserData(int index, LPARAM data);

        int getPageImageIndex(int index) const;
        void setPageImageIndex(int index, int imageIndex);

        int getCurSel() const;
        bool setCurSel(int index);
        int getPageCount() const;

        void setImageList(zhuziImageList* _iml);
        void setImageList(HIMAGELIST himl);
        HIMAGELIST getImageList() const;

        void setOnSelChange(std::function<void(int, int)> callback);

        void updateCurrentPageLayout();

        virtual void onParentResize(int parentWidth, int parentHeight) override;

        bool handleNotifyFromParent(NMHDR* pnmh);

    protected:
        RECT getContentRect() const;
        void layoutCurrentPage();
        void updatePagesVisibility();
        void updateTabStyle();  // 刷新样式（修改样式后调用）

        static LRESULT CALLBACK TabWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        LRESULT handleSubclassMessage(UINT msg, WPARAM wParam, LPARAM lParam);

        void registerParentNotify();

    private:
        std::vector<TabPageInfo> m_pages;
        HIMAGELIST m_hImageList;
        int m_curSel;
        std::function<void(int, int)> m_onSelChange;
        bool m_isSubclassed;
        bool m_notifyRegistered;

        // 存储创建时的原始样式，用于动态修改样式时重新应用
        DWORD m_originalStyle;
    };

} // namespace zhuzi