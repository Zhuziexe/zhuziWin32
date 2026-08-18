#include "zhuziImage.h"
#include <wincodec.h>
#include <vector>
#include <cstring>
#include <cmath>
#include <gdiplus.h>

#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

namespace zhuzi {

    // 静态 GDI+ 初始化
    static ULONG_PTR gdiplusToken = 0;
    static bool gdiplusInitialized = false;
    static void EnsureGdiplus() {
        if (!gdiplusInitialized) {
            GdiplusStartupInput input;
            GdiplusStartup(&gdiplusToken, &input, nullptr);
            gdiplusInitialized = true;
        }
    }

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

    zhuziImage::zhuziImage() : m_hBitmap(nullptr), m_width(0), m_height(0) {
        EnsureGdiplus();
    }
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
        EnsureGdiplus();
        loadFromFile(filePath);
    }

    zhuziImage::zhuziImage(int resourceId, const wchar_t* resourceType)
        : m_hBitmap(nullptr), m_width(0), m_height(0) {
        EnsureGdiplus();
        loadFromResource(resourceId, resourceType);
    }

    // 转换任意 HBITMAP 为 32 位 ARGB
    HBITMAP zhuziImage::convertTo32Bit(HBITMAP hSrc) const {
        if (!hSrc) return nullptr;
        BITMAP bm;
        GetObject(hSrc, sizeof(bm), &bm);
        if (bm.bmBitsPixel == 32 && bm.bmBits != nullptr) {
            // 已经是 32 位，直接复制
            HDC hdc = GetDC(nullptr);
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP hCopy = CreateCompatibleBitmap(hdc, bm.bmWidth, bm.bmHeight);
            if (hCopy) {
                HBITMAP old = (HBITMAP)SelectObject(memDC, hCopy);
                HDC srcDC = CreateCompatibleDC(hdc);
                HBITMAP oldSrc = (HBITMAP)SelectObject(srcDC, hSrc);
                BitBlt(memDC, 0, 0, bm.bmWidth, bm.bmHeight, srcDC, 0, 0, SRCCOPY);
                SelectObject(srcDC, oldSrc);
                DeleteDC(srcDC);
                SelectObject(memDC, old);
            }
            DeleteDC(memDC);
            ReleaseDC(nullptr, hdc);
            return hCopy;
        }
        else {
            // 非 32 位，使用 GDI+ 转换为 32 位 ARGB
            Bitmap* pBitmap = Bitmap::FromHBITMAP(hSrc, nullptr);
            if (!pBitmap) return nullptr;
            int w = pBitmap->GetWidth();
            int h = pBitmap->GetHeight();
            Bitmap dst(w, h, PixelFormat32bppARGB);
            Graphics graphics(&dst);
            graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
            graphics.DrawImage(pBitmap, 0, 0, w, h);
            HBITMAP hResult = nullptr;
            dst.GetHBITMAP(Color(0, 0, 0, 0), &hResult);
            delete pBitmap;
            return hResult;
        }
    }

    bool zhuziImage::createFromWIC(IWICBitmapSource* pSource) {
        if (!pSource) return false;
        UINT w, h;
        pSource->GetSize(&w, &h);
        // 强制转换为 32 位 ARGB
        IWICBitmapSource* pConverted = nullptr;
        IWICFormatConverter* pConverter = nullptr;
        HRESULT hr = GetWICFactory()->CreateFormatConverter(&pConverter);
        if (SUCCEEDED(hr)) {
            hr = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppBGRA,
                WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
            if (SUCCEEDED(hr)) pConverted = pConverter;
        }
        if (!pConverted) {
            if (pConverter) pConverter->Release();
            return false;
        }

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
        if (!hBitmap) {
            pConverter->Release();
            return false;
        }

        UINT stride = w * 4;
        UINT bufferSize = h * stride;
        std::vector<BYTE> buffer(bufferSize);
        WICRect rect = { 0, 0, (INT)w, (INT)h };
        hr = pConverted->CopyPixels(&rect, stride, bufferSize, buffer.data());
        if (SUCCEEDED(hr)) {
            memcpy(pBits, buffer.data(), bufferSize);
            destroy();
            m_hBitmap = hBitmap;
            m_width = w;
            m_height = h;
            pConverter->Release();
            return true;
        }
        DeleteObject(hBitmap);
        pConverter->Release();
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
        HBITMAP h32 = convertTo32Bit(hBitmap);
        if (!h32) return false;
        destroy();
        m_hBitmap = h32;
        BITMAP bm;
        GetObject(h32, sizeof(bm), &bm);
        m_width = bm.bmWidth;
        m_height = bm.bmHeight;
        return true;
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
        return convertTo32Bit(m_hBitmap);
    }

    zhuziImage zhuziImage::scale(int newWidth, int newHeight, bool keepAspect, bool highQuality) const {
        zhuziImage result;
        if (!m_hBitmap || newWidth <= 0 || newHeight <= 0) return result;
        Bitmap src(m_hBitmap, nullptr);
        int srcW = src.GetWidth(), srcH = src.GetHeight();
        int dstW = newWidth, dstH = newHeight;
        if (keepAspect) {
            double ratioSrc = (double)srcW / srcH;
            double ratioDst = (double)newWidth / newHeight;
            if (ratioSrc > ratioDst) {
                dstH = (int)(newWidth / ratioSrc);
                dstW = newWidth;
            }
            else {
                dstW = (int)(newHeight * ratioSrc);
                dstH = newHeight;
            }
        }
        Bitmap dst(dstW, dstH, PixelFormat32bppARGB);
        Graphics graphics(&dst);
        graphics.SetInterpolationMode(highQuality ? InterpolationModeHighQualityBicubic : InterpolationModeNearestNeighbor);
        graphics.DrawImage(&src, 0, 0, dstW, dstH);
        HBITMAP hDst = nullptr;
        dst.GetHBITMAP(Color(0, 0, 0, 0), &hDst);
        if (hDst) {
            result.loadFromHBITMAP(hDst);
            DeleteObject(hDst);
        }
        return result;
    }

    zhuziImage zhuziImage::crop(int x, int y, int width, int height) const {
        zhuziImage result;
        if (!m_hBitmap || width <= 0 || height <= 0 || x < 0 || y < 0 || x + width > m_width || y + height > m_height)
            return result;
        Bitmap src(m_hBitmap, nullptr);
        Bitmap dst(width, height, PixelFormat32bppARGB);
        Graphics graphics(&dst);
        graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        // 修正：使用正确的 DrawImage 重载
        graphics.DrawImage(&src, Rect(0, 0, width, height), x, y, width, height, UnitPixel);
        HBITMAP hDst = nullptr;
        dst.GetHBITMAP(Color(0, 0, 0, 0), &hDst);
        if (hDst) {
            result.loadFromHBITMAP(hDst);
            DeleteObject(hDst);
        }
        return result;
    }

    zhuziImage zhuziImage::rotate(float angleDegrees) const {
        zhuziImage result;
        if (!m_hBitmap) return result;
        int angle = (int)angleDegrees % 360;
        if (angle < 0) angle += 360;
        if (angle == 0) return clone();
        if (angle != 90 && angle != 180 && angle != 270) return result;

        Bitmap src(m_hBitmap, nullptr);
        int srcW = src.GetWidth(), srcH = src.GetHeight();
        int newW, newH;
        if (angle == 90 || angle == 270) { newW = srcH; newH = srcW; }
        else { newW = srcW; newH = srcH; }

        Bitmap dst(newW, newH, PixelFormat32bppARGB);
        Graphics graphics(&dst);
        graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        graphics.TranslateTransform((Gdiplus::REAL)newW / 2, (Gdiplus::REAL)newH / 2);
        graphics.RotateTransform((Gdiplus::REAL)angle);
        graphics.TranslateTransform(-(Gdiplus::REAL)srcW / 2, -(Gdiplus::REAL)srcH / 2);
        graphics.DrawImage(&src, 0, 0, srcW, srcH);
        HBITMAP hDst = nullptr;
        dst.GetHBITMAP(Color(0, 0, 0, 0), &hDst);
        if (hDst) {
            result.loadFromHBITMAP(hDst);
            DeleteObject(hDst);
        }
        return result;
    }

    zhuziImage zhuziImage::flip(bool horizontal, bool vertical) const {
        zhuziImage result;
        if (!m_hBitmap) return result;
        Bitmap src(m_hBitmap, nullptr);
        int w = src.GetWidth(), h = src.GetHeight();
        Bitmap dst(w, h, PixelFormat32bppARGB);
        Graphics graphics(&dst);
        graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        if (horizontal && vertical) {
            graphics.ScaleTransform(-1, -1);
            graphics.TranslateTransform(-(Gdiplus::REAL)w, -(Gdiplus::REAL)h);
        }
        else if (horizontal) {
            graphics.ScaleTransform(-1, 1);
            graphics.TranslateTransform(-(Gdiplus::REAL)w, 0);
        }
        else if (vertical) {
            graphics.ScaleTransform(1, -1);
            graphics.TranslateTransform(0, -(Gdiplus::REAL)h);
        }
        graphics.DrawImage(&src, 0, 0, w, h);
        HBITMAP hDst = nullptr;
        dst.GetHBITMAP(Color(0, 0, 0, 0), &hDst);
        if (hDst) {
            result.loadFromHBITMAP(hDst);
            DeleteObject(hDst);
        }
        return result;
    }

    zhuziImage zhuziImage::makeTransparent(COLORREF colorKey, BYTE alpha) const {
        return clone();
    }

    zhuziImage zhuziImage::applyAlpha(BYTE globalAlpha) const {
        return clone();
    }

    zhuziImage zhuziImage::clone() const {
        zhuziImage result;
        if (!m_hBitmap) return result;
        HBITMAP hCopy = toHBITMAP();
        if (hCopy) {
            result.loadFromHBITMAP(hCopy);
            DeleteObject(hCopy);
        }
        return result;
    }

} // namespace zhuzi