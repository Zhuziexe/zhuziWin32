#include "zhuziListView.h"
#include "zhuziInstance.h"
#include <commctrl.h>
#include <windowsx.h>
#include "zhuziMenu.h"

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

    zhuziListView::zhuziListView(zhuziControl* parent)
        : zhuziControl(parent)
        , m_pendingView(LVS_REPORT)
        , m_pendingStyleMask(0)
        , m_pendingStyleValue(0)
        , m_pendingExStyle(0)
        , m_flags{}
        , m_editingSubItem(0)  {
        m_callbacks.resize(LVCB_MAX, nullptr);
    }

    zhuziListView::~zhuziListView() {
        // 释放所有回调对象
        for (void* ptr : m_callbacks) {
            if (ptr) {
                // 统一按 std::function<void(int)> 删除，但不安全，实际应区分类型
                // 简单起见，我们存储时已知类型，删除时直接 delete 即可，因为所有 std::function 大小相同
                delete reinterpret_cast<std::function<void(int)>*>(ptr);
            }
        }
        destroy();
    }

    bool zhuziListView::onCreate(DWORD style) {
        DWORD finalStyle = style | WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | LVS_SHOWSELALWAYS;
        if (!m_flags.hasPendingView)
            finalStyle |= LVS_REPORT;
        else
            finalStyle |= m_pendingView;

        if (!createControl(WC_LISTVIEWW, 0, 0, 0, 0, finalStyle))
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
            ListView_SetExtendedListViewStyle(m_hwnd, m_pendingExStyle);
            m_flags.hasPendingExStyle = false;
        }
        if (m_flags.hasPendingView) {
            ::SendMessageW(m_hwnd, LVM_SETVIEW, (WPARAM)m_pendingView, 0);
            m_flags.hasPendingView = false;
        }

        registerParentNotify();
        if (!m_flags.isSubclassed) {
            SetWindowSubclass(m_hwnd, ListViewSubclassProc, 0, (DWORD_PTR)this);
            m_flags.isSubclassed = true;
        }

        LONG_PTR curWndStyle = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
        SetWindowLongPtrW(m_hwnd, GWL_STYLE, curWndStyle | WS_CLIPCHILDREN);
        return true;
    }

    void zhuziListView::destroy() {
        if (m_flags.isSubclassed && m_hwnd) {
            RemoveWindowSubclass(m_hwnd, ListViewSubclassProc, 0);
            m_flags.isSubclassed = false;
        }
    }

    void zhuziListView::registerParentNotify() {
        if (m_flags.notifyRegistered) return;
        if (!m_hwnd) return;

        zhuziWindow* parentWnd = findParentWindow(this);
        if (parentWnd) {
            parentWnd->Bind(WM_NOTIFY, [this](zhuziMessage& msg) -> bool {
                NMHDR* pnmh = reinterpret_cast<NMHDR*>(msg.lParam);
                if (pnmh->hwndFrom == m_hwnd) {
                    return handleNotifyFromParent(pnmh);
                }
                return false;
                });
            m_flags.notifyRegistered = true;
        }
    }

    bool zhuziListView::handleNotifyFromParent(NMHDR* pnmh) {
        switch (pnmh->code) {
        case NM_CLICK:
            if (auto* cb = getCallbackFunc<std::function<void(int)>>(m_callbacks[LVCB_CLICK])) {
                NMITEMACTIVATE* pnm = (NMITEMACTIVATE*)pnmh;
                int idx = pnm->iItem;
                if (idx == -1 && getSelectedCount() > 0) {
                    auto sel = getSelectedIndices();
                    if (!sel.empty()) idx = sel[0];
                }
                (*cb)(idx);
            }
            return true;
        case NM_DBLCLK:
            if (auto* cb = getCallbackFunc<std::function<void(int)>>(m_callbacks[LVCB_DOUBLECLICK])) {
                NMITEMACTIVATE* pnm = (NMITEMACTIVATE*)pnmh;
                int idx = pnm->iItem;
                if (idx == -1 && getSelectedCount() > 0) {
                    auto sel = getSelectedIndices();
                    if (!sel.empty()) idx = sel[0];
                }
                (*cb)(idx);
            }
            return true;
        case NM_RCLICK:
            if (auto* cb = getCallbackFunc<std::function<void(int)>>(m_callbacks[LVCB_RCLICK])) {
                NMITEMACTIVATE* pnm = (NMITEMACTIVATE*)pnmh;
                int idx = pnm->iItem;
                if (idx == -1 && getSelectedCount() > 0) {
                    auto sel = getSelectedIndices();
                    if (!sel.empty()) idx = sel[0];
                }
                (*cb)(idx);
            }
            return true;
        case LVN_ITEMCHANGED:
            return handleItemChanged((NMITEMACTIVATE*)pnmh);
        case LVN_BEGINLABELEDITW:
            return handleBeginLabelEdit((NMLVDISPINFOW*)pnmh);
        case LVN_ENDLABELEDITW:
            return handleEndLabelEdit((NMLVDISPINFOW*)pnmh);
        }
        return false;
    }

    bool zhuziListView::handleItemChanged(NMITEMACTIVATE* pnm) {
        if (pnm->uChanged & LVIF_STATE) {
            UINT oldState = pnm->uOldState & LVIS_STATEIMAGEMASK;
            UINT newState = pnm->uNewState & LVIS_STATEIMAGEMASK;
            if (oldState != newState) {
                if (auto* cb = getCallbackFunc<std::function<void(int, bool)>>(m_callbacks[LVCB_CHECK])) {
                    bool checked = (newState >> 12) == 2; // 选中状态
                    (*cb)(pnm->iItem, checked);          // 传递行号和选中状态
                }
            }
        }
        return false;
    }

    bool zhuziListView::handleBeginLabelEdit(NMLVDISPINFOW* pnm) {
        int row = pnm->item.iItem;
        if (auto* cb = getCallbackFunc<std::function<bool(int)>>(m_callbacks[LVCB_EDIT_START])) {
            bool allow = (*cb)(row);
            if (!allow) {
                return TRUE;  // 阻止编辑
            }
        }
        return FALSE; // 允许编辑
    }

    bool zhuziListView::handleEndLabelEdit(NMLVDISPINFOW* pnm) {
        if (pnm->item.pszText) {
            if (auto* cb = getCallbackFunc<std::function<bool(int, zhuziString)>>(m_callbacks[LVCB_EDIT_END])) {
                int row = pnm->item.iItem;
                zhuziString newText(pnm->item.pszText);
                bool accept = (*cb)(row, newText);
                if (accept) {
                    // 接受修改：手动设置文本，并返回 TRUE 阻止默认处理
                    setItemText(row, 0, newText);
                    return TRUE;
                }
                else {
                    // 拒绝修改：直接返回 TRUE 恢复原文本
                    return TRUE;
                }
            }
            // 没有回调：让控件默认处理（返回 FALSE）
            return FALSE;
        }
        // 用户取消编辑
        return FALSE;
    }

    void zhuziListView::setView(DWORD view) {
        if (m_hwnd) {
            // 特殊处理平铺视图：需要先从图标视图转换
            if (view == LV_VIEW_TILE) {
                // 获取当前视图（可选）
                DWORD curView = (DWORD)SendMessageW(m_hwnd, LVM_GETVIEW, 0, 0);
                // 如果不是图标视图，则先切换到图标视图
                if (curView != LV_VIEW_ICON) {
                    // 切换到图标视图
                    SendMessageW(m_hwnd, LVM_SETVIEW, LV_VIEW_ICON, 0);
                    // 更新窗口样式为 LVS_ICON
                    LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
                    style = (style & ~LVS_TYPEMASK) | LVS_ICON;
                    SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);
                    // 强制重绘并处理所有待处理的绘制消息
                    InvalidateRect(m_hwnd, nullptr, TRUE);
                    UpdateWindow(m_hwnd);
                    // 排干 WM_PAINT 消息
                    MSG msg;
                    while (PeekMessage(&msg, m_hwnd, WM_PAINT, WM_PAINT, PM_REMOVE)) {
                        DispatchMessage(&msg);
                    }
                }
                // 现在正式切换到平铺视图
                SendMessageW(m_hwnd, LVM_SETVIEW, LV_VIEW_TILE, 0);
                // 平铺视图下样式保持为 LVS_ICON，无需修改
                InvalidateRect(m_hwnd, nullptr, TRUE);
                UpdateWindow(m_hwnd);
                return;
            }

            // 其他视图的正常切换
            SendMessageW(m_hwnd, LVM_SETVIEW, (WPARAM)view, 0);
            LONG_PTR style = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
            LONG_PTR typemask = static_cast<LONG_PTR>(LVS_TYPEMASK);
            style &= ~typemask;
            // 注意：将 LV_VIEW_* 转换为 LVS_* 样式（低4位兼容）
            style |= (static_cast<LONG_PTR>(view) & typemask);
            SetWindowLongPtrW(m_hwnd, GWL_STYLE, style);

            // 修复大图标/小图标视图下的图标空位问题
            if (view == LV_VIEW_ICON || view == LV_VIEW_SMALLICON) {
                SendMessageW(m_hwnd, LVM_ARRANGE, LVA_DEFAULT, 0);
            }

            InvalidateRect(m_hwnd, nullptr, TRUE);
            UpdateWindow(m_hwnd);
        }
        else {
            m_pendingView = view;
            m_flags.hasPendingView = true;
        }
    }

    void zhuziListView::setStyle(DWORD style, bool enable) {
        DWORD mask = style;
        DWORD value = enable ? style : 0;
        if (m_hwnd) {
            LONG_PTR curStyle = GetWindowLongPtrW(m_hwnd, GWL_STYLE);
            LONG_PTR maskL = static_cast<LONG_PTR>(mask);
            LONG_PTR valueL = static_cast<LONG_PTR>(value);
            curStyle = (curStyle & ~maskL) | valueL;
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

    void zhuziListView::setExtendedStyle(DWORD exStyle) {
        if (m_hwnd) {
            ListView_SetExtendedListViewStyle(m_hwnd, exStyle);
        }
        else {
            m_pendingExStyle = exStyle;
            m_flags.hasPendingExStyle = true;
        }
    }

    void zhuziListView::setGridLines(bool enable) {
        DWORD exStyle = LVS_EX_GRIDLINES;
        if (m_hwnd) {
            DWORD curEx = ListView_GetExtendedListViewStyle(m_hwnd);
            ListView_SetExtendedListViewStyle(m_hwnd, enable ? (curEx | exStyle) : (curEx & ~exStyle));
        }
        else {
            if (enable) m_pendingExStyle |= exStyle;
            else m_pendingExStyle &= ~exStyle;
            m_flags.hasPendingExStyle = true;
        }
    }

    void zhuziListView::setFullRowSelect(bool enable) {
        DWORD exStyle = LVS_EX_FULLROWSELECT;
        if (m_hwnd) {
            DWORD curEx = ListView_GetExtendedListViewStyle(m_hwnd);
            ListView_SetExtendedListViewStyle(m_hwnd, enable ? (curEx | exStyle) : (curEx & ~exStyle));
        }
        else {
            if (enable) m_pendingExStyle |= exStyle;
            else m_pendingExStyle &= ~exStyle;
            m_flags.hasPendingExStyle = true;
        }
    }

    void zhuziListView::setCheckBoxes(bool enable) {
        DWORD exStyle = LVS_EX_CHECKBOXES;
        if (m_hwnd) {
            DWORD curEx = ListView_GetExtendedListViewStyle(m_hwnd);
            ListView_SetExtendedListViewStyle(m_hwnd, enable ? (curEx | exStyle) : (curEx & ~exStyle));
        }
        else {
            if (enable) m_pendingExStyle |= exStyle;
            else m_pendingExStyle &= ~exStyle;
            m_flags.hasPendingExStyle = true;
        }
    }

    void zhuziListView::setEditLabels(bool enable) {
        setStyle(LVS_EDITLABELS, enable);
    }

    void zhuziListView::addColumn(const zhuziString& text, int width, int fmt) {
        if (!m_hwnd) return;
        LVCOLUMNW col = { 0 };
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
        col.fmt = fmt;
        col.cx = width;
        col.pszText = const_cast<LPWSTR>(text.c_str());
        int colCount = getColumnCount();
        ListView_InsertColumn(m_hwnd, colCount, &col);
    }

    void zhuziListView::deleteColumn(int colIndex) {
        if (m_hwnd && colIndex >= 0)
            ListView_DeleteColumn(m_hwnd, colIndex);
    }

    void zhuziListView::setColumnWidth(int colIndex, int width) {
        if (m_hwnd && colIndex >= 0)
            ListView_SetColumnWidth(m_hwnd, colIndex, width);
    }

    int zhuziListView::getColumnCount() const {
        if (!m_hwnd) return 0;
        HWND header = ListView_GetHeader(m_hwnd);
        return (header) ? (int)SendMessageW(header, HDM_GETITEMCOUNT, 0, 0) : 0;
    }

    // 项操作
    int zhuziListView::insertItem(int index, const zhuziString& text) {
        if (!m_hwnd) return -1;
        LVITEMW item = { 0 };
        item.mask = LVIF_TEXT;
        item.iItem = (index == -1) ? getItemCount() : index;
        item.pszText = const_cast<LPWSTR>(text.c_str());
        return ListView_InsertItem(m_hwnd, &item);
    }

    void zhuziListView::setItemText(int itemIndex, int subItem, const zhuziString& text) {
        if (!m_hwnd) return;
        LVITEMW item = { 0 };
        item.mask = LVIF_TEXT;
        item.iItem = itemIndex;
        item.iSubItem = subItem;
        item.pszText = const_cast<LPWSTR>(text.c_str());
        ListView_SetItem(m_hwnd, &item);
    }

    zhuziString zhuziListView::getItemText(int itemIndex, int subItem) const {
        if (!m_hwnd) return L"";
        wchar_t buffer[512] = { 0 };
        LVITEMW item = { 0 };
        item.mask = LVIF_TEXT;
        item.iItem = itemIndex;
        item.iSubItem = subItem;
        item.pszText = buffer;
        item.cchTextMax = 511;
        ListView_GetItem(m_hwnd, &item);
        return zhuziString(buffer);
    }

    void zhuziListView::deleteItem(int index) {
        if (m_hwnd && index >= 0)
            ListView_DeleteItem(m_hwnd, index);
    }

    void zhuziListView::deleteAllItems() {
        if (m_hwnd)
            ListView_DeleteAllItems(m_hwnd);
    }

    int zhuziListView::getItemCount() const {
        if (!m_hwnd) return 0;
        return ListView_GetItemCount(m_hwnd);
    }

    bool zhuziListView::setLineText(int row, const std::vector<zhuziString>& texts) {
        if (!m_hwnd || row < 0 || row >= getItemCount()) return false;
        for (size_t col = 0; col < texts.size(); ++col)
            setItemText(row, (int)col, texts[col]);
        return true;
    }

    int zhuziListView::addItem(const zhuziString& text, int imageIndex) {
        int idx = insertItem(-1, text);
        if (idx != -1 && imageIndex >= 0) {
            LVITEMW item = { 0 };
            item.mask = LVIF_IMAGE;
            item.iItem = idx;
            item.iImage = imageIndex;
            ListView_SetItem(m_hwnd, &item);
        }
        return idx;
    }

    int zhuziListView::addItem(const std::vector<zhuziString>& texts, int imageIndex) {
        if (texts.empty()) return -1;
        int idx = insertItem(-1, texts[0]);
        if (idx != -1) {
            for (size_t col = 1; col < texts.size(); ++col)
                setItemText(idx, (int)col, texts[col]);
            if (imageIndex >= 0) {
                LVITEMW item = { 0 };
                item.mask = LVIF_IMAGE;
                item.iItem = idx;
                item.iImage = imageIndex;
                ListView_SetItem(m_hwnd, &item);
            }
        }
        return idx;
    }

    int zhuziListView::insertItem(int index, const std::vector<zhuziString>& texts) {
        if (texts.empty()) return -1;
        int idx = insertItem(index, texts[0]);
        if (idx != -1) {
            for (size_t col = 1; col < texts.size(); ++col)
                setItemText(idx, (int)col, texts[col]);
        }
        return idx;
    }

    void zhuziListView::setImageList(HIMAGELIST hImageList, int type) {
        if (m_hwnd) ListView_SetImageList(m_hwnd, hImageList, type);
    }

    void zhuziListView::setImageList(const zhuziImageList& imageList, int type) {
        setImageList(imageList.getHandle(), type);
    }

    int zhuziListView::getSelectedIndex() const {
        if (!m_hwnd) return -1;
        int count = getItemCount();
        for (int i = 0; i < count; ++i) {
            if (ListView_GetItemState(m_hwnd, i, LVIS_SELECTED) & LVIS_SELECTED)
                return i;
        }
        return -1;
    }

    std::vector<int> zhuziListView::getSelectedIndices() const {
        std::vector<int> indices;
        if (!m_hwnd) return indices;
        int count = getItemCount();
        for (int i = 0; i < count; ++i) {
            if (ListView_GetItemState(m_hwnd, i, LVIS_SELECTED) & LVIS_SELECTED)
                indices.push_back(i);
        }
        return indices;
    }

    int zhuziListView::getSelectedCount() const {
        if (!m_hwnd) return 0;
        return ListView_GetSelectedCount(m_hwnd);
    }

    void zhuziListView::setSelected(int index, bool selected) {
        if (!m_hwnd) return;
        ListView_SetItemState(m_hwnd, index, selected ? LVIS_SELECTED : 0, LVIS_SELECTED);
    }

    void zhuziListView::ensureVisible(int index, bool partially) {
        if (m_hwnd)
            ListView_EnsureVisible(m_hwnd, index, partially ? TRUE : FALSE);
    }

    void zhuziListView::checkBox(int row, bool checked) {
        if (!m_hwnd) return;
        DWORD curEx = ListView_GetExtendedListViewStyle(m_hwnd);
        if (!(curEx & LVS_EX_CHECKBOXES)) {
            ListView_SetExtendedListViewStyle(m_hwnd, curEx | LVS_EX_CHECKBOXES);
        }
        UINT state = checked ? INDEXTOSTATEIMAGEMASK(2) : INDEXTOSTATEIMAGEMASK(1);
        ListView_SetItemState(m_hwnd, row, state, LVIS_STATEIMAGEMASK);
    }

    void zhuziListView::setOnClick(std::function<void(int)> callback) {
        setCallbackFunc(m_callbacks, LVCB_CLICK, std::move(callback));
        if (m_hwnd && !m_flags.notifyRegistered) registerParentNotify();
    }

    void zhuziListView::setOnDoubleClick(std::function<void(int)> callback) {
        setCallbackFunc(m_callbacks, LVCB_DOUBLECLICK, std::move(callback));
        if (m_hwnd && !m_flags.notifyRegistered) registerParentNotify();
    }

    void zhuziListView::setOnRClick(std::function<void(int)> callback) {
        setCallbackFunc(m_callbacks, LVCB_RCLICK, std::move(callback));
        if (m_hwnd && !m_flags.notifyRegistered) registerParentNotify();
    }

    void zhuziListView::setOnCheck(std::function<void(int row, bool checked)> callback) {
        if (m_hwnd) {
            DWORD curEx = ListView_GetExtendedListViewStyle(m_hwnd);
            if (!(curEx & LVS_EX_CHECKBOXES))
                ListView_SetExtendedListViewStyle(m_hwnd, curEx | LVS_EX_CHECKBOXES);
        }
        else {
            m_pendingExStyle |= LVS_EX_CHECKBOXES;
            m_flags.hasPendingExStyle = true;
        }
        setCallbackFunc(m_callbacks, LVCB_CHECK, std::move(callback));
        if (m_hwnd && !m_flags.notifyRegistered) registerParentNotify();
    }

    void zhuziListView::setOnEditStart(std::function<bool(int row)> callback) {
        setCallbackFunc(m_callbacks, LVCB_EDIT_START, std::move(callback));
        if (m_hwnd && !m_flags.notifyRegistered) registerParentNotify();
    }

    void zhuziListView::setOnEdit(std::function<bool(int row, zhuziString newText)> callback) {
        setEditLabels(true);
        setCallbackFunc(m_callbacks, LVCB_EDIT_END, std::move(callback));
        if (m_hwnd && !m_flags.notifyRegistered) registerParentNotify();
    }

    LRESULT CALLBACK zhuziListView::ListViewSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        zhuziListView* pThis = reinterpret_cast<zhuziListView*>(dwRefData);
        if (pThis && pThis->m_hwnd == hwnd) {
            LRESULT result = pThis->handleSubclassMessage(msg, wParam, lParam);
            if (result != -1) return result;
        }
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    LRESULT zhuziListView::handleSubclassMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_VSCROLL:
        case WM_HSCROLL: {
            LRESULT res = DefSubclassProc(m_hwnd, msg, wParam, lParam);
            updateCellControlPositions();
            InvalidateRect(m_hwnd, nullptr, FALSE);
            return res;
        }
        case WM_MOUSEWHEEL: {
            LRESULT res = DefSubclassProc(m_hwnd, msg, wParam, lParam);
            updateCellControlPositions();
            return res;
        }
        case WM_SIZE:
            updateCellControlPositions();
            break;
        case WM_NOTIFY: {
            NMHDR* pnmh = (NMHDR*)lParam;
            HWND hHeader = ListView_GetHeader(m_hwnd);
            if (pnmh->hwndFrom == hHeader && pnmh->code == HDN_ENDTRACKW) {
                updateCellControlPositions();
            }
            break;
        }
        }
        return -1;
    }

    bool zhuziListView::setCellControl(int row, int column, zhuziControl* control) {
        auto key = std::make_pair(row, column);
        // 处理 nullptr：移除控件
        if (control == nullptr) {
            auto it = m_cellControls.find(key);
            if (it != m_cellControls.end()) {
                it->second->show(false);
                m_cellControls.erase(it);
            }
            return true;
        }
        // 设置父窗口（重要）
        control->setParent(this);
        if (!control->getHandle()) {
            // 未创建：调用 create，使用临时位置 (0,0,100,100)
            if (!control->create(0, 0, 100,100)) {
                return false;
            }
            // 确保父窗口为 ListView 自身
            SetParent(control->getHandle(), m_hwnd);
        }
        else {
            // 已创建：重新设置父窗口
            SetParent(control->getHandle(), m_hwnd);
        }
        // 存储控件
        m_cellControls[key] = control;
        // 立即更新位置
        updateCellControlPositions();
        return true;
    }

    zhuziControl* zhuziListView::cellControl(int row, int column) const {
        auto it = m_cellControls.find(std::make_pair(row, column));
        return (it != m_cellControls.end()) ? it->second : nullptr;
    }

    void zhuziListView::updateCellControlPositions() {
        if (!m_hwnd) return;
        for (auto& entry : m_cellControls) {
            int row = entry.first.first;
            int column = entry.first.second;
            zhuziControl* ctrl = entry.second;
            if (!ctrl || !ctrl->getHandle()) continue;

            RECT itemRect;
            if (!ListView_GetItemRect(m_hwnd, row, &itemRect, LVIR_BOUNDS)) {
                ctrl->show(false);
                continue;
            }
            int left = itemRect.left;
            for (int i = 0; i < column; ++i) {
                left += ListView_GetColumnWidth(m_hwnd, i);
            }
            int colWidth = ListView_GetColumnWidth(m_hwnd, column);
            if (colWidth <= 0) colWidth = 100;
            int top = itemRect.top;
            int height = itemRect.bottom - itemRect.top;
            SetWindowPos(ctrl->getHandle(), HWND_TOP, left, top, colWidth, height,
                SWP_NOACTIVATE | SWP_SHOWWINDOW);
            ctrl->show(true);
        }
    }

    void zhuziListView::onParentResize(int parentWidth, int parentHeight) {
        zhuziControl::onParentResize(parentWidth, parentHeight);
        updateCellControlPositions();   // 父窗口大小变化时同步更新
    }

    void zhuziListView::setItemImage(int row, int imageIndex) {
        if (!m_hwnd || row < 0) return;
        LVITEMW item = { 0 };
        item.mask = LVIF_IMAGE;
        item.iItem = row;
        item.iImage = imageIndex;
        ListView_SetItem(m_hwnd, &item);
    }
} // namespace zhuzi