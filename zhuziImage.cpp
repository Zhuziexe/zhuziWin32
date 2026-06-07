#include "zhuziImage.h"
#include <wincodec.h>
#include <vector>
#include <cstring>
#include <cmath>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "gdi32.lib")

namespace zhuzi {

    static IWICImagingFactory* GetWICFactory() {
        static IWICImagingFactory* pFactory = []() -> IWICImagingFactory* {
            IWICImagingFactory* p = nullptr;
            CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                IID_IWICImagingFactory, (void**)&p);
            return p;
            }();
        return pFactory;
    }

    static zhuziString GetFileExtension(const zhuziString& path) {
        const wchar_t* p = path.c_str();
        const wchar_t* dot = nullptr;
        for (const wchar_t* q = p; *q; ++q) {
            if (*q == L'.') dot = q;
        }
        if (dot) return zhuziString(dot + 1);
        return L"";
    }

    zhuziImage::zhuziImage() : m_hBitmap(nullptr), m_width(0), m_height(0) {}
    zhuziImage::~zhuziImage() { destroy(); }

    void zhuziImage::destroy() {
        if (m_hBitmap) DeleteObject(m_hBitmap);
        m_hBitmap = nullptr;
        m_width = m_height = 0;
    }

    void zhuziImage::moveFrom(zhuziImage&& other) {
        m_hBitmap = other.m_hBitmap;
        m_width = other.m_width;
        m_height = other.m_height;
        other.m_hBitmap = nullptr;
        other.m_width = other.m_height = 0;
    }

    zhuziImage::zhuziImage(zhuziImage&& other) noexcept : m_hBitmap(nullptr), m_width(0), m_height(0) {
        moveFrom(std::move(other));
    }

    zhuziImage& zhuziImage::operator=(zhuziImage&& other) noexcept {
        if (this != &other) {
            destroy();
            moveFrom(std::move(other));
        }
        return *this;
    }

    zhuziImage::zhuziImage(const zhuziString& filePath)
        : m_hBitmap(nullptr), m_width(0), m_height(0) {
        loadFromFile(filePath);
	}

    zhuziImage::zhuziImage(int resourceId, const wchar_t* resourceType)
		:m_hBitmap(nullptr), m_width(0), m_height(0) {
		loadFromResource(resourceId, resourceType);
    }

    bool zhuziImage::createFromWIC(IWICBitmapSource* pSource) {
        if (!pSource) return false;
        UINT w, h;
        pSource->GetSize(&w, &h);
        BITMAPINFO bmi = { 0 };
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = w;
        bmi.bmiHeader.biHeight = -((int)h);
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        void* pBits = nullptr;
        HDC hdc = GetDC(nullptr);
        HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
        ReleaseDC(nullptr, hdc);
        if (!hBitmap) return false;

        UINT stride = w * 4;
        UINT bufferSize = h * stride;
        std::vector<BYTE> buffer(bufferSize);
        WICRect rect = { 0, 0, (INT)w, (INT)h };
        HRESULT hr = pSource->CopyPixels(&rect, stride, bufferSize, buffer.data());
        if (SUCCEEDED(hr)) {
            memcpy(pBits, buffer.data(), bufferSize);
            destroy();
            m_hBitmap = hBitmap;
            m_width = w;
            m_height = h;
            return true;
        }
        DeleteObject(hBitmap);
        return false;
    }

    bool zhuziImage::loadFromFile(const zhuziString& filePath) {
        IWICBitmapDecoder* pDecoder = nullptr;
        HRESULT hr = GetWICFactory()->CreateDecoderFromFilename(filePath.c_str(), nullptr,
            GENERIC_READ, WICDecodeMetadataCacheOnLoad, &pDecoder);
        if (FAILED(hr)) return false;

        IWICBitmapFrameDecode* pFrame = nullptr;
        hr = pDecoder->GetFrame(0, &pFrame);
        pDecoder->Release();
        if (FAILED(hr)) return false;

        bool ok = createFromWIC(pFrame);
        pFrame->Release();
        return ok;
    }

    bool zhuziImage::loadFromResource(int resourceId, const wchar_t* resourceType) {
        HINSTANCE hInst = GetModuleHandleW(nullptr);
        HRSRC hRsrc = FindResourceW(hInst, MAKEINTRESOURCEW(resourceId), resourceType);
        if (!hRsrc) return false;
        HGLOBAL hGlobal = LoadResource(hInst, hRsrc);
        if (!hGlobal) return false;
        DWORD size = SizeofResource(hInst, hRsrc);
        const void* data = LockResource(hGlobal);
        if (!data || size == 0) return false;

        IStream* pStream = nullptr;
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
        if (!hMem) return false;
        void* pMem = GlobalLock(hMem);
        memcpy(pMem, data, size);
        GlobalUnlock(hMem);
        if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) != S_OK) {
            GlobalFree(hMem);
            return false;
        }

        IWICBitmapDecoder* pDecoder = nullptr;
        HRESULT hr = GetWICFactory()->CreateDecoderFromStream(pStream, nullptr, WICDecodeMetadataCacheOnLoad, &pDecoder);
        pStream->Release();
        if (FAILED(hr)) return false;

        IWICBitmapFrameDecode* pFrame = nullptr;
        hr = pDecoder->GetFrame(0, &pFrame);
        pDecoder->Release();
        if (FAILED(hr)) return false;

        bool ok = createFromWIC(pFrame);
        pFrame->Release();
        return ok;
    }

    bool zhuziImage::loadFromMemory(const void* data, size_t size) {
        if (!data || size == 0) return false;
        IStream* pStream = nullptr;
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
        if (!hMem) return false;
        void* pMem = GlobalLock(hMem);
        memcpy(pMem, data, size);
        GlobalUnlock(hMem);
        if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) != S_OK) {
            GlobalFree(hMem);
            return false;
        }

        IWICBitmapDecoder* pDecoder = nullptr;
        HRESULT hr = GetWICFactory()->CreateDecoderFromStream(pStream, nullptr, WICDecodeMetadataCacheOnLoad, &pDecoder);
        pStream->Release();
        if (FAILED(hr)) return false;

        IWICBitmapFrameDecode* pFrame = nullptr;
        hr = pDecoder->GetFrame(0, &pFrame);
        pDecoder->Release();
        if (FAILED(hr)) return false;

        bool ok = createFromWIC(pFrame);
        pFrame->Release();
        return ok;
    }

    bool zhuziImage::loadFromHBITMAP(HBITMAP hBitmap) {
        if (!hBitmap) return false;
        BITMAP bm;
        GetObject(hBitmap, sizeof(bm), &bm);
        HDC hdc = GetDC(nullptr);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP hCopy = CreateCompatibleBitmap(hdc, bm.bmWidth, bm.bmHeight);
        if (hCopy) {
            HBITMAP old = (HBITMAP)SelectObject(memDC, hCopy);
            HDC srcDC = CreateCompatibleDC(hdc);
            HBITMAP oldSrc = (HBITMAP)SelectObject(srcDC, hBitmap);
            BitBlt(memDC, 0, 0, bm.bmWidth, bm.bmHeight, srcDC, 0, 0, SRCCOPY);
            SelectObject(srcDC, oldSrc);
            DeleteDC(srcDC);
            SelectObject(memDC, old);
            DeleteDC(memDC);
            ReleaseDC(nullptr, hdc);
            destroy();
            m_hBitmap = hCopy;
            m_width = bm.bmWidth;
            m_height = bm.bmHeight;
            return true;
        }
        DeleteDC(memDC);
        ReleaseDC(nullptr, hdc);
        return false;
    }

    bool zhuziImage::saveToFile(const zhuziString& filePath) const {
        if (!m_hBitmap) return false;
        IWICBitmap* pWICBitmap = nullptr;
        HRESULT hr = GetWICFactory()->CreateBitmapFromHBITMAP(m_hBitmap, nullptr, WICBitmapUseAlpha, &pWICBitmap);
        if (FAILED(hr)) return false;

        zhuziString ext = GetFileExtension(filePath);
        GUID containerFormat;
        if (ext == L"png") containerFormat = GUID_ContainerFormatPng;
        else if (ext == L"jpg" || ext == L"jpeg") containerFormat = GUID_ContainerFormatJpeg;
        else if (ext == L"bmp") containerFormat = GUID_ContainerFormatBmp;
        else if (ext == L"gif") containerFormat = GUID_ContainerFormatGif;
        else containerFormat = GUID_ContainerFormatPng;

        IWICBitmapEncoder* pEncoder = nullptr;
        hr = GetWICFactory()->CreateEncoder(containerFormat, nullptr, &pEncoder);
        if (FAILED(hr)) {
            pWICBitmap->Release();
            return false;
        }

        IWICStream* pStream = nullptr;
        hr = GetWICFactory()->CreateStream(&pStream);
        if (SUCCEEDED(hr)) {
            hr = pStream->InitializeFromFilename(filePath.c_str(), GENERIC_WRITE);
            if (SUCCEEDED(hr)) {
                hr = pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);
                if (SUCCEEDED(hr)) {
                    IWICBitmapFrameEncode* pFrame = nullptr;
                    hr = pEncoder->CreateNewFrame(&pFrame, nullptr);
                    if (SUCCEEDED(hr)) {
                        pFrame->Initialize(nullptr);
                        pFrame->SetSize(m_width, m_height);
                        pFrame->WriteSource(pWICBitmap, nullptr);
                        pFrame->Commit();
                        pFrame->Release();
                    }
                    pEncoder->Commit();
                }
            }
            pStream->Release();
        }
        pEncoder->Release();
        pWICBitmap->Release();
        return SUCCEEDED(hr);
    }

    int zhuziImage::getWidth() const { return m_width; }
    int zhuziImage::getHeight() const { return m_height; }
    SIZE zhuziImage::getSize() const { return { m_width, m_height }; }
    bool zhuziImage::isEmpty() const { return m_hBitmap == nullptr; }

    HBITMAP zhuziImage::toHBITMAP() const {
        if (!m_hBitmap) return nullptr;
        HDC hdc = GetDC(nullptr);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP hCopy = CreateCompatibleBitmap(hdc, m_width, m_height);
        if (hCopy) {
            HBITMAP old = (HBITMAP)SelectObject(memDC, hCopy);
            HDC srcDC = CreateCompatibleDC(hdc);
            HBITMAP oldSrc = (HBITMAP)SelectObject(srcDC, m_hBitmap);
            BitBlt(memDC, 0, 0, m_width, m_height, srcDC, 0, 0, SRCCOPY);
            SelectObject(srcDC, oldSrc);
            DeleteDC(srcDC);
            SelectObject(memDC, old);
        }
        DeleteDC(memDC);
        ReleaseDC(nullptr, hdc);
        return hCopy;
    }

    zhuziImage zhuziImage::clone() const {
        zhuziImage result;
        if (!m_hBitmap) return result;
        result.loadFromHBITMAP(m_hBitmap);
        return result;
    }

    zhuziImage zhuziImage::scale(int newWidth, int newHeight, bool highQuality) const {
        zhuziImage result;
        if (!m_hBitmap || newWidth <= 0 || newHeight <= 0) return result;
        HDC hdc = GetDC(nullptr);
        HDC srcDC = CreateCompatibleDC(hdc);
        HDC dstDC = CreateCompatibleDC(hdc);
        HBITMAP hOldSrc = (HBITMAP)SelectObject(srcDC, m_hBitmap);
        HBITMAP hNewBitmap = CreateCompatibleBitmap(hdc, newWidth, newHeight);
        if (hNewBitmap) {
            HBITMAP hOldDst = (HBITMAP)SelectObject(dstDC, hNewBitmap);
            int oldMode = SetStretchBltMode(dstDC, highQuality ? HALFTONE : COLORONCOLOR);
            StretchBlt(dstDC, 0, 0, newWidth, newHeight, srcDC, 0, 0, m_width, m_height, SRCCOPY);
            SetStretchBltMode(dstDC, oldMode);
            SelectObject(dstDC, hOldDst);
            result.m_hBitmap = hNewBitmap;
            result.m_width = newWidth;
            result.m_height = newHeight;
        }
        SelectObject(srcDC, hOldSrc);
        DeleteDC(srcDC);
        DeleteDC(dstDC);
        ReleaseDC(nullptr, hdc);
        return result;
    }

    zhuziImage zhuziImage::crop(int x, int y, int width, int height) const {
        zhuziImage result;
        if (!m_hBitmap || width <= 0 || height <= 0 || x < 0 || y < 0 || x + width > m_width || y + height > m_height)
            return result;
        HDC hdc = GetDC(nullptr);
        HDC srcDC = CreateCompatibleDC(hdc);
        HBITMAP hOldSrc = (HBITMAP)SelectObject(srcDC, m_hBitmap);
        HBITMAP hNewBitmap = CreateCompatibleBitmap(hdc, width, height);
        if (hNewBitmap) {
            HDC dstDC = CreateCompatibleDC(hdc);
            HBITMAP hOldDst = (HBITMAP)SelectObject(dstDC, hNewBitmap);
            BitBlt(dstDC, 0, 0, width, height, srcDC, x, y, SRCCOPY);
            SelectObject(dstDC, hOldDst);
            DeleteDC(dstDC);
            result.m_hBitmap = hNewBitmap;
            result.m_width = width;
            result.m_height = height;
        }
        SelectObject(srcDC, hOldSrc);
        DeleteDC(srcDC);
        ReleaseDC(nullptr, hdc);
        return result;
    }

    zhuziImage zhuziImage::rotate(float angleDegrees) const {
        zhuziImage result;
        if (!m_hBitmap) return result;
        int angle = (int)angleDegrees % 360;
        if (angle < 0) angle += 360;
        if (angle == 0) return clone();
        if (angle != 90 && angle != 180 && angle != 270) return result;

        int newW, newH;
        if (angle == 90 || angle == 270) { newW = m_height; newH = m_width; }
        else { newW = m_width; newH = m_height; }

        HDC hdc = GetDC(nullptr);
        HDC srcDC = CreateCompatibleDC(hdc);
        HBITMAP hOldSrc = (HBITMAP)SelectObject(srcDC, m_hBitmap);
        HBITMAP hNewBitmap = CreateCompatibleBitmap(hdc, newW, newH);
        if (hNewBitmap) {
            HDC dstDC = CreateCompatibleDC(hdc);
            HBITMAP hOldDst = (HBITMAP)SelectObject(dstDC, hNewBitmap);
            switch (angle) {
            case 90:
                for (int y = 0; y < m_height; ++y)
                    BitBlt(dstDC, newW - 1 - y, 0, 1, m_width, srcDC, 0, y, SRCCOPY);
                break;
            case 180:
                StretchBlt(dstDC, 0, 0, newW, newH, srcDC, m_width - 1, m_height - 1, -m_width, -m_height, SRCCOPY);
                break;
            case 270:
                for (int y = 0; y < m_height; ++y)
                    BitBlt(dstDC, y, 0, 1, m_width, srcDC, m_width - 1, y, SRCCOPY);
                break;
            }
            SelectObject(dstDC, hOldDst);
            DeleteDC(dstDC);
            result.m_hBitmap = hNewBitmap;
            result.m_width = newW;
            result.m_height = newH;
        }
        SelectObject(srcDC, hOldSrc);
        DeleteDC(srcDC);
        ReleaseDC(nullptr, hdc);
        return result;
    }

    zhuziImage zhuziImage::flip(bool horizontal, bool vertical) const {
        zhuziImage result;
        if (!m_hBitmap) return result;
        HDC hdc = GetDC(nullptr);
        HDC srcDC = CreateCompatibleDC(hdc);
        HBITMAP hOldSrc = (HBITMAP)SelectObject(srcDC, m_hBitmap);
        HBITMAP hNewBitmap = CreateCompatibleBitmap(hdc, m_width, m_height);
        if (hNewBitmap) {
            HDC dstDC = CreateCompatibleDC(hdc);
            HBITMAP hOldDst = (HBITMAP)SelectObject(dstDC, hNewBitmap);
            int srcX = horizontal ? m_width - 1 : 0;
            int srcY = vertical ? m_height - 1 : 0;
            int dstW = horizontal ? -m_width : m_width;
            int dstH = vertical ? -m_height : m_height;
            StretchBlt(dstDC, 0, 0, m_width, m_height, srcDC, srcX, srcY, dstW, dstH, SRCCOPY);
            SelectObject(dstDC, hOldDst);
            DeleteDC(dstDC);
            result.m_hBitmap = hNewBitmap;
            result.m_width = m_width;
            result.m_height = m_height;
        }
        SelectObject(srcDC, hOldSrc);
        DeleteDC(srcDC);
        ReleaseDC(nullptr, hdc);
        return result;
    }

    zhuziImage zhuziImage::makeTransparent(COLORREF colorKey, BYTE alpha) const {
        zhuziImage result = clone();
        if (!result.m_hBitmap) return result;
        BITMAP bm;
        GetObject(result.m_hBitmap, sizeof(bm), &bm);
        if (bm.bmBitsPixel != 32) return result;
        HDC hdc = GetDC(nullptr);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP old = (HBITMAP)SelectObject(memDC, result.m_hBitmap);
        BITMAPINFO bmi = { 0 };
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = result.m_width;
        bmi.bmiHeader.biHeight = -result.m_height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        std::vector<BYTE> pixels(result.m_width * result.m_height * 4);
        if (GetDIBits(memDC, result.m_hBitmap, 0, result.m_height, pixels.data(), &bmi, DIB_RGB_COLORS)) {
            BYTE keyR = GetRValue(colorKey);
            BYTE keyG = GetGValue(colorKey);
            BYTE keyB = GetBValue(colorKey);
            for (int y = 0; y < result.m_height; ++y) {
                for (int x = 0; x < result.m_width; ++x) {
                    BYTE* p = &pixels[(y * result.m_width + x) * 4];
                    if (p[2] == keyR && p[1] == keyG && p[0] == keyB) {
                        p[3] = alpha;
                    }
                }
            }
            SetDIBits(memDC, result.m_hBitmap, 0, result.m_height, pixels.data(), &bmi, DIB_RGB_COLORS);
        }
        SelectObject(memDC, old);
        DeleteDC(memDC);
        ReleaseDC(nullptr, hdc);
        return result;
    }

    zhuziImage zhuziImage::applyAlpha(BYTE globalAlpha) const {
        zhuziImage result = clone();
        if (!result.m_hBitmap) return result;
        BITMAP bm;
        GetObject(result.m_hBitmap, sizeof(bm), &bm);
        if (bm.bmBitsPixel != 32) return result;
        HDC hdc = GetDC(nullptr);
        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP old = (HBITMAP)SelectObject(memDC, result.m_hBitmap);
        BITMAPINFO bmi = { 0 };
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = result.m_width;
        bmi.bmiHeader.biHeight = -result.m_height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        std::vector<BYTE> pixels(result.m_width * result.m_height * 4);
        if (GetDIBits(memDC, result.m_hBitmap, 0, result.m_height, pixels.data(), &bmi, DIB_RGB_COLORS)) {
            for (int y = 0; y < result.m_height; ++y) {
                for (int x = 0; x < result.m_width; ++x) {
                    BYTE* p = &pixels[(y * result.m_width + x) * 4];
                    p[3] = globalAlpha;
                }
            }
            SetDIBits(memDC, result.m_hBitmap, 0, result.m_height, pixels.data(), &bmi, DIB_RGB_COLORS);
        }
        SelectObject(memDC, old);
        DeleteDC(memDC);
        ReleaseDC(nullptr, hdc);
        return result;
    }
    
} // namespace zhuzi