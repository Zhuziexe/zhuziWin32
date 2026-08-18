#pragma once
#include "zhuziControl.h"
#include <functional>
#include <memory>

namespace zhuzi {

    class zhuziDialog;

    // 对话框容器控件（继承 zhuziControl，用于放置子控件）
    class zhuziDialogWindow : public zhuziControl {
    public:
        zhuziDialogWindow();
        virtual ~zhuziDialogWindow();

        // 关联到已有对话框窗口句柄（由 DialogBox 创建）
        void attach(HWND hwnd);

        // 重写：不调整自身位置，只调整子控件
        virtual void onParentResize(int parentWidth, int parentHeight) override;

        // 实现纯虚函数：容器本身不需要创建窗口，返回 true 即可
        virtual bool onCreate(DWORD style) override;

        // 获取所属的 zhuziDialog 对象
        zhuziDialog* getDialog() const;

        // 设置所属对话框指针（由 zhuziDialog 在初始化时调用）
        void setDialog(zhuziDialog* dlg);

        // 是否启用自定义绘制（默认 false，避免干扰标准对话框外观）
        virtual bool getTransparent() const override { return false; }

    private:
        zhuziDialog* m_dialog;
    };

    // 模态对话框管理类
    class zhuziDialog {
    public:
        zhuziDialog();
        ~zhuziDialog();

        // 禁止拷贝
        zhuziDialog(const zhuziDialog&) = delete;
        zhuziDialog& operator=(const zhuziDialog&) = delete;

        // 设置初始化回调函数（在 WM_INITDIALOG 中调用）
        void setInitProc(std::function<void(zhuziDialog* dlg, zhuziDialogWindow* wnd)> initProc);

        // 创建并显示模态对话框
        // parent: 父窗口（拥有者），可为 nullptr
        // width, height: 对话框客户区尺寸（单位：像素）
        // title: 窗口标题
        // 返回 EndDialog 传入的结果（如 IDOK / IDCANCEL）
        int createModal(zhuziControl* parent, int width, int height, const zhuziString& title, \
            DWORD dwStyle = WS_POPUP | WS_BORDER | WS_SYSMENU | WS_CAPTION | DS_MODALFRAME | DS_CENTER);

        // 结束对话框（由用户或默认按钮调用）
        void endDialog(int result);

        // 获取对话框窗口句柄
        HWND getHandle() const;

        // 获取对话框容器控件
        zhuziDialogWindow* getWindow() { return &m_wnd; }

    private:
        static INT_PTR CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        static HGLOBAL BuildDialogTemplate(int width, int height, const zhuziString& title,DWORD dlgStyle);

        zhuziDialogWindow m_wnd;
        std::function<void(zhuziDialog*, zhuziDialogWindow*)> m_initProc;
        int m_result;
        bool m_ended;
    };

} // namespace zhuzi