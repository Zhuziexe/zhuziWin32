// zhuziMenu.cpp
#include "zhuziMenu.h"
#include "zhuziControl.h"   // 为了使用 zhuziWindow 及其 getHandle
#include <stdexcept>

namespace zhuzi {

    // ==================== zhuziMenu 静态成员 ====================
    int zhuziMenu::s_nextCmdId = 30000;

    int zhuziMenu::allocateCmdId() {
        if (s_nextCmdId >= 40000) {
            throw std::out_of_range("No available menu command ID (range 30000-39999)");
        }
        return s_nextCmdId++;
    }

    void zhuziMenu::releaseCmdId(int /*id*/) {
        // 简单实现不回收 ID，保持递增
    }

    // ==================== zhuziMenu ====================
    zhuziMenu::zhuziMenu(bool isPopup)
        : m_hMenu(nullptr), m_isPopup(isPopup) {
        m_hMenu = m_isPopup ? CreatePopupMenu() : CreateMenu();
        if (!m_hMenu) {
            throw std::runtime_error("Failed to create menu");
        }
    }

    zhuziMenu::~zhuziMenu() {
        if (m_hMenu) {
            DestroyMenu(m_hMenu);
            m_hMenu = nullptr;
        }
    }

    int zhuziMenu::addItem(const zhuziString& text, int cmdId) {
        if (!m_hMenu) return -1;

        if (cmdId < 0) {
            cmdId = allocateCmdId();
        }

        UINT flags = MF_STRING;
        if (text.empty()) {
            flags = MF_SEPARATOR;
        }

        if (!AppendMenuW(m_hMenu, flags, cmdId, text.c_str())) {
            releaseCmdId(cmdId);
            return -1;
        }
        return cmdId;
    }

    void zhuziMenu::addSeparator() {
        if (m_hMenu) {
            AppendMenuW(m_hMenu, MF_SEPARATOR, 0, nullptr);
        }
    }

    zhuziMenu* zhuziMenu::addSubMenu(const zhuziString& text, zhuziMenu* subMenu) {
        if (!m_hMenu || !subMenu || !subMenu->m_hMenu) return nullptr;
        // 子菜单必须是弹出菜单
        if (!subMenu->m_isPopup) {
            // 可以强制转换，但最好检查
            return nullptr;
        }
        AppendMenuW(m_hMenu, MF_POPUP | MF_STRING, (UINT_PTR)subMenu->m_hMenu, text.c_str());
        return subMenu;
    }

    void zhuziMenu::setItemEnabled(int cmdId, bool enabled) {
        if (m_hMenu) {
            EnableMenuItem(m_hMenu, cmdId, MF_BYCOMMAND | (enabled ? MF_ENABLED : MF_GRAYED));
        }
    }

    void zhuziMenu::setItemChecked(int cmdId, bool checked) {
        if (m_hMenu) {
            CheckMenuItem(m_hMenu, cmdId, MF_BYCOMMAND | (checked ? MF_CHECKED : MF_UNCHECKED));
        }
    }

    void zhuziMenu::setItemText(int cmdId, const zhuziString& text) {
        if (m_hMenu) {
            ModifyMenuW(m_hMenu, cmdId, MF_BYCOMMAND | MF_STRING, cmdId, text.c_str());
        }
    }

    int zhuziMenu::getItemCount() const {
        if (!m_hMenu) return 0;
        return GetMenuItemCount(m_hMenu);
    }

    void zhuziMenu::trackPopupMenu(HWND hWndOwner, int x, int y) {
        if (m_hMenu && m_isPopup) {
            if (!IsWindow(hWndOwner)) return;  // 确保窗口有效
            TrackPopupMenu(m_hMenu, TPM_LEFTALIGN | TPM_TOPALIGN, x, y, 0, hWndOwner, nullptr);
        }
    }

    // ==================== zhuziMainMenu 静态成员 ====================
    std::unordered_map<HWND, zhuziMainMenu*> zhuziMainMenu::s_windowMenuMap;
    std::unordered_map<HWND, bool> zhuziMainMenu::s_windowHooked;

    // 静态消息处理函数，通过 zhuziWindow 的 BindChain 注册
    bool zhuziMainMenu::MenuMessageHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM /*lParam*/) {
        if (msg != WM_COMMAND) return false;

        auto it = s_windowMenuMap.find(hwnd);
        if (it == s_windowMenuMap.end() || it->second == nullptr) {
            return false;   // 没有活动菜单，继续传递
        }

        int cmdId = LOWORD(wParam);
        return it->second->handleCommand(cmdId);
    }

    // ==================== zhuziMainMenu ====================
    zhuziMainMenu::zhuziMainMenu()
        : zhuziMenu(false)   // 主菜单栏，不是弹出菜单
        , m_attachedWindow(nullptr) {
    }

    zhuziMainMenu::~zhuziMainMenu() {
        detach();
    }

    void zhuziMainMenu::attachToWindow(zhuziWindow* window) {
        if (!window) return;
        HWND hwnd = window->getHandle();
        if (!hwnd) return;

        // 如果已经附加到同一个窗口，不做任何事
        if (m_attachedWindow == window) return;

        // 先 detach 当前菜单（如果已附加到其他窗口）
        detach();

        // 设置窗口菜单
        if (!SetMenu(hwnd, m_hMenu)) {
            return;
        }

        // 如果该窗口还没有安装静态消息钩子，则绑定一次
        if (!s_windowHooked[hwnd]) {
            window->Bind(WM_COMMAND, [hwnd](zhuziMessage& msg) -> bool {
                return MenuMessageHandler(hwnd, WM_COMMAND, msg.wParam, msg.lParam);
                });
            s_windowHooked[hwnd] = true;
        }

        // 记录窗口对应的活动主菜单
        s_windowMenuMap[hwnd] = this;
        m_attachedWindow = window;
    }

    void zhuziMainMenu::detach() {
        if (m_attachedWindow) {
            HWND hwnd = m_attachedWindow->getHandle();
            if (hwnd && IsWindow(hwnd)) {
                // 移除窗口上的菜单栏
                SetMenu(hwnd, nullptr);
                // 从映射表中删除（但不清除静态钩子，因为可能其他菜单后续再 attach）
                s_windowMenuMap.erase(hwnd);
            }
            m_attachedWindow = nullptr;
        }
    }

    void zhuziMainMenu::setOnClick(int cmdId, std::function<void()> callback) {
        if (cmdId >= 0) {
            m_callbacks[cmdId] = callback;
        }
    }

    void zhuziMainMenu::removeOnClick(int cmdId) {
        m_callbacks.erase(cmdId);
    }

    bool zhuziMainMenu::handleCommand(int cmdId) {
        auto it = m_callbacks.find(cmdId);
        if (it != m_callbacks.end() && it->second) {
            it->second();
            return true;   // 消息已处理
        }
        return false;      // 未处理，继续传递
    }
}