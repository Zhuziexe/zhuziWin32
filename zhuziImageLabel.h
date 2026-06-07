#pragma once
#include "zhuziControl.h"
#include "zhuziImage.h"
#include <functional>
#include <memory>

namespace zhuzi {

    class zhuziImageLabel : public zhuziControl {
    public:
        enum class ScaleMode {
            None,       // 原尺寸居中
            Fit,        // 保持比例缩放，完全显示（可能留白）
            Fill,       // 保持比例缩放，填满控件（可能裁剪）
            Stretch     // 拉伸填满（可能变形）
        };

        zhuziImageLabel(zhuziControl* parent = nullptr);
        ~zhuziImageLabel();

        virtual bool onCreate(DWORD style) override;

        // 图像加载
        bool loadImage(const zhuziString& filePath);
        bool loadImage(int resourceId, const wchar_t* resourceType = L"PNG");
        bool loadImageFromMemory(const void* data, size_t size);
        void clearImage();

        // 设置缩放模式
        void setScaleMode(ScaleMode mode);
        ScaleMode getScaleMode() const;

        // 背景色（当图像未填满或透明时显示）
        void setBackgroundColor(COLORREF color);
        COLORREF getBackgroundColor() const;

        // 获取原始图像尺寸
        SIZE getImageSize() const;

    protected:
        virtual void onPaint(zhuziPaint& paint) override;

    private:
        zhuziImage m_image;
        ScaleMode m_scaleMode;
        COLORREF m_bgColor;
        HBRUSH m_hBgBrush;

        void createBrush();
        void updateImage();
        static LRESULT CALLBACK StaticProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        void drawContent(zhuziPaint& paint);
    };

} // namespace zhuzi