#pragma once
#include "zhuziControl.h"
#include <functional>
#include <memory>

namespace Gdiplus {
    class Image;
    class Graphics;
}

namespace zhuzi {

    class zhuziImageLabel : public zhuziControl {
    public:
        enum class ScaleMode {
            None,
            Fit,
            Fill,
            Stretch
        };

        zhuziImageLabel(zhuziControl* parent = nullptr);
        ~zhuziImageLabel();

        virtual bool onCreate(DWORD style) override;

        bool loadImage(const zhuziString& filePath);
        bool loadImage(int resourceId, const wchar_t* resourceType = L"PNG");
        bool loadImageFromMemory(const void* data, size_t size);
        void clearImage();

        void setScaleMode(ScaleMode mode);
        void setBackgroundColor(COLORREF color);
        void onPaint();

    protected:
        virtual void draw(Gdiplus::Graphics& graphics, const RECT& rect);

    private:
        void* m_image;          // Gdiplus::Image*
        ScaleMode m_scaleMode;
        COLORREF m_bgColor;
        HBRUSH m_hBgBrush;

        static ULONG_PTR m_gdiplusToken;
        static int m_gdiplusRefCount;

        void createBrush();
        void updateImage();
        static void initGdiplus();
        static void shutdownGdiplus();
        static LRESULT CALLBACK StaticProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    };

}