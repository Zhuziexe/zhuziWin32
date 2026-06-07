#pragma once
#include <windows.h>
#include <wincodec.h>
#include "zhuziString.h"

namespace zhuzi {

    class zhuziImage {
    public:
        zhuziImage();
        ~zhuziImage();

        zhuziImage(const zhuziImage&) = delete;
        zhuziImage& operator=(const zhuziImage&) = delete;
        zhuziImage(zhuziImage&& other) noexcept;
        zhuziImage& operator=(zhuziImage&& other) noexcept;
        
        zhuziImage(const zhuziString& filePath);
		zhuziImage(int resourceId, const wchar_t* resourceType = L"PNG");

        bool loadFromFile(const zhuziString& filePath);
        bool loadFromResource(int resourceId, const wchar_t* resourceType = L"PNG");
        bool loadFromMemory(const void* data, size_t size);
        bool loadFromHBITMAP(HBITMAP hBitmap);   // 总是深拷贝
        bool saveToFile(const zhuziString& filePath) const;

        int getWidth() const;
        int getHeight() const;
        SIZE getSize() const;
        bool isEmpty() const;

        HBITMAP toHBITMAP() const;   // 返回副本，调用者需 DeleteObject

        zhuziImage scale(int newWidth, int newHeight, bool highQuality = true) const;
        zhuziImage crop(int x, int y, int width, int height) const;
        zhuziImage rotate(float angleDegrees) const;   // 仅支持 90,180,270
        zhuziImage flip(bool horizontal, bool vertical) const;
        zhuziImage makeTransparent(COLORREF colorKey, BYTE alpha = 0) const;
        zhuziImage applyAlpha(BYTE globalAlpha) const;
        zhuziImage clone() const;

		HBITMAP getHandle() const { return m_hBitmap; }
    private:
        HBITMAP m_hBitmap;
        int m_width;
        int m_height;

        void destroy();
        void moveFrom(zhuziImage&& other);
        bool createFromWIC(IWICBitmapSource* pSource);
    };

} // namespace zhuzi