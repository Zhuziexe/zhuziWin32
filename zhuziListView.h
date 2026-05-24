#pragma once
#include "zhuziControl.h"
#include "zhuziImageList.h"
#include <vector>
#include <functional>
#include <bitset>

namespace zhuzi {

    // 回调索引枚举
    enum ListViewCallbackIndex {
        LVCB_CLICK = 0,
        LVCB_DOUBLECLICK = 1,
        LVCB_RCLICK = 2,
        LVCB_CHECK = 3,
        LVCB_EDIT_START = 4,
        LVCB_EDIT_END = 5,
        LVCB_MAX = 10
    };

    class zhuziListView : public zhuziControl {
    public:
        zhuziListView(zhuziControl* parent = nullptr);
        virtual ~zhuziListView();

        virtual bool onCreate(DWORD style) override;
        virtual void onParentResize(int parentWidth, int parentHeight);
        void setItemImage(int row, int imageIndex);
        void destroy();
        void setView(DWORD view);
        void setStyle(DWORD style, bool enable);
        void setExtendedStyle(DWORD exStyle);
        void setGridLines(bool enable);
        void setFullRowSelect(bool enable);
        void setCheckBoxes(bool enable);
        void setEditLabels(bool enable);

        void addColumn(const zhuziString& text, int width, int fmt = LVCFMT_LEFT);
        void deleteColumn(int colIndex);
        void setColumnWidth(int colIndex, int width);
        int  getColumnCount() const;

        int  insertItem(int index, const zhuziString& text);
        void setItemText(int itemIndex, int subItem, const zhuziString& text);
        zhuziString getItemText(int itemIndex, int subItem) const;
        void deleteItem(int index);
        void deleteAllItems();
        int  getItemCount() const;

        bool setLineText(int row, const std::vector<zhuziString>& texts);
        int addItem(const zhuziString& text, int imageIndex = -1);
        int addItem(const std::vector<zhuziString>& texts, int imageIndex = -1);
        int  insertItem(int index, const std::vector<zhuziString>& texts);

        void setImageList(HIMAGELIST hImageList, int type);
        void setImageList(const zhuziImageList& imageList, int type);

        int  getSelectedIndex() const;
        std::vector<int> getSelectedIndices() const;
        int  getSelectedCount() const;
        void setSelected(int index, bool selected = true);
        void ensureVisible(int index, bool partially = false);

        void setOnClick(std::function<void(int)> callback);
        void setOnDoubleClick(std::function<void(int)> callback);
        void setOnRClick(std::function<void(int)> callback);
        void setOnCheck(std::function<void(int,bool)> callback);
        void setOnEditStart(std::function<bool(int row)> callback);
        void setOnEdit(std::function<bool(int row, zhuziString newText)> callback);
        void checkBox(int row, bool checked);
        bool setCellControl(int row, int column, zhuziControl* control);
        zhuziControl* cellControl(int row, int column) const;
        void updateCellControlPositions();
    protected:
        void registerParentNotify();
        bool handleNotifyFromParent(NMHDR* pnmh);
        bool handleItemChanged(NMITEMACTIVATE* pnm);
        bool handleBeginLabelEdit(NMLVDISPINFOW* pnm);
        bool handleEndLabelEdit(NMLVDISPINFOW* pnm);
        LRESULT handleSubclassMessage(UINT msg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK ListViewSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    private:
        DWORD m_pendingView;
        DWORD m_pendingStyleMask;
        DWORD m_pendingStyleValue;
        DWORD m_pendingExStyle;
        struct PackedFlags {
            unsigned hasPendingView : 1;
            unsigned hasPendingStyle : 1;
            unsigned hasPendingExStyle : 1;
            unsigned isSubclassed : 1;
            unsigned notifyRegistered : 1;
        };
        PackedFlags m_flags;
        int  m_editingSubItem;

        std::vector<void*> m_callbacks;
        std::map<std::pair<int, int>, zhuziControl*> m_cellControls;
    };
}