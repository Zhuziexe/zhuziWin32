#include "zhuziTreeView.h"
#include "zhuziInstance.h"
#include <commctrl.h>
#include <windowsx.h>

// 兼容旧 SDK：定义缺少的常量
#ifndef TVS_EX_CHECKBOXES
#define TVS_EX_CHECKBOXES 0x0004
#endif
#ifndef TVM_SETEXTENDEDSTYLE
#define TVM_SETEXTENDEDSTYLE (TVM_FIRST + 44)
#endif
#ifndef TVS_FULLROWSELECT
#define TVS_FULLROWSELECT 0x0100
#endif

namespace zhuzi {

    template<typename T>
    inline T* getCallbackFunc(void* ptr) {
        return ptr ? reinterpret_cast<T*>(ptr) : nullptr;
    }

    template<typename T>
    inline void setCallbackFunc(std::vector<void*>& arr, int idx, T&& func) {
        if (arr[idx]) {
            delete reinterpret_cast<T*>(arr[idx]);
            arr[idx] = nullptr;
        }
        if (func) {
            arr[idx] = new T(std::move(func));
        }
    }

    // ==================== zhuziTreeView 实现 ====================
    zhuziTreeView::zhuziTreeView(zhuziControl* parent)
        : zhuziControl(parent)
        , m_pendingStyleMask(0)
        , m_pendingStyleValue(0)
        , m_pendingExStyle(0)
        , m_flags{} {
        m_callbacks.resize(TVCB_MAX, nullptr);
    }

    zhuziTreeView::~zhuziTreeView() {
        for (void* ptr : m_callbacks) {
            if (ptr) delete reinterpret_cast<std::function<void(int)>*>(ptr);
        }
        destroy();
    }

    bool zhuziTreeView::onCreate(DWORD style) {
        DWORD finalStyle = style | WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_SHOWSELALWAYS;
        if (!createControl(WC_TREEVIEWW, 0, 0, 0, 0, finalStyle))
            return false;

        // 应用缓存的样式
        if (m_flags.hasPendingStyle) {
            LONG_PTR curStyle = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
            LONG_PTR mask = static_cast<LONG_PTR>(m_pendingStyleMask);
            LONG_PTR value = static_cast<LONG_PTR>(m_pendingStyleValue);
            curStyle = (curStyle & ~mask) | (value & mask);
            SetWindowLongPtrW(m_hwnd, GWL_STYLE, curStyle);
            SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            m_flags.hasPendingStyle = false;
        }
        if (m_flags.hasPendingExStyle) {
            SendMessageW(m_hwnd, TVM_SETEXTENDEDSTYLE, 0, (LPARAM)m_pendingExStyle);
            m_flags.hasPendingExStyle = false;
        }

        registerParentNotify();
        if (!m_flags.isSubclassed) {
            SetWindowSubclass(m_hwnd, TreeViewSubclassProc, 0, (DWORD_PTR)this);
            m_flags.isSubclassed = true;
        }
        return true;
    }

    void zhuziTreeView::destroy() {
        if (m_flags.isSubclassed && m_hwnd) {
            RemoveWindowSubclass(m_hwnd, TreeViewSubclassProc, 0);
            m_flags.isSubclassed = false;
        }
    }

    void zhuziTreeView::registerParentNotify() {
        if (m_flags.notifyRegistered) return;
        if (!m_hwnd) return;

        zhuziWindow* parentWnd = nullptr;
        zhuziControl* p = m_parent;
        while (p) {
            parentWnd = dynamic_cast<zhuziWindow*>(p);
            if (parentWnd) break;
            p = p->getParent();
        }
        if (parentWnd) {
            parentWnd->BindChain(WM_NOTIFY, [this](WPARAM, LPARAM lParam) -> bool {
                NMHDR* pnmh = reinterpret_cast<NMHDR*>(lParam);
                if (pnmh->hwndFrom == m_hwnd) {
                    return handleNotifyFromParent(pnmh);
                }
                return false;
                });
            m_flags.notifyRegistered = true;
        }
    }

    bool zhuziTreeView::handleNotifyFromParent(NMHDR* pnmh) {
        switch (pnmh->code) {
        case TVN_SELCHANGEDW:
            if (auto* cb = getCallbackFunc<std::function<void(zhuziTreeItem)>>(m_callbacks[TVCB_SELCHANGE])) {
                NMTREEVIEWW* pnmtv = (NMTREEVIEWW*)pnmh;
                (*cb)(zhuziTreeItem(pnmtv->itemNew.hItem));
            }
            return true;

        case NM_DBLCLK:
            if (auto* cb = getCallbackFunc<std::function<void(zhuziTreeItem)>>(m_callbacks[TVCB_DOUBLECLICK])) {
                (*cb)(getSelectedItem());
            }
            return true;

        case NM_RCLICK: {
            if (auto* cb = getCallbackFunc<std::function<void(zhuziTreeItem)>>(m_callbacks[TVCB_RCLICK])) {
                TVHITTESTINFO ht = { 0 };
                DWORD pos = GetMessagePos();
                ht.pt.x = GET_X_LPARAM(pos);
                ht.pt.y = GET_Y_LPARAM(pos);
                ScreenToClient(m_hwnd, &ht.pt);
                TreeView_HitTest(m_hwnd, &ht);
                (*cb)(zhuziTreeItem(ht.hItem ? ht.hItem : getSelectedItem().handle()));
            }
            return true;
        }

        case TVN_ITEMEXPANDINGW:
            if (auto* cb = getCallbackFunc<std::function<void(zhuziTreeItem, bool)>>(m_callbacks[TVCB_EXPAND])) {
                NMTREEVIEWW* pnmtv = (NMTREEVIEWW*)pnmh;
                bool expanding = (pnmtv->action == TVE_EXPAND);
                (*cb)(zhuziTreeItem(pnmtv->itemNew.hItem), expanding);
            }
            return false;

        case TVN_ITEMCHANGEDW:
            return handleItemChanged((NMTREEVIEWW*)pnmh);

        case TVN_BEGINLABELEDITW:
            return handleBeginLabelEdit((NMTVDISPINFOW*)pnmh);

        case TVN_ENDLABELEDITW:
            return handleEndLabelEdit((NMTVDISPINFOW*)pnmh);
        }
        return false;
    }

    bool zhuziTreeView::handleItemChanged(NMTREEVIEWW* pnmtv) {
        UINT oldState = pnmtv->itemOld.state & TVIS_STATEIMAGEMASK;
        UINT newState = pnmtv->itemNew.state & TVIS_STATEIMAGEMASK;
        if (oldState != newState) {
            if (auto* cb = getCallbackFunc<std::function<void(zhuziTreeItem, bool)>>(m_callbacks[TVCB_CHECK])) {
                bool checked = (newState >> 12) == 2;
                (*cb)(zhuziTreeItem(pnmtv->itemNew.hItem), checked);
            }
        }
        return false;
    }

    bool zhuziTreeView::handleBeginLabelEdit(NMTVDISPINFOW* pnm) {
        if (auto* cb = getCallbackFunc<std::function<bool(zhuziTreeItem)>>(m_callbacks[TVCB_EDIT_START])) {
            if (!(*cb)(zhuziTreeItem(pnm->item.hItem)))
                return TRUE;
        }
        return FALSE;
    }

    bool zhuziTreeView::handleEndLabelEdit(NMTVDISPINFOW* pnm) {
        if (pnm->item.pszText) {
            zhuziTreeItem hItem(pnm->item.hItem);
            zhuziString newText(pnm->item.pszText);
            if (auto* cb = getCallbackFunc<std::function<bool(zhuziTreeItem, const zhuziString&)>>(m_callbacks[TVCB_EDIT_END])) {
                if ((*cb)(hItem, newText)) {
                    setItemText(hItem, newText);
                    return TRUE;
                }
                return TRUE;
            }
            return FALSE;
        }
        return FALSE;
    }

    void zhuziTreeView::setStyle(DWORD style, bool enable) {
        DWORD mask = style;
        DWORD value = enable ? style : 0;
        if (m_hwnd) {
            LONG_PTR curStyle = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
            curStyle = (curStyle & ~mask) | (value & mask);
            SetWindowLongPtrW(m_hwnd, GWL_STYLE, curStyle);
            SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
        else {
            m_pendingStyleMask |= mask;
            m_pendingStyleValue = (m_pendingStyleValue & ~mask) | value;
            m_flags.hasPendingStyle = true;
        }
    }

    void zhuziTreeView::setExtendedStyle(DWORD exStyle) {
        if (m_hwnd) {
            SendMessageW(m_hwnd, TVM_SETEXTENDEDSTYLE, 0, (LPARAM)exStyle);
        }
        else {
            m_pendingExStyle = exStyle;
            m_flags.hasPendingExStyle = true;
        }
    }

    void zhuziTreeView::setCheckBoxes(bool enable) {
        if (m_hwnd) {
            LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
            if (enable)
                style |= TVS_CHECKBOXES;
            else
                style &= ~TVS_CHECKBOXES;
            SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);
            SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            // 如果启用复选框，刷新所有现有节点的状态图像
            if (enable)
                refreshAllCheckStates();
            InvalidateRect(m_hwnd, nullptr, TRUE);
        }
        else {
            // 尚未创建，缓存样式
            if (enable) {
                m_pendingStyleMask |= TVS_CHECKBOXES;
                m_pendingStyleValue |= TVS_CHECKBOXES;
            }
            else {
                m_pendingStyleMask |= TVS_CHECKBOXES;
                m_pendingStyleValue &= ~TVS_CHECKBOXES;
            }
            m_flags.hasPendingStyle = true;
        }
    }

    void zhuziTreeView::setEditLabels(bool enable) {
        setStyle(TVS_EDITLABELS, enable);
    }

    void zhuziTreeView::setLines(bool enable) {
        setStyle(TVS_HASLINES, enable);
    }

    void zhuziTreeView::setLinesAtRoot(bool enable) {
        setStyle(TVS_LINESATROOT, enable);
        if (m_hwnd) {
            SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            RedrawWindow(m_hwnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE);
        }
    }

    void zhuziTreeView::setButtons(bool enable) {
        setStyle(TVS_HASBUTTONS, enable);
        if (m_hwnd) {
            SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            RedrawWindow(m_hwnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE);
        }
    }

    void zhuziTreeView::setFullRowSelect(bool enable) {
        setStyle(TVS_FULLROWSELECT, enable);
        if (m_hwnd) {
            SetWindowPos(m_hwnd, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            InvalidateRect(m_hwnd, nullptr, TRUE);
        }
    }

    void zhuziTreeView::setImageList(HIMAGELIST hImageList) {
        if (m_hwnd) TreeView_SetImageList(m_hwnd, hImageList, TVSIL_NORMAL);
    }

    void zhuziTreeView::setImageList(const zhuziImageList& imageList) {
        setImageList(imageList.getHandle());
    }

    zhuziTreeItem zhuziTreeView::insertItem(zhuziTreeItem hParent, zhuziTreeItem hInsertAfter,
        const zhuziString& text, int image, int selectedImage, LPARAM data) {
        if (!m_hwnd) return zhuziTreeItem();
        TVINSERTSTRUCTW tvins = { 0 };
        tvins.hParent = hParent.handle();
        tvins.hInsertAfter = hInsertAfter.handle();
        tvins.item.mask = TVIF_TEXT;
        if (image >= 0 || selectedImage >= 0) {
            tvins.item.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
            tvins.item.iImage = image;
            tvins.item.iSelectedImage = (selectedImage >= 0) ? selectedImage : image;
        }
        if (data != 0) {
            tvins.item.mask |= TVIF_PARAM;
            tvins.item.lParam = data;
        }
        tvins.item.pszText = const_cast<LPWSTR>(text.c_str());
        return zhuziTreeItem(TreeView_InsertItem(m_hwnd, &tvins));
    }

    void zhuziTreeView::deleteItem(zhuziTreeItem hItem) {
        if (m_hwnd && hItem.handle()) TreeView_DeleteItem(m_hwnd, hItem.handle());
    }

    void zhuziTreeView::deleteAllItems() {
        if (m_hwnd) TreeView_DeleteAllItems(m_hwnd);
    }

    void zhuziTreeView::setItemText(zhuziTreeItem hItem, const zhuziString& text) {
        if (!m_hwnd || !hItem.handle()) return;
        TVITEMW item = { 0 };
        item.mask = TVIF_TEXT;
        item.hItem = hItem.handle();
        item.pszText = const_cast<LPWSTR>(text.c_str());
        TreeView_SetItem(m_hwnd, &item);
    }

    zhuziString zhuziTreeView::getItemText(zhuziTreeItem hItem) const {
        if (!m_hwnd || !hItem.handle()) return L"";
        wchar_t buffer[512] = { 0 };
        TVITEMW item = { 0 };
        item.mask = TVIF_TEXT;
        item.hItem = hItem.handle();
        item.pszText = buffer;
        item.cchTextMax = 511;
        TreeView_GetItem(m_hwnd, &item);
        return zhuziString(buffer);
    }

    void zhuziTreeView::setItemData(zhuziTreeItem hItem, LPARAM data) {
        if (!m_hwnd || !hItem.handle()) return;
        TVITEMW item = { 0 };
        item.mask = TVIF_PARAM;
        item.hItem = hItem.handle();
        item.lParam = data;
        TreeView_SetItem(m_hwnd, &item);
    }

    LPARAM zhuziTreeView::getItemData(zhuziTreeItem hItem) const {
        if (!m_hwnd || !hItem.handle()) return 0;
        TVITEMW item = { 0 };
        item.mask = TVIF_PARAM;
        item.hItem = hItem.handle();
        TreeView_GetItem(m_hwnd, &item);
        return item.lParam;
    }

    void zhuziTreeView::setItemImage(zhuziTreeItem hItem, int image, int selectedImage) {
        if (!m_hwnd || !hItem.handle()) return;
        TVITEMW item = { 0 };
        item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        item.hItem = hItem.handle();
        item.iImage = image;
        item.iSelectedImage = (selectedImage >= 0) ? selectedImage : image;
        TreeView_SetItem(m_hwnd, &item);
    }

    void zhuziTreeView::getItemImage(zhuziTreeItem hItem, int& image, int& selectedImage) const {
        image = selectedImage = -1;
        if (!m_hwnd || !hItem.handle()) return;
        TVITEMW item = { 0 };
        item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        item.hItem = hItem.handle();
        if (TreeView_GetItem(m_hwnd, &item)) {
            image = item.iImage;
            selectedImage = item.iSelectedImage;
        }
    }

    void zhuziTreeView::expand(zhuziTreeItem hItem, bool expand) {
        if (m_hwnd && hItem.handle()) {
            TreeView_Expand(m_hwnd, hItem.handle(), expand ? TVE_EXPAND : TVE_COLLAPSE);
        }
    }

    bool zhuziTreeView::isExpanded(zhuziTreeItem hItem) const {
        if (!m_hwnd || !hItem.handle()) return false;
        TVITEMW item = { 0 };
        item.mask = TVIF_STATE;
        item.hItem = hItem.handle();
        item.stateMask = TVIS_EXPANDED;
        TreeView_GetItem(m_hwnd, &item);
        return (item.state & TVIS_EXPANDED) != 0;
    }

    void zhuziTreeView::ensureVisible(zhuziTreeItem hItem) {
        if (m_hwnd && hItem.handle()) TreeView_EnsureVisible(m_hwnd, hItem.handle());
    }

    void zhuziTreeView::selectItem(zhuziTreeItem hItem) {
        if (m_hwnd) TreeView_SelectItem(m_hwnd, hItem.handle());
    }

    zhuziTreeItem zhuziTreeView::getSelectedItem() const {
        if (!m_hwnd) return zhuziTreeItem();
        return zhuziTreeItem(TreeView_GetSelection(m_hwnd));
    }

    zhuziTreeItem zhuziTreeView::getRootItem() const {
        if (!m_hwnd) return zhuziTreeItem();
        return zhuziTreeItem(TreeView_GetRoot(m_hwnd));
    }

    zhuziTreeItem zhuziTreeView::getChildItem(zhuziTreeItem hItem) const {
        if (!m_hwnd || !hItem.handle()) return zhuziTreeItem();
        return zhuziTreeItem(TreeView_GetChild(m_hwnd, hItem.handle()));
    }

    zhuziTreeItem zhuziTreeView::getNextItem(zhuziTreeItem hItem) const {
        if (!m_hwnd || !hItem.handle()) return zhuziTreeItem();
        return zhuziTreeItem(TreeView_GetNextSibling(m_hwnd, hItem.handle()));
    }

    zhuziTreeItem zhuziTreeView::getPrevItem(zhuziTreeItem hItem) const {
        if (!m_hwnd || !hItem.handle()) return zhuziTreeItem();
        return zhuziTreeItem(TreeView_GetPrevSibling(m_hwnd, hItem.handle()));
    }

    zhuziTreeItem zhuziTreeView::getParentItem(zhuziTreeItem hItem) const {
        if (!m_hwnd || !hItem.handle()) return zhuziTreeItem();
        return zhuziTreeItem(TreeView_GetParent(m_hwnd, hItem.handle()));
    }

    void zhuziTreeView::setCheckState(zhuziTreeItem hItem, bool checked) {
        if (!m_hwnd || !hItem.handle()) return;
        TVITEMW item = { 0 };
        item.mask = TVIF_STATE;
        item.hItem = hItem.handle();
        item.stateMask = TVIS_STATEIMAGEMASK;
        item.state = checked ? INDEXTOSTATEIMAGEMASK(2) : INDEXTOSTATEIMAGEMASK(1);
        TreeView_SetItem(m_hwnd, &item);
    }

    bool zhuziTreeView::getCheckState(zhuziTreeItem hItem) const {
        if (!m_hwnd || !hItem.handle()) return false;
        TVITEMW item = { 0 };
        item.mask = TVIF_STATE;
        item.hItem = hItem.handle();
        item.stateMask = TVIS_STATEIMAGEMASK;
        TreeView_GetItem(m_hwnd, &item);
        return (item.state >> 12) == 2;
    }

    bool zhuziTreeView::editLabel(zhuziTreeItem hItem) {
        if (!m_hwnd || !hItem.handle()) return false;
        return TreeView_EditLabel(m_hwnd, hItem.handle()) != nullptr;
    }

    void zhuziTreeView::refreshAllCheckStates() {
        if (!m_hwnd) return;

        std::function<void(HTREEITEM)> recurse = [&](HTREEITEM hItem) {
            if (!hItem) return;
            TVITEMW item = { 0 };
            item.mask = TVIF_STATE;
            item.hItem = hItem;
            item.stateMask = TVIS_STATEIMAGEMASK;
            if (TreeView_GetItem(m_hwnd, &item)) {
                UINT stateImage = (item.state >> 12);
                if (stateImage != 1 && stateImage != 2) {
                    item.state = INDEXTOSTATEIMAGEMASK(1);
                    TreeView_SetItem(m_hwnd, &item);
                }
            }
            HTREEITEM hChild = TreeView_GetChild(m_hwnd, hItem);
            while (hChild) {
                recurse(hChild);
                hChild = TreeView_GetNextSibling(m_hwnd, hChild);
            }
            };

        HTREEITEM hRoot = TreeView_GetRoot(m_hwnd);
        while (hRoot) {
            recurse(hRoot);
            hRoot = TreeView_GetNextSibling(m_hwnd, hRoot);
        }
    }

    // ---------- 回调设置 ----------
    void zhuziTreeView::setOnSelChange(std::function<void(zhuziTreeItem)> callback) {
        setCallbackFunc(m_callbacks, TVCB_SELCHANGE, std::move(callback));
        registerParentNotify();
    }

    void zhuziTreeView::setOnDoubleClick(std::function<void(zhuziTreeItem)> callback) {
        setCallbackFunc(m_callbacks, TVCB_DOUBLECLICK, std::move(callback));
        registerParentNotify();
    }

    void zhuziTreeView::setOnRClick(std::function<void(zhuziTreeItem)> callback) {
        setCallbackFunc(m_callbacks, TVCB_RCLICK, std::move(callback));
        registerParentNotify();
    }

    void zhuziTreeView::setOnExpand(std::function<void(zhuziTreeItem, bool expanding)> callback) {
        setCallbackFunc(m_callbacks, TVCB_EXPAND, std::move(callback));
        registerParentNotify();
    }

    void zhuziTreeView::setOnCheck(std::function<void(zhuziTreeItem, bool checked)> callback) {
        setCheckBoxes(true);
        setCallbackFunc(m_callbacks, TVCB_CHECK, std::move(callback));
        registerParentNotify();
    }

    void zhuziTreeView::setOnEditStart(std::function<bool(zhuziTreeItem)> callback) {
        setEditLabels(true);
        setCallbackFunc(m_callbacks, TVCB_EDIT_START, std::move(callback));
        registerParentNotify();
    }

    void zhuziTreeView::setOnEditEnd(std::function<bool(zhuziTreeItem, const zhuziString&)> callback) {
        setEditLabels(true);
        setCallbackFunc(m_callbacks, TVCB_EDIT_END, std::move(callback));
        registerParentNotify();
    }

    LRESULT CALLBACK zhuziTreeView::TreeViewSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        zhuziTreeView* pThis = reinterpret_cast<zhuziTreeView*>(dwRefData);
        if (pThis && pThis->m_hwnd == hwnd) {
            LRESULT result = pThis->handleSubclassMessage(msg, wParam, lParam);
            if (result != -1) return result;
        }
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    LRESULT zhuziTreeView::handleSubclassMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        // 预留扩展，直接返回 -1 让默认处理
        return -1;
    }

    void zhuziTreeView::onParentResize(int parentWidth, int parentHeight) {
        zhuziControl::onParentResize(parentWidth, parentHeight);
    }

    void zhuziTreeView::addFromBTree(const zhuziBinaryTree<zhuziString>& tree) {
        if (tree.empty()) return;

        // 获取二叉树根节点
        auto rootNode = tree.getRoot();
        if (!rootNode) return;

        // 批量插入前关闭重绘
        setAutoRedraw(false);

        // 插入根节点
        zhuziTreeItem rootItem = insertRootItem(rootNode->data);

        // 递归添加子节点
        std::function<void(const zhuziBinaryTree<zhuziString>::Node*, zhuziTreeItem)> addChildren =
            [&](const zhuziBinaryTree<zhuziString>::Node* node, zhuziTreeItem parentItem) {
            if (!node) return;

            // 处理左子树
            if (node->left) {
                zhuziTreeItem leftItem = insertItem(parentItem, node->left->data);
                addChildren(node->left, leftItem);
            }
            // 处理右子树
            if (node->right) {
                zhuziTreeItem rightItem = insertItem(parentItem, node->right->data);
                addChildren(node->right, rightItem);
            }
            };

        addChildren(rootNode, rootItem);

        // 恢复重绘并刷新
        setAutoRedraw(true);
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void zhuziTreeView::setAutoRedraw(bool enable) {
        if (m_hwnd) {
            SendMessageW(m_hwnd, WM_SETREDRAW, enable ? TRUE : FALSE, 0);
        }
    }
} // namespace zhuzi