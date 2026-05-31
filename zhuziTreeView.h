#pragma once
#include "zhuziControl.h"
#include "zhuziImageList.h"
#include "zhuziBinaryTree.h"
#include <vector>
#include <functional>

namespace zhuzi {

    class zhuziTreeItem {
    public:
        zhuziTreeItem() : m_hItem(nullptr) {}
        explicit zhuziTreeItem(HTREEITEM hItem) : m_hItem(hItem) {}
        HTREEITEM handle() const { return m_hItem; }
        bool isValid() const { return m_hItem != nullptr; }
        bool operator==(const zhuziTreeItem& other) const { return m_hItem == other.m_hItem; }
        bool operator!=(const zhuziTreeItem& other) const { return !(*this == other); }
        explicit operator bool() const { return isValid(); }
    private:
        HTREEITEM m_hItem;
    };

    enum TreeViewCallbackIndex {
        TVCB_SELCHANGE = 0,
        TVCB_DOUBLECLICK = 1,
        TVCB_RCLICK = 2,
        TVCB_EXPAND = 3,
        TVCB_EDIT_START = 4,
        TVCB_EDIT_END = 5,
        TVCB_MAX = 10
    };

    class zhuziTreeView : public zhuziControl {
    public:
        zhuziTreeView(zhuziControl* parent = nullptr);
        virtual ~zhuziTreeView();

        virtual bool onCreate(DWORD style) override;
        virtual void destroy() override;
        virtual void onParentResize(int parentWidth, int parentHeight) override;

        void setStyle(DWORD style, bool enable);
        void setExtendedStyle(DWORD exStyle);
        void setCheckBoxes(bool enable);
        void setEditLabels(bool enable);
        void setLines(bool enable);
        void setLinesAtRoot(bool enable);
        void setButtons(bool enable);
        void setFullRowSelect(bool enable);

        void setImageList(HIMAGELIST hImageList);
        void setImageList(const zhuziImageList& imageList);

        zhuziTreeItem insertItem(zhuziTreeItem hParent, zhuziTreeItem hInsertAfter,
            const zhuziString& text, int image = -1, int selectedImage = -1, LPARAM data = 0);
        zhuziTreeItem insertItem(zhuziTreeItem hParent, const zhuziString& text,
            int image = -1, int selectedImage = -1, LPARAM data = 0) {
            return insertItem(hParent, zhuziTreeItem(TVI_LAST), text, image, selectedImage, data);
        }
        zhuziTreeItem insertRootItem(const zhuziString& text,
            int image = -1, int selectedImage = -1, LPARAM data = 0) {
            return insertItem(zhuziTreeItem(TVI_ROOT), zhuziTreeItem(TVI_LAST), text, image, selectedImage, data);
        }

        void deleteItem(zhuziTreeItem hItem);
        void deleteAllItems();

        void setItemText(zhuziTreeItem hItem, const zhuziString& text);
        zhuziString getItemText(zhuziTreeItem hItem) const;

        void setItemData(zhuziTreeItem hItem, LPARAM data);
        LPARAM getItemData(zhuziTreeItem hItem) const;

        void setItemImage(zhuziTreeItem hItem, int image, int selectedImage = -1);
        void getItemImage(zhuziTreeItem hItem, int& image, int& selectedImage) const;

        void expand(zhuziTreeItem hItem, bool expand = true);
        bool isExpanded(zhuziTreeItem hItem) const;
        void ensureVisible(zhuziTreeItem hItem);

        void selectItem(zhuziTreeItem hItem);
        zhuziTreeItem getSelectedItem() const;

        zhuziTreeItem getRootItem() const;
        zhuziTreeItem getChildItem(zhuziTreeItem hItem) const;
        zhuziTreeItem getNextItem(zhuziTreeItem hItem) const;
        zhuziTreeItem getPrevItem(zhuziTreeItem hItem) const;
        zhuziTreeItem getParentItem(zhuziTreeItem hItem) const;

        // 复选框操作（不触发任何回调）
        void setCheckState(zhuziTreeItem hItem, bool checked);
        bool getCheckState(zhuziTreeItem hItem) const;

        bool editLabel(zhuziTreeItem hItem);

        // 事件回调（无 onCheck）
        void setOnSelChange(std::function<void(zhuziTreeItem)> callback);
        void setOnDoubleClick(std::function<void(zhuziTreeItem)> callback);
        void setOnRClick(std::function<void(zhuziTreeItem)> callback);
        void setOnExpand(std::function<void(zhuziTreeItem, bool expanding)> callback);
        void setOnEditStart(std::function<bool(zhuziTreeItem)> callback);
        void setOnEditEnd(std::function<bool(zhuziTreeItem, const zhuziString& newText)> callback);

        void addFromBTree(const zhuziBinaryTree<zhuziString>& tree);
        void setAutoRedraw(bool enable);

    protected:
        void registerParentNotify();
        bool handleNotifyFromParent(NMHDR* pnmh);
        bool handleBeginLabelEdit(NMTVDISPINFOW* pnm);
        bool handleEndLabelEdit(NMTVDISPINFOW* pnm);
        LRESULT handleSubclassMessage(UINT msg, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK TreeViewSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

    private:
        DWORD m_pendingStyleMask;
        DWORD m_pendingStyleValue;
        DWORD m_pendingExStyle;
        struct PackedFlags {
            unsigned hasPendingStyle : 1;
            unsigned hasPendingExStyle : 1;
            unsigned isSubclassed : 1;
            unsigned notifyRegistered : 1;
        };
        PackedFlags m_flags;

        std::vector<void*> m_callbacks;   // 存储 std::function 指针

        void refreshAllCheckStates();
    };

} // namespace zhuzi