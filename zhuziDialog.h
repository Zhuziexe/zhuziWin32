#pragma once
#include "zhuziControl.h"
#include <windows.h>
#include <functional>
#include <unordered_map>

namespace zhuzi {

    class zhuziDialog {
    public:
        zhuziDialog(zhuziControl* parent = nullptr);
        virtual ~zhuziDialog();

        // 从资源加载对话框并显示（模态或非模态）
        bool loadDialog(int resourceId, bool modal = false);
        void closeDialog();

        // 设置对话框标题（仅对非模态有效）
        void setTitle(const zhuziString& title);

        /**
        * @brief 设置按钮点击回调
        * 
        * 注意:先设置回调再创建模态窗口
        */
        void setOnClick(int controlId, std::function<void()> callback);

        // 获取控件句柄
        HWND getControlHandle(int controlId) const;

        void setControlText(int controlId, const zhuziString& text);
        zhuziString getControlText(int controlId) const;

        // 结束模态对话框并返回结果
        void endDialog(int result = IDOK);

    private:
        zhuziControl* m_parent;
        HWND m_hwnd;                 // 对话框窗口句柄
        int m_resourceId;
        bool m_modal;
        std::unordered_map<int, std::function<void()>> m_clickHandlers;

        static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        INT_PTR handleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

        void onCommand(WORD ctrlId);
        void centerWindow();
    };

} // namespace zhuzi