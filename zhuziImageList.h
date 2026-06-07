#pragma once
#include <windows.h>
#include <commctrl.h>
#include <vector>
#include <functional>
#include <memory>
#include <gdiplus.h>          // 必须包含，用于 Gdiplus::Image
#include "zhuziString.h"

namespace zhuzi {

    class zhuziImage;  // 前向声明

    class zhuziImageList {
    public:
        enum class ColorDepth : char {
            Default = 0,
            Mono = 1,
            _4Bit = 4,
            _8Bit = 8,
            _16Bit = 16,
            _24Bit = 24,
            _32Bit = 32
        };

        enum class DrawStyle : char {
            Normal,
            Transparent,
            Selected,
            Focused
        };

        struct CreateParams {
            int width;
            int height;
            ColorDepth colorDepth = ColorDepth::Default;
            int initialCount = 4;
            int growCount = 4;
            bool useMask = false;
        };

        zhuziImageList();
        ~zhuziImageList();

        zhuziImageList(const zhuziImageList&) = delete;
        zhuziImageList& operator=(const zhuziImageList&) = delete;
        zhuziImageList(zhuziImageList&& other) noexcept;
        zhuziImageList& operator=(zhuziImageList&& other) noexcept;

        bool create(const CreateParams& params);
        bool create(int width, int height, ColorDepth depth = ColorDepth::Default,
            int initialCount = 4, int growCount = 4);
        void destroy();
        bool isCreated() const { return m_hImageList != nullptr; }
        HIMAGELIST getHandle() const { return m_hImageList; }

        int getWidth() const;
        int getHeight() const;
        int getImageCount() const;

        // 添加图像（自动缩放）
        int add(HBITMAP hBitmap, HBITMAP hMask = nullptr);
        int add(HICON hIcon);
		int add(const zhuziImage& image);
        int addFromMemory(const void* data, size_t size);
        int addFromFile(const wchar_t* filePath);
        int addFromResource(int resourceId, const wchar_t* resourceType = L"PNG");
        int addFromGdiplusImage(Gdiplus::Image* image);   // 保留兼容性
        int addSolidColor(int width, int height, COLORREF color);

        bool insert(int index, HBITMAP hBitmap, HBITMAP hMask = nullptr);
        bool replace(int index, HBITMAP hBitmap, HBITMAP hMask = nullptr);
        bool replace(int index, HICON hIcon);
        bool remove(int index);
        void removeAll();

        bool draw(HDC hdc, int x, int y, int imageIndex, DrawStyle style = DrawStyle::Normal) const;
        bool draw(HDC hdc, int x, int y, int width, int height, int imageIndex) const;
        bool drawEx(HDC hdc, int x, int y, int dx, int dy,
            int imageIndex, COLORREF rgbBk = CLR_NONE, COLORREF rgbFg = CLR_NONE,
            UINT style = ILD_NORMAL) const;

        HICON extractIcon(int imageIndex) const;
        HBITMAP extractBitmap(int imageIndex) const;

        bool setOverlayImage(int overlayIndex, int imageIndex);
        bool drawOverlay(HDC hdc, int x, int y, int imageIndex, int overlayIndex) const;

        void setBkColor(COLORREF clrBk);
        COLORREF getBkColor() const;
        bool copyToClipboard(HWND hwndOwner, int imageIndex) const;
        int pasteFromClipboard();

    private:
        HIMAGELIST m_hImageList;
        int m_width, m_height;


        // 内部缩放辅助
        HBITMAP scaleBitmap(HBITMAP hSrc, int targetW, int targetH) const;
    };

} // namespace zhuzi