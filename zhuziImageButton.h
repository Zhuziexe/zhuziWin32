#pragma once
#include "zhuziControl.h"
#include <functional>

namespace zhuzi {

    class zhuziImageButton : public zhuziControl {
    public:
        zhuziImageButton(zhuziControl* parent = nullptr);
        virtual ~zhuziImageButton();

        virtual bool onCreate(DWORD style) override;
        virtual void onPaint(zhuziPaint& paint) override;
        virtual void onMouseMove(int x, int y) override;
        virtual void onMouseLeave() override;
        virtual void onLButtonUp(int x, int y) override;

        // 文字
        void setText(const zhuziString& text);
        zhuziString getText() const;

        // 图像（普通状态）
        void setImageNormal(const zhuziString& imagePath);
        void setImageNormal(int resourceId, const wchar_t* resourceType = L"PNG");
        void setImageHover(const zhuziString& imagePath);
        void setImageHover(int resourceId, const wchar_t* resourceType = L"PNG");
        void clearImages();

        // 图像缩放模式
        enum class ImageScaleMode { None, Fit, Fill, Center };
        void setImageScaleMode(ImageScaleMode mode);
        ImageScaleMode getImageScaleMode() const;

        // 文字与图像间距
        void setImageTextSpacing(int spacing);
        int getImageTextSpacing() const;

        // 是否仅显示图像
        void setImageOnly(bool imageOnly);
        bool isImageOnly() const;

        // 背景色
        void setBackColor(COLORREF color);
        void resetBackColor();

        // 点击事件
        void setOnClick(std::function<void()> callback);

    protected:
        void updateState(int x, int y);
        void redraw();
        void drawImage(zhuziPaint& paint, HBITMAP hBitmap, int x, int y, int width, int height);
        void drawContent(zhuziPaint& paint);
        void ensureMouseTracking();
        void updateBackgroundBrush();   // 声明
        void loadImage(const zhuziString& path, HBITMAP& target, bool& ownFlag);
        void loadImage(int resourceId, const wchar_t* resourceType, HBITMAP& target, bool& ownFlag);

    private:
        enum class State { Normal, Hover };
        State m_state;
        zhuziString m_text;
        HBITMAP m_hBitmapNormal;
        HBITMAP m_hBitmapHover;
        bool m_ownNormal;
        bool m_ownHover;
        ImageScaleMode m_scaleMode;
        int m_imageTextSpacing;
        bool m_imageOnly;
        COLORREF m_backColor;
        HBRUSH m_hBackBrush;
        std::function<void()> m_onClick;
        bool m_trackingMouse;
    };

} // namespace zhuzi