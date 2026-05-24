#include "zhuziImageList.h"
#include <gdiplus.h>
#include <memory>
#include "zhuziInstance.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")

namespace zhuzi {

    // 静态 GDI+ 初始化辅助
    static ULONG_PTR g_gdiplusToken = 0;
    static int g_gdiplusRefCount = 0;

    static void initGdiPlus() {
        if (g_gdiplusRefCount == 0) {
            Gdiplus::GdiplusStartupInput input;
            Gdiplus::GdiplusStartup(&g_gdiplusToken, &input, nullptr);
        }
        g_gdiplusRefCount++;
    }

    static void shutdownGdiPlus() {
        if (g_gdiplusRefCount > 0) {
            g_gdiplusRefCount--;
            if (g_gdiplusRefCount == 0 && g_gdiplusToken != 0) {
                Gdiplus::GdiplusShutdown(g_gdiplusToken);
                g_gdiplusToken = 0;
            }
        }
    }

    static UINT colorDepthToFlags(zhuziImageList::ColorDepth depth) {
        return ILC_MASK | ILC_COLOR32;
    }

    // 将 GDI+ Bitmap 转换为 HBITMAP
    HBITMAP zhuziImageList::bitmapFromGdiplusImage(Gdiplus::Image* image) {
        if (!image) return nullptr;
        Gdiplus::Bitmap* pBitmap = dynamic_cast<Gdiplus::Bitmap*>(image);
        if (!pBitmap) return nullptr;
        HBITMAP hBitmap = nullptr;
        pBitmap->GetHBITMAP(Gdiplus::Color(0, 0, 0), &hBitmap);
        return hBitmap;
    }

    // 缩放 HBITMAP 到目标尺寸
    HBITMAP zhuziImageList::scaleBitmap(HBITMAP hSrc, int targetW, int targetH) const {
        if (!hSrc) return nullptr;
        BITMAP bm;
        GetObject(hSrc, sizeof(bm), &bm);
        if (bm.bmWidth == targetW && bm.bmHeight == targetH) {
            // 尺寸相同，不需要缩放，返回原图副本（调用者负责释放）
            HDC hdc = GetDC(nullptr);
            HBITMAP hCopy = CreateCompatibleBitmap(hdc, targetW, targetH);
            HDC hdcSrc = CreateCompatibleDC(hdc);
            HDC hdcDst = CreateCompatibleDC(hdc);
            SelectObject(hdcSrc, hSrc);
            SelectObject(hdcDst, hCopy);
            BitBlt(hdcDst, 0, 0, targetW, targetH, hdcSrc, 0, 0, SRCCOPY);
            DeleteDC(hdcSrc);
            DeleteDC(hdcDst);
            ReleaseDC(nullptr, hdc);
            return hCopy;
        }
        // 缩放
        HDC hdc = GetDC(nullptr);
        HDC hdcMemSrc = CreateCompatibleDC(hdc);
        HDC hdcMemDst = CreateCompatibleDC(hdc);
        HBITMAP hDest = CreateCompatibleBitmap(hdc, targetW, targetH);
        SelectObject(hdcMemSrc, hSrc);
        SelectObject(hdcMemDst, hDest);
        SetStretchBltMode(hdcMemDst, HALFTONE);
        StretchBlt(hdcMemDst, 0, 0, targetW, targetH,
            hdcMemSrc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
        DeleteDC(hdcMemSrc);
        DeleteDC(hdcMemDst);
        ReleaseDC(nullptr, hdc);
        return hDest;
    }

    // 缩放 GDI+ Image 到目标尺寸
    Gdiplus::Bitmap* zhuziImageList::scaleGdiplusImage(Gdiplus::Image* pImage, int targetW, int targetH) const {
        if (!pImage) return nullptr;
        int srcW = pImage->GetWidth();
        int srcH = pImage->GetHeight();
        if (srcW == targetW && srcH == targetH) {
            // 尺寸相同，返回原图的副本（调用者负责释放）
            Gdiplus::Bitmap* pCopy = new Gdiplus::Bitmap(targetW, targetH, PixelFormat32bppARGB);
            Gdiplus::Graphics graphics(pCopy);
            graphics.DrawImage(pImage, 0, 0, targetW, targetH);
            return pCopy;
        }
        Gdiplus::Bitmap* pScaled = new Gdiplus::Bitmap(targetW, targetH, PixelFormat32bppARGB);
        Gdiplus::Graphics graphics(pScaled);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        graphics.DrawImage(pImage, 0, 0, targetW, targetH);
        return pScaled;
    }

    zhuziImageList::zhuziImageList() : m_hImageList(nullptr), m_width(0), m_height(0) {
        initGdiPlus();
    }

    zhuziImageList::~zhuziImageList() {
        destroy();
        shutdownGdiPlus();
    }

    zhuziImageList::zhuziImageList(zhuziImageList&& other) noexcept
        : m_hImageList(other.m_hImageList), m_width(other.m_width), m_height(other.m_height) {
        other.m_hImageList = nullptr;
        other.m_width = other.m_height = 0;
    }

    zhuziImageList& zhuziImageList::operator=(zhuziImageList&& other) noexcept {
        if (this != &other) {
            destroy();
            m_hImageList = other.m_hImageList;
            m_width = other.m_width;
            m_height = other.m_height;
            other.m_hImageList = nullptr;
            other.m_width = other.m_height = 0;
        }
        return *this;
    }

    bool zhuziImageList::create(const CreateParams& params) {
        return create(params.width, params.height, params.colorDepth,
            params.initialCount, params.growCount);
    }

    bool zhuziImageList::create(int width, int height, ColorDepth depth,
        int initialCount, int growCount) {
        destroy();
        UINT flags = colorDepthToFlags(depth);
        m_hImageList = ImageList_Create(width, height, flags, initialCount, growCount);
        if (m_hImageList) {
            m_width = width;
            m_height = height;
            return true;
        }
        return false;
    }

    void zhuziImageList::destroy() {
        if (m_hImageList) {
            ImageList_Destroy(m_hImageList);
            m_hImageList = nullptr;
        }
        m_width = m_height = 0;
    }

    int zhuziImageList::getWidth() const { return m_width; }
    int zhuziImageList::getHeight() const { return m_height; }
    int zhuziImageList::getImageCount() const {
        return m_hImageList ? ImageList_GetImageCount(m_hImageList) : 0;
    }

    int zhuziImageList::add(HBITMAP hBitmap, HBITMAP hMask) {
        if (!m_hImageList || !hBitmap) return -1;
        // 自动缩放
        HBITMAP hScaled = scaleBitmap(hBitmap, m_width, m_height);
        int idx = ImageList_Add(m_hImageList, hScaled, hMask);
        if (hScaled != hBitmap) DeleteObject(hScaled);
        return idx;
    }

    int zhuziImageList::add(HICON hIcon) {
        // 图标自动缩放由系统处理，无需额外代码
        if (!m_hImageList || !hIcon) return -1;
        return ImageList_AddIcon(m_hImageList, hIcon);
    }

    int zhuziImageList::addFromFile(const wchar_t* filePath) {
        Gdiplus::Bitmap bitmap(filePath);
        if (bitmap.GetLastStatus() != Gdiplus::Ok) return -1;
        return addFromGdiplusImage(&bitmap);
    }

    int zhuziImageList::addFromResource(int resourceId, const wchar_t* resourceType) {
        HINSTANCE hInst = zhuziInstance::getHandle();

        // 图标资源：使用 LoadImage 指定尺寸自动缩放
        if (wcscmp(resourceType, L"ICON") == 0) {
            HICON hIcon = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(resourceId),
                IMAGE_ICON, m_width, m_height, LR_DEFAULTCOLOR);
            if (hIcon) {
                int idx = add(hIcon);
                DestroyIcon(hIcon);
                if (idx != -1) return idx;
            }
            // 备选：LoadIcon 无法指定尺寸，不推荐
            return -1;
        }

        // 位图资源：使用 LoadImage 指定尺寸自动缩放
        if (wcscmp(resourceType, L"BITMAP") == 0) {
            HBITMAP hBitmap = (HBITMAP)LoadImageW(hInst, MAKEINTRESOURCEW(resourceId),
                IMAGE_BITMAP, m_width, m_height, LR_DEFAULTCOLOR);
            if (hBitmap) {
                int idx = add(hBitmap, nullptr);
                DeleteObject(hBitmap);
                if (idx != -1) return idx;
            }
            return -1;
        }

        // PNG 或其他自定义资源：先用 GDI+ 从内存加载，再自动缩放
        HRSRC hRsrc = FindResourceW(hInst, MAKEINTRESOURCEW(resourceId), resourceType);
        if (!hRsrc) return -1;
        HGLOBAL hGlobal = LoadResource(hInst, hRsrc);
        if (!hGlobal) return -1;
        DWORD size = SizeofResource(hInst, hRsrc);
        const void* data = LockResource(hGlobal);
        if (!data || size == 0) return -1;
        return addFromMemory(data, size);
    }

    int zhuziImageList::addFromMemory(const void* data, size_t size) {
        IStream* pStream = nullptr;
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
        if (!hGlobal) return -1;
        void* pMem = GlobalLock(hGlobal);
        memcpy(pMem, data, size);
        GlobalUnlock(hGlobal);
        if (CreateStreamOnHGlobal(hGlobal, TRUE, &pStream) != S_OK) {
            GlobalFree(hGlobal);
            return -1;
        }
        Gdiplus::Bitmap bitmap(pStream);
        pStream->Release();
        if (bitmap.GetLastStatus() != Gdiplus::Ok) return -1;
        return addFromGdiplusImage(&bitmap);
    }

    int zhuziImageList::addFromGdiplusImage(Gdiplus::Image* image) {
        if (!image) return -1;
        // 自动缩放
        Gdiplus::Bitmap* pScaled = scaleGdiplusImage(image, m_width, m_height);
        HBITMAP hBitmap = bitmapFromGdiplusImage(pScaled);
        if (pScaled && pScaled != image) delete pScaled;
        if (!hBitmap) return -1;
        int idx = add(hBitmap, nullptr);
        DeleteObject(hBitmap);
        return idx;
    }

    int zhuziImageList::addSolidColor(int width, int height, COLORREF color) {
        HDC hdc = GetDC(nullptr);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
        HDC hdcMem = CreateCompatibleDC(hdc);
        HGDIOBJ oldBmp = SelectObject(hdcMem, hBitmap);
        HBRUSH brush = CreateSolidBrush(color);
        RECT rect = { 0, 0, width, height };
        FillRect(hdcMem, &rect, brush);
        DeleteObject(brush);
        SelectObject(hdcMem, oldBmp);
        DeleteDC(hdcMem);
        ReleaseDC(nullptr, hdc);
        int idx = add(hBitmap, nullptr);
        DeleteObject(hBitmap);
        return idx;
    }

    bool zhuziImageList::insert(int index, HBITMAP hBitmap, HBITMAP hMask) {
        if (!m_hImageList || !hBitmap) return false;
        HBITMAP hScaled = scaleBitmap(hBitmap, m_width, m_height);
        bool ok = (ImageList_Replace(m_hImageList, index, hScaled, hMask) != 0);
        if (hScaled != hBitmap) DeleteObject(hScaled);
        return ok;
    }

    bool zhuziImageList::replace(int index, HBITMAP hBitmap, HBITMAP hMask) {
        if (!m_hImageList || !hBitmap) return false;
        HBITMAP hScaled = scaleBitmap(hBitmap, m_width, m_height);
        bool ok = (ImageList_Replace(m_hImageList, index, hScaled, hMask) != 0);
        if (hScaled != hBitmap) DeleteObject(hScaled);
        return ok;
    }

    bool zhuziImageList::replace(int index, HICON hIcon) {
        if (!m_hImageList || !hIcon) return false;
        return ImageList_ReplaceIcon(m_hImageList, index, hIcon) != 0;
    }

    bool zhuziImageList::remove(int index) {
        if (!m_hImageList) return false;
        return ImageList_Remove(m_hImageList, index) != 0;
    }

    void zhuziImageList::removeAll() {
        if (m_hImageList) ImageList_RemoveAll(m_hImageList);
    }

    bool zhuziImageList::draw(HDC hdc, int x, int y, int imageIndex, DrawStyle style) const {
        if (!m_hImageList) return false;
        UINT fStyle = ILD_NORMAL;
        switch (style) {
        case DrawStyle::Transparent: fStyle = ILD_TRANSPARENT; break;
        case DrawStyle::Selected:    fStyle = ILD_SELECTED; break;
        case DrawStyle::Focused:     fStyle = ILD_FOCUS; break;
        default: break;
        }
        return ImageList_Draw(m_hImageList, imageIndex, hdc, x, y, fStyle) != 0;
    }

    bool zhuziImageList::draw(HDC hdc, int x, int y, int w, int h, int imageIndex) const {
        if (!m_hImageList) return false;
        return ImageList_DrawEx(m_hImageList, imageIndex, hdc, x, y, w, h,
            CLR_NONE, CLR_NONE, ILD_NORMAL) != 0;
    }

    bool zhuziImageList::drawEx(HDC hdc, int x, int y, int dx, int dy,
        int imageIndex, COLORREF rgbBk, COLORREF rgbFg,
        UINT style) const {
        if (!m_hImageList) return false;
        return ImageList_DrawEx(m_hImageList, imageIndex, hdc, x, y, dx, dy,
            rgbBk, rgbFg, style) != 0;
    }

    HICON zhuziImageList::extractIcon(int imageIndex) const {
        if (!m_hImageList) return nullptr;
        return ImageList_GetIcon(m_hImageList, imageIndex, ILD_NORMAL);
    }

    HBITMAP zhuziImageList::extractBitmap(int imageIndex) const {
        if (!m_hImageList) return nullptr;
        HDC hdc = GetDC(nullptr);
        HDC hdcMem = CreateCompatibleDC(hdc);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdc, m_width, m_height);
        HGDIOBJ oldBmp = SelectObject(hdcMem, hBitmap);
        COLORREF bkColor = ImageList_GetBkColor(m_hImageList);
        HBRUSH bgBrush = CreateSolidBrush(bkColor);
        RECT rect = { 0, 0, m_width, m_height };
        FillRect(hdcMem, &rect, bgBrush);
        DeleteObject(bgBrush);
        ImageList_Draw(m_hImageList, imageIndex, hdcMem, 0, 0, ILD_NORMAL);
        SelectObject(hdcMem, oldBmp);
        DeleteDC(hdcMem);
        ReleaseDC(nullptr, hdc);
        return hBitmap;
    }

    bool zhuziImageList::setOverlayImage(int overlayIndex, int imageIndex) {
        if (!m_hImageList || overlayIndex < 1 || overlayIndex > 4) return false;
        return ImageList_SetOverlayImage(m_hImageList, imageIndex, overlayIndex) != 0;
    }

    bool zhuziImageList::drawOverlay(HDC hdc, int x, int y, int imageIndex, int overlayIndex) const {
        if (!m_hImageList || overlayIndex < 1 || overlayIndex > 4) return false;
        UINT style = INDEXTOOVERLAYMASK(overlayIndex);
        return ImageList_Draw(m_hImageList, imageIndex, hdc, x, y, style) != 0;
    }

    void zhuziImageList::setBkColor(COLORREF clrBk) {
        if (m_hImageList) ImageList_SetBkColor(m_hImageList, clrBk);
    }

    COLORREF zhuziImageList::getBkColor() const {
        return m_hImageList ? ImageList_GetBkColor(m_hImageList) : CLR_NONE;
    }

    bool zhuziImageList::copyToClipboard(HWND hwndOwner, int imageIndex) const {
        HBITMAP hBitmap = extractBitmap(imageIndex);
        if (!hBitmap) return false;
        if (!OpenClipboard(hwndOwner)) {
            DeleteObject(hBitmap);
            return false;
        }
        EmptyClipboard();
        SetClipboardData(CF_BITMAP, hBitmap);
        CloseClipboard();
        return true;
    }

    int zhuziImageList::pasteFromClipboard() {
        if (!OpenClipboard(nullptr)) return -1;
        HANDLE hData = GetClipboardData(CF_BITMAP);
        if (!hData) {
            CloseClipboard();
            return -1;
        }
        HBITMAP hBitmap = (HBITMAP)hData;
        BITMAP bm;
        GetObject(hBitmap, sizeof(bm), &bm);
        HDC hdc = GetDC(nullptr);
        HBITMAP hCopy = CreateCompatibleBitmap(hdc, bm.bmWidth, bm.bmHeight);
        HDC hdcSrc = CreateCompatibleDC(hdc);
        HDC hdcDst = CreateCompatibleDC(hdc);
        SelectObject(hdcSrc, hBitmap);
        SelectObject(hdcDst, hCopy);
        BitBlt(hdcDst, 0, 0, bm.bmWidth, bm.bmHeight, hdcSrc, 0, 0, SRCCOPY);
        DeleteDC(hdcSrc);
        DeleteDC(hdcDst);
        ReleaseDC(nullptr, hdc);
        CloseClipboard();
        int index = add(hCopy, nullptr);
        DeleteObject(hCopy);
        return index;
    }

} // namespace zhuzi