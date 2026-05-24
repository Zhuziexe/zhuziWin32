#pragma once
#include "zhuziControl.h"
#include "zhuziImageList.h"
#include "zhuziBinaryTree.h"
#include <vector>
#include <functional>

namespace zhuzi {

    // 封装 HTREEITEM 的树节点句柄类
    class zhuziTreeItem {
    public:
        // 默认构造（空节点）
        zhuziTreeItem() : m_hItem(nullptr) {}
        // 从原生 HTREEITEM 构造
        explicit zhuziTreeItem(HTREEITEM hItem) : m_hItem(hItem) {}

        // 获取原生句柄（仅供内部使用）
        HTREEITEM handle() const { return m_hItem; }

        // 判断是否有效
        bool isValid() const { return m_hItem != nullptr; }

        // 相等比较
        bool operator==(const zhuziTreeItem& other) const { return m_hItem == other.m_hItem; }
        bool operator!=(const zhuziTreeItem& other) const { return !(*this == other); }

        // 隐式转换为 bool（检查有效性）
        explicit operator bool() const { return isValid(); }

    private:
        HTREEITEM m_hItem;
    };

    // 树控件回调索引（与之前相同）
    enum TreeViewCallbackIndex {
        TVCB_SELCHANGE = 0,
        TVCB_DOUBLECLICK = 1,
        TVCB_RCLICK = 2,
        TVCB_EXPAND = 3,
        TVCB_CHECK = 4,
        TVCB_EDIT_START = 5,
        TVCB_EDIT_END = 6,
        TVCB_MAX = 10
    };

    class zhuziTreeView : public zhuziControl {
    public:
        zhuziTreeView(zhuziControl* parent = nullptr);
        virtual ~zhuziTreeView();

        virtual bool onCreate(DWORD style) override;
        virtual void destroy() override;
        virtual void onParentResize(int parentWidth, int parentHeight) override;

        // ---------- 样式操作 ----------
        void setStyle(DWORD style, bool enable);
        void setExtendedStyle(DWORD exStyle);
        void setCheckBoxes(bool enable);
        void setEditLabels(bool enable);
        void setLines(bool enable);
        void setLinesAtRoot(bool enable);
        void setButtons(bool enable);
        void setFullRowSelect(bool enable);   // 新增整行选择

        // ---------- 图像列表 ----------
        void setImageList(HIMAGELIST hImageList);
        void setImageList(const zhuziImageList& imageList);

        // ---------- 节点操作 ----------
        // 插入节点（返回封装的节点对象）
        zhuziTreeItem insertItem(zhuziTreeItem hParent, zhuziTreeItem hInsertAfter,
            const zhuziString& text, int image = -1, int selectedImage = -1, LPARAM data = 0);
        // 便捷插入：作为 hParent 的最后子节点
        zhuziTreeItem insertItem(zhuziTreeItem hParent, const zhuziString& text,
            int image = -1, int selectedImage = -1, LPARAM data = 0) {
            return insertItem(hParent, zhuziTreeItem(TVI_LAST), text, image, selectedImage, data);
        }
        // 插入根节点
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

        // 遍历辅助方法（返回封装的节点）
        zhuziTreeItem getRootItem() const;
        zhuziTreeItem getChildItem(zhuziTreeItem hItem) const;
        zhuziTreeItem getNextItem(zhuziTreeItem hItem) const;
        zhuziTreeItem getPrevItem(zhuziTreeItem hItem) const;
        zhuziTreeItem getParentItem(zhuziTreeItem hItem) const;

        void setCheckState(zhuziTreeItem hItem, bool checked);
        bool getCheckState(zhuziTreeItem hItem) const;

        bool editLabel(zhuziTreeItem hItem);

        // ---------- 事件回调 ----------
        void setOnSelChange(std::function<void(zhuziTreeItem)> callback);
        void setOnDoubleClick(std::function<void(zhuziTreeItem)> callback);
        void setOnRClick(std::function<void(zhuziTreeItem)> callback);
        void setOnExpand(std::function<void(zhuziTreeItem, bool expanding)> callback);
        void setOnCheck(std::function<void(zhuziTreeItem, bool checked)> callback);
        void setOnEditStart(std::function<bool(zhuziTreeItem)> callback);
        void setOnEditEnd(std::function<bool(zhuziTreeItem, const zhuziString& newText)> callback);

        void addFromBTree(const zhuziBinaryTree<zhuziString>& tree);
        void setAutoRedraw(bool enable);  // 启用/禁用重绘
    protected:
        void registerParentNotify();
        bool handleNotifyFromParent(NMHDR* pnmh);
        bool handleItemChanged(NMTREEVIEWW* pnmtv);
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

        // 辅助函数：刷新所有节点的复选框状态图像（在启用复选框后调用）
        void refreshAllCheckStates();
    };

} // namespace zhuzi