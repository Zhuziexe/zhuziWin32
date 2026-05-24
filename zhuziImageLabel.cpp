#include "zhuziImageLabel.h"
#include "zhuziInstance.h"
#include <windows.h>
#include <gdiplus.h>
#include <memory>
#include <vector>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

namespace zhuzi {

    // 静态成员初始化
    ULONG_PTR zhuziImageLabel::m_gdiplusToken = 0;
    int zhuziImageLabel::m_gdiplusRefCount = 0;

    void zhuziImageLabel::initGdiplus() {
        if (m_gdiplusRefCount == 0) {
            GdiplusStartupInput gdiplusStartupInput;
            GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, nullptr);
        }
        m_gdiplusRefCount++;
    }

    void zhuziImageLabel::shutdownGdiplus() {
        if (m_gdiplusRefCount > 0) {
            m_gdiplusRefCount--;
            if (m_gdiplusRefCount == 0 && m_gdiplusToken != 0) {
                GdiplusShutdown(m_gdiplusToken);
                m_gdiplusToken = 0;
            }
        }
    }

    // 辅助函数
    static Color ColorRefToGdiColor(COLORREF cr) {
        return Color(GetRValue(cr), GetGValue(cr), GetBValue(cr));
    }

    zhuziImageLabel::zhuziImageLabel(zhuziControl* parent)
        : zhuziControl(parent), m_image(nullptr), m_scaleMode(ScaleMode::Fit), m_bgColor(RGB(255, 255, 255)), m_hBgBrush(nullptr) {
        initGdiplus();  // 每个实例构造时增加引用计数，确保 GDI+ 已初始化
    }

    zhuziImageLabel::~zhuziImageLabel() {
        destroy();
        shutdownGdiplus();  // 减少引用计数，可能关闭 GDI+
    }

    bool zhuziImageLabel::onCreate(DWORD style) {
        DWORD finalStyle = style | SS_NOTIFY;
        if (!createControl(L"STATIC", 0, 0, 0, 0, finalStyle))
            return false;
        SetWindowSubclass(m_hwnd, StaticProc, 0, (DWORD_PTR)this);
        createBrush();
        return true;
    }

    void zhuziImageLabel::createBrush() {
        if (m_hBgBrush) DeleteObject(m_hBgBrush);
        m_hBgBrush = CreateSolidBrush(m_bgColor);
    }

    bool zhuziImageLabel::loadImage(const zhuziString& filePath) {
        clearImage();
        m_image = new Bitmap(filePath.c_str());
        if (m_image && ((Bitmap*)m_image)->GetLastStatus() != Ok) {
            delete (Bitmap*)m_image;
            m_image = nullptr;
            return false;
        }
        updateImage();
        return m_image != nullptr;
    }

    bool zhuziImageLabel::loadImage(int resourceId, const wchar_t* resourceType) {
        HINSTANCE hInst = zhuziInstance::getHandle();
        HRSRC hRsrc = FindResourceW(hInst, MAKEINTRESOURCEW(resourceId), resourceType);
        if (!hRsrc) return false;

        HGLOBAL hGlobal = LoadResource(hInst, hRsrc);
        if (!hGlobal) return false;

        DWORD size = SizeofResource(hInst, hRsrc);
        const void* data = LockResource(hGlobal);
        if (!data || size == 0) return false;

        bool result = loadImageFromMemory(data, size);
        // 资源无需手动释放，进程结束时自动清理
        return result;
    }

    bool zhuziImageLabel::loadImageFromMemory(const void* data, size_t size) {
        clearImage();
        IStream* pStream = nullptr;
        HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
        if (!hGlobal) return false;
        void* pMem = GlobalLock(hGlobal);
        memcpy(pMem, data, size);
        GlobalUnlock(hGlobal);
        if (CreateStreamOnHGlobal(hGlobal, TRUE, &pStream) != S_OK) {
            GlobalFree(hGlobal);
            return false;
        }
        m_image = Bitmap::FromStream(pStream);
        pStream->Release();
        if (m_image && ((Bitmap*)m_image)->GetLastStatus() != Ok) {
            delete (Bitmap*)m_image;
            m_image = nullptr;
            return false;
        }
        updateImage();
        return m_image != nullptr;
    }

    void zhuziImageLabel::clearImage() {
        if (m_image) {
            delete (Image*)m_image;
            m_image = nullptr;
        }
        updateImage();
    }

    void zhuziImageLabel::setScaleMode(ScaleMode mode) {
        m_scaleMode = mode;
        updateImage();
    }

    void zhuziImageLabel::setBackgroundColor(COLORREF color) {
        m_bgColor = color;
        createBrush();
        if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void zhuziImageLabel::updateImage() {
        if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    void zhuziImageLabel::onPaint() {
        if (!m_hwnd) return;
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(m_hwnd, &ps);
        if (!hdc) return;

        RECT clientRect;
        GetClientRect(m_hwnd, &clientRect);
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;
        if (width <= 0 || height <= 0) {
            EndPaint(m_hwnd, &ps);
            return;
        }

        Graphics graphics(hdc);
        graphics.SetSmoothingMode(SmoothingModeHighQuality);
        graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);

        SolidBrush bgBrush(ColorRefToGdiColor(m_bgColor));
        graphics.FillRectangle(&bgBrush, 0, 0, width, height);

        if (m_image) {
            int imgWidth = ((Image*)m_image)->GetWidth();
            int imgHeight = ((Image*)m_image)->GetHeight();
            if (imgWidth > 0 && imgHeight > 0) {
                Rect destRect(0, 0, width, height);
                switch (m_scaleMode) {
                case ScaleMode::None: {
                    destRect.X = (width - imgWidth) / 2;
                    destRect.Y = (height - imgHeight) / 2;
                    destRect.Width = imgWidth;
                    destRect.Height = imgHeight;
                    break;
                }
                case ScaleMode::Fit: {
                    double ratioImg = (double)imgWidth / imgHeight;
                    double ratioDst = (double)width / height;
                    if (ratioImg > ratioDst) {
                        destRect.Width = width;
                        destRect.Height = (int)(width / ratioImg);
                        destRect.Y = (height - destRect.Height) / 2;
                    }
                    else {
                        destRect.Height = height;
                        destRect.Width = (int)(height * ratioImg);
                        destRect.X = (width - destRect.Width) / 2;
                    }
                    break;
                }
                case ScaleMode::Fill: {
                    double ratioImg = (double)imgWidth / imgHeight;
                    double ratioDst = (double)width / height;
                    if (ratioImg > ratioDst) {
                        destRect.Height = height;
                        destRect.Width = (int)(height * ratioImg);
                        destRect.X = (width - destRect.Width) / 2;
                    }
                    else {
                        destRect.Width = width;
                        destRect.Height = (int)(width / ratioImg);
                        destRect.Y = (height - destRect.Height) / 2;
                    }
                    break;
                }
                case ScaleMode::Stretch:
                default:
                    break;
                }
                graphics.DrawImage((Image*)m_image, destRect);
            }
        }

        EndPaint(m_hwnd, &ps);
    }

    void zhuziImageLabel::draw(Gdiplus::Graphics& graphics, const RECT& rect) {
        // 预留扩展函数
    }

    LRESULT CALLBACK zhuziImageLabel::StaticProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        zhuziImageLabel* pThis = (zhuziImageLabel*)dwRefData;
        if (!pThis) return DefSubclassProc(hwnd, msg, wParam, lParam);
        if (msg == WM_PAINT) {
            pThis->onPaint();
            return 0;
        }
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

} // namespace zhuzi