#include "zhuziImageLabel.h"
#include "zhuziInstance.h"
#include <windowsx.h>

namespace zhuzi {

    zhuziImageLabel::zhuziImageLabel(zhuziControl* parent)
        : zhuziControl(parent), m_scaleMode(ScaleMode::Fit), m_bgColor(RGB(255, 255, 255)), m_hBgBrush(nullptr) {
        m_isCustomDraw = true;
    }

    zhuziImageLabel::~zhuziImageLabel() {
        if (m_hBgBrush) DeleteObject(m_hBgBrush);
        destroy();
    }

    bool zhuziImageLabel::onCreate(DWORD style) {
        DWORD finalStyle = style | WS_CHILD | WS_VISIBLE | SS_NOTIFY;
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

    void zhuziImageLabel::updateImage() {
        if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    bool zhuziImageLabel::loadImage(const zhuziString& filePath) {
        if (m_image.loadFromFile(filePath)) {
            updateImage();
            return true;
        }
        return false;
    }

    bool zhuziImageLabel::loadImage(int resourceId, const wchar_t* resourceType) {
        if (m_image.loadFromResource(resourceId, resourceType)) {
            updateImage();
            return true;
        }
        return false;
    }

    bool zhuziImageLabel::loadImageFromMemory(const void* data, size_t size) {
        if (m_image.loadFromMemory(data, size)) {
            updateImage();
            return true;
        }
        return false;
    }

    void zhuziImageLabel::clearImage() {
        m_image = zhuziImage();  // 清空
        updateImage();
    }

    void zhuziImageLabel::setScaleMode(ScaleMode mode) {
        m_scaleMode = mode;
        updateImage();
    }

    zhuziImageLabel::ScaleMode zhuziImageLabel::getScaleMode() const {
        return m_scaleMode;
    }

    void zhuziImageLabel::setBackgroundColor(COLORREF color) {
        m_bgColor = color;
        createBrush();
        if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    COLORREF zhuziImageLabel::getBackgroundColor() const {
        return m_bgColor;
    }

    SIZE zhuziImageLabel::getImageSize() const {
        return m_image.getSize();
    }

    void zhuziImageLabel::drawContent(zhuziPaint& paint) {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;
        if (w <= 0 || h <= 0) return;

        paint.fillRect(0, 0, w, h, zhuziBrush(zhuziColor(m_bgColor)));

        if (m_image.isEmpty()) return;

        int imgW = m_image.getWidth();
        int imgH = m_image.getHeight();
        if (imgW == 0 || imgH == 0) return;

        // 保证目标矩形宽高为正
        RECT destRect = { 0, 0, w, h };
        switch (m_scaleMode) {
        case ScaleMode::None:
            destRect.left = (w - imgW) / 2;
            destRect.right = destRect.left + imgW;
            destRect.top = (h - imgH) / 2;
            destRect.bottom = destRect.top + imgH;
            break;
        case ScaleMode::Fit: {
            double ratioImg = (double)imgW / imgH;
            double ratioDst = (double)w / h;
            if (ratioImg > ratioDst) {
                // 宽度填满，高度按比例缩放
                int newHeight = (int)(w / ratioImg);
                destRect.top = (h - newHeight) / 2;
                destRect.bottom = destRect.top + newHeight;
                destRect.left = 0;
                destRect.right = w;
            }
            else {
                // 高度填满，宽度按比例缩放
                int newWidth = (int)(h * ratioImg);
                destRect.left = (w - newWidth) / 2;
                destRect.right = destRect.left + newWidth;
                destRect.top = 0;
                destRect.bottom = h;
            }
            break;
        }
        case ScaleMode::Fill: {
            double ratioImg = (double)imgW / imgH;
            double ratioDst = (double)w / h;
            if (ratioImg > ratioDst) {
                // 高度填满，宽度超出（左右裁剪）
                int newWidth = (int)(h * ratioImg);
                destRect.left = (w - newWidth) / 2;
                destRect.right = destRect.left + newWidth;
                destRect.top = 0;
                destRect.bottom = h;
            }
            else {
                // 宽度填满，高度超出（上下裁剪）
                int newHeight = (int)(w / ratioImg);
                destRect.top = (h - newHeight) / 2;
                destRect.bottom = destRect.top + newHeight;
                destRect.left = 0;
                destRect.right = w;
            }
            break;
        }
        case ScaleMode::Stretch:
        default:
            destRect.left = 0;
            destRect.top = 0;
            destRect.right = w;
            destRect.bottom = h;
            break;
        }

        // 绘制图像
        HBITMAP hBitmap = m_image.toHBITMAP();
        if (hBitmap) {
            HDC hdc = paint.getHDC();
            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP old = (HBITMAP)SelectObject(memDC, hBitmap);
            SetStretchBltMode(hdc, HALFTONE);
            StretchBlt(hdc,
                destRect.left, destRect.top,
                destRect.right - destRect.left,
                destRect.bottom - destRect.top,
                memDC, 0, 0, imgW, imgH, SRCCOPY);
            SelectObject(memDC, old);
            DeleteDC(memDC);
            paint.releaseHDC(hdc);
            DeleteObject(hBitmap);
        }
    }

    void zhuziImageLabel::onPaint(zhuziPaint& paint) {
        drawContent(paint);
    }

    LRESULT CALLBACK zhuziImageLabel::StaticProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        zhuziImageLabel* pThis = (zhuziImageLabel*)dwRefData;
        if (!pThis || pThis->getHandle() != hwnd) {
            RemoveWindowSubclass(hwnd, StaticProc, uIdSubclass);
            return DefSubclassProc(hwnd, msg, wParam, lParam);
        }
        if (msg == WM_PAINT) {
            // 让基类处理 WM_PAINT，会调用 onPaint
            return DefSubclassProc(hwnd, msg, wParam, lParam);
        }
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

} // namespace zhuzi