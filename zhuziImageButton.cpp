#include "zhuziImageButton.h"
#include "zhuziInstance.h"
#include <gdiplus.h>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

namespace zhuzi {

    static HBITMAP LoadBitmapFromFile(const zhuziString& path) {
        Gdiplus::Bitmap bitmap(path.c_str());
        if (bitmap.GetLastStatus() != Gdiplus::Ok) return nullptr;
        HBITMAP hBitmap = nullptr;
        bitmap.GetHBITMAP(Gdiplus::Color(0, 0, 0), &hBitmap);
        return hBitmap;
    }

    static HBITMAP LoadBitmapFromResource(int resourceId, const wchar_t* resourceType) {
        HINSTANCE hInst = zhuziInstance::getHandle();
        HRSRC hRsrc = FindResourceW(hInst, MAKEINTRESOURCEW(resourceId), resourceType);
        if (!hRsrc) return nullptr;
        HGLOBAL hGlobal = LoadResource(hInst, hRsrc);
        if (!hGlobal) return nullptr;
        DWORD size = SizeofResource(hInst, hRsrc);
        const void* data = LockResource(hGlobal);
        if (!data || size == 0) return nullptr;
        IStream* pStream = nullptr;
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
        if (!hMem) return nullptr;
        void* pMem = GlobalLock(hMem);
        memcpy(pMem, data, size);
        GlobalUnlock(hMem);
        if (CreateStreamOnHGlobal(hMem, TRUE, &pStream) != S_OK) {
            GlobalFree(hMem);
            return nullptr;
        }
        Gdiplus::Bitmap bitmap(pStream);
        pStream->Release();
        if (bitmap.GetLastStatus() != Gdiplus::Ok) return nullptr;
        HBITMAP hBitmap = nullptr;
        bitmap.GetHBITMAP(Gdiplus::Color(0, 0, 0), &hBitmap);
        return hBitmap;
    }

    zhuziImageButton::zhuziImageButton(zhuziControl* parent)
        : zhuziControl(parent), m_state(State::Normal), m_hBitmapNormal(nullptr), m_hBitmapHover(nullptr),
        m_ownNormal(false), m_ownHover(false), m_scaleMode(ImageScaleMode::Fit), m_imageTextSpacing(5),
        m_imageOnly(false), m_backColor(GetSysColor(COLOR_BTNFACE)), m_hBackBrush(nullptr), m_onClick(nullptr),
        m_trackingMouse(false) {
        m_isCustomDraw = true;
    }

    zhuziImageButton::~zhuziImageButton() {
        if (m_ownNormal && m_hBitmapNormal) DeleteObject(m_hBitmapNormal);
        if (m_ownHover && m_hBitmapHover) DeleteObject(m_hBitmapHover);
        if (m_hBackBrush) DeleteObject(m_hBackBrush);
        destroy();
    }

    bool zhuziImageButton::onCreate(DWORD style) {
        // 使用 STATIC 控件 + SS_NOTIFY 以接收鼠标消息，避免干扰父窗口非客户区
        if (!createControl(L"STATIC", 0, 0, 0, 0, style | WS_CHILD | WS_VISIBLE | SS_NOTIFY, 0, true))
            return false;
        updateBackgroundBrush();
        return true;
    }

    void zhuziImageButton::updateBackgroundBrush() {
        if (m_hBackBrush) DeleteObject(m_hBackBrush);
        m_hBackBrush = CreateSolidBrush(m_backColor);
    }

    void zhuziImageButton::redraw() { if (m_hwnd) InvalidateRect(m_hwnd, nullptr, TRUE); }

    void zhuziImageButton::setText(const zhuziString& text) { m_text = text; redraw(); }
    zhuziString zhuziImageButton::getText() const { return m_text; }

    void zhuziImageButton::loadImage(const zhuziString& path, HBITMAP& target, bool& ownFlag) {
        if (ownFlag && target) DeleteObject(target);
        target = LoadBitmapFromFile(path);
        ownFlag = true;
        redraw();
    }

    void zhuziImageButton::loadImage(int resourceId, const wchar_t* resourceType, HBITMAP& target, bool& ownFlag) {
        if (ownFlag && target) DeleteObject(target);
        target = LoadBitmapFromResource(resourceId, resourceType);
        ownFlag = true;
        redraw();
    }

    void zhuziImageButton::setImageNormal(const zhuziString& imagePath) { loadImage(imagePath, m_hBitmapNormal, m_ownNormal); }
    void zhuziImageButton::setImageNormal(int resourceId, const wchar_t* resourceType) { loadImage(resourceId, resourceType, m_hBitmapNormal, m_ownNormal); }
    void zhuziImageButton::setImageHover(const zhuziString& imagePath) { loadImage(imagePath, m_hBitmapHover, m_ownHover); }
    void zhuziImageButton::setImageHover(int resourceId, const wchar_t* resourceType) { loadImage(resourceId, resourceType, m_hBitmapHover, m_ownHover); }

    void zhuziImageButton::clearImages() {
        if (m_ownNormal && m_hBitmapNormal) DeleteObject(m_hBitmapNormal);
        if (m_ownHover && m_hBitmapHover) DeleteObject(m_hBitmapHover);
        m_hBitmapNormal = m_hBitmapHover = nullptr;
        m_ownNormal = m_ownHover = false;
        redraw();
    }

    void zhuziImageButton::setImageScaleMode(ImageScaleMode mode) { m_scaleMode = mode; redraw(); }
    zhuziImageButton::ImageScaleMode zhuziImageButton::getImageScaleMode() const { return m_scaleMode; }
    void zhuziImageButton::setImageTextSpacing(int spacing) { m_imageTextSpacing = spacing; redraw(); }
    int zhuziImageButton::getImageTextSpacing() const { return m_imageTextSpacing; }
    void zhuziImageButton::setImageOnly(bool imageOnly) { m_imageOnly = imageOnly; redraw(); }
    bool zhuziImageButton::isImageOnly() const { return m_imageOnly; }
    void zhuziImageButton::setBackColor(COLORREF color) { m_backColor = color; updateBackgroundBrush(); redraw(); }
    void zhuziImageButton::resetBackColor() { m_backColor = GetSysColor(COLOR_BTNFACE); updateBackgroundBrush(); redraw(); }
    void zhuziImageButton::setOnClick(std::function<void()> callback) { m_onClick = callback; }

    void zhuziImageButton::ensureMouseTracking() {
        if (!m_trackingMouse && m_hwnd) {
            TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hwnd, 0 };
            if (TrackMouseEvent(&tme)) m_trackingMouse = true;
        }
    }

    void zhuziImageButton::updateState(int x, int y) {
        RECT rc; GetClientRect(m_hwnd, &rc);
        POINT pt = { x, y };
        State newState = PtInRect(&rc, pt) ? State::Hover : State::Normal;
        if (m_state != newState) {
            m_state = newState;
            redraw();
            if (m_state == State::Hover) ensureMouseTracking();
        }
    }

    void zhuziImageButton::onMouseMove(int x, int y) { updateState(x, y); }
    void zhuziImageButton::onMouseLeave() {
        m_state = State::Normal;
        m_trackingMouse = false;
        redraw();
    }
    void zhuziImageButton::onLButtonUp(int x, int y) {
        RECT rc; GetClientRect(m_hwnd, &rc);
        POINT pt = { x, y };
        if (PtInRect(&rc, pt) && m_onClick) m_onClick();
        updateState(x, y);
    }

    void zhuziImageButton::drawImage(zhuziPaint& paint, HBITMAP hBitmap, int x, int y, int width, int height) {
        if (!hBitmap) return;
        Gdiplus::Bitmap bitmap(hBitmap, nullptr);
        if (bitmap.GetLastStatus() != Gdiplus::Ok) return;
        int imgW = bitmap.GetWidth();
        int imgH = bitmap.GetHeight();
        if (imgW == 0 || imgH == 0) return;
        int destX = x, destY = y, destW = width, destH = height;
        switch (m_scaleMode) {
        case ImageScaleMode::None: destW = imgW; destH = imgH; break;
        case ImageScaleMode::Fit: {
            float ratioImg = (float)imgW / imgH;
            float ratioDest = (float)width / height;
            if (ratioImg > ratioDest) {
                destW = width;
                destH = (int)(width / ratioImg);
            }
            else {
                destH = height;
                destW = (int)(height * ratioImg);
            }
            destX = x + (width - destW) / 2;
            destY = y + (height - destH) / 2;
            break;
        }
        case ImageScaleMode::Fill: destW = width; destH = height; break;
        case ImageScaleMode::Center: destW = imgW; destH = imgH; destX = x + (width - destW) / 2; destY = y + (height - destH) / 2; break;
        }
        Gdiplus::Graphics& graphics = paint.getGraphics();
        graphics.DrawImage(&bitmap, destX, destY, destW, destH);
    }

    void zhuziImageButton::drawContent(zhuziPaint& paint) {
        RECT rc; GetClientRect(m_hwnd, &rc);
        int w = rc.right - rc.left;
        int h = rc.bottom - rc.top;

        paint.fillRect(0, 0, w, h, zhuziBrush(zhuziColor(m_backColor)));

        HBITMAP hImage = (m_state == State::Hover && m_hBitmapHover) ? m_hBitmapHover : m_hBitmapNormal;

        RECT imageRect = rc;
        RECT textRect = rc;
        if (!m_imageOnly && !m_text.empty()) {
            SIZE textSize;
            paint.measureText(m_text, getFont(), textSize);
            imageRect.right = w - textSize.cx - m_imageTextSpacing;
            textRect.left = imageRect.right + m_imageTextSpacing;
        }

        if (hImage) {
            drawImage(paint, hImage, imageRect.left, imageRect.top,
                imageRect.right - imageRect.left, imageRect.bottom - imageRect.top);
        }

        if (!m_imageOnly && !m_text.empty()) {
            zhuziBrush textBrush(zhuziColor(GetSysColor(COLOR_BTNTEXT)));
            paint.drawText(m_text, getFont(), textBrush, textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
    }

    void zhuziImageButton::onPaint(zhuziPaint& paint) { drawContent(paint); }

} // namespace zhuzi