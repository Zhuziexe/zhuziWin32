// zhuziMenu.h
#pragma once
#include "zhuziString.h"
#include <windows.h>
#include <functional>
#include <unordered_map>
#include <memory>

namespace zhuzi {
    class zhuziWindow;

    // 基础菜单类（可作弹出菜单或子菜单）
    class zhuziMenu {
    public:
        // isPopup: true 创建弹出菜单（用于子菜单或右键菜单），false 创建普通菜单（用于主菜单栏）
        zhuziMenu(bool isPopup = false);
        virtual ~zhuziMenu();

        // 禁止拷贝，但允许移动
        zhuziMenu(const zhuziMenu&) = delete;
        zhuziMenu& operator=(const zhuziMenu&) = delete;

        // 获取原始 HMENU 句柄
        HMENU getHandle() const { return m_hMenu; }

        // 添加菜单项，自动分配 ID 或使用指定 ID（若 cmdId >= 0）
        // 返回实际菜单项 ID
        int addItem(const zhuziString& text, int cmdId = -1);
        void addSeparator();

        // 添加子菜单（text 为菜单项文本，subMenu 必须是弹出菜单类型）
        // 返回 subMenu 自身，方便链式调用
        zhuziMenu* addSubMenu(const zhuziString& text, zhuziMenu* subMenu);

        // 启用/禁用菜单项
        void setItemEnabled(int cmdId, bool enabled);
        // 设置/清除选中标记（复选标记）
        void setItemChecked(int cmdId, bool checked);
        // 修改菜单项文本
        void setItemText(int cmdId, const zhuziString& text);

        // 获取菜单项数量（顶层项）
        int getItemCount() const;

        // 弹出右键菜单（仅对弹出菜单有效）
        // hWndOwner: 父窗口句柄，x, y: 屏幕坐标
        void trackPopupMenu(HWND hWndOwner, int x, int y);

    protected:
        HMENU m_hMenu;
        bool m_isPopup;

        // 自动分配菜单项 ID（范围 30000-39999）
        static int allocateCmdId();
        static void releaseCmdId(int id);
        static int s_nextCmdId;
    };

    // 顶层主菜单（显示在窗口标题栏下方）
    class zhuziMainMenu : public zhuziMenu {
    public:
        zhuziMainMenu();
        virtual ~zhuziMainMenu();

        // 附加到某个窗口（将菜单栏设置到窗口上，并注册消息处理）
        // 如果窗口已有关联的主菜单，会先 detach
        void attachToWindow(zhuziWindow* window);
        void detach();

        // 为指定菜单项绑定点击回调
        void setOnClick(int cmdId, std::function<void()> callback);

        // 移除某个菜单项的回调
        void removeOnClick(int cmdId);

        // 获取当前附加的窗口（如果没有返回 nullptr）
        zhuziWindow* getAttachedWindow() const { return m_attachedWindow; }

    private:
        // 处理 WM_COMMAND 消息
        bool handleCommand(int cmdId);

        // 静态消息处理函数（供窗口链式调用）
        static bool MenuMessageHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        // 每个窗口当前活动的主菜单指针
        static std::unordered_map<HWND, zhuziMainMenu*> s_windowMenuMap;
        // 是否已为窗口安装了静态消息钩子（每个窗口只装一次）
        static std::unordered_map<HWND, bool> s_windowHooked;

        zhuziWindow* m_attachedWindow;
        std::unordered_map<int, std::function<void()>> m_callbacks;
    };
}