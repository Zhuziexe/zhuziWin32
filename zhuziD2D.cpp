#include "zhuziD2D.h"
#include <math.h>

namespace zhuzi {

    zhuziD2DFactory::zhuziD2DFactory()
        : m_pD2DFactory(nullptr), m_pDWriteFactory(nullptr), m_pWICFactory(nullptr) {
        D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &m_pD2DFactory);
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
        CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_IWICImagingFactory, (void**)&m_pWICFactory);
    }

    zhuziD2DFactory::~zhuziD2DFactory() {
        // 不主动释放，避免程序退出时的访问冲突
    }

    zhuziD2DFactory& zhuziD2DFactory::instance() {
        static zhuziD2DFactory factory;
        return factory;
    }

    zhuziD2DBrush::zhuziD2DBrush(ID2D1RenderTarget* pRT, COLORREF color, float opacity) {
        D2D1_COLOR_F d2dColor = {
            GetRValue(color) / 255.0f,
            GetGValue(color) / 255.0f,
            GetBValue(color) / 255.0f,
            opacity
        };
        pRT->CreateSolidColorBrush(d2dColor, &m_pBrush);
    }
    zhuziD2DBrush::~zhuziD2DBrush() { if (m_pBrush) m_pBrush->Release(); }

    zhuziD2DPen::zhuziD2DPen(ID2D1RenderTarget* pRT, COLORREF color, float width, D2D1_DASH_STYLE dashStyle)
        : m_width(width), m_pBrush(nullptr), m_pStrokeStyle(nullptr) {
        D2D1_COLOR_F d2dColor = {
            GetRValue(color) / 255.0f,
            GetGValue(color) / 255.0f,
            GetBValue(color) / 255.0f,
            1.0f
        };
        pRT->CreateSolidColorBrush(d2dColor, &m_pBrush);
        if (dashStyle != D2D1_DASH_STYLE_SOLID) {
            auto& factory = zhuziD2DFactory::instance();
            factory.getD2DFactory()->CreateStrokeStyle(
                D2D1::StrokeStyleProperties(
                    D2D1_CAP_STYLE_FLAT,   // startCap
                    D2D1_CAP_STYLE_FLAT,   // endCap
                    D2D1_CAP_STYLE_FLAT,   // dashCap
                    D2D1_LINE_JOIN_MITER,  // lineJoin
                    10.0f,                 // miterLimit
                    dashStyle,             // dashStyle
                    0.0f                   // dashOffset
                ),
                nullptr, 0, &m_pStrokeStyle);
        }
    }
    zhuziD2DPen::~zhuziD2DPen() {
        if (m_pBrush) m_pBrush->Release();
        if (m_pStrokeStyle) m_pStrokeStyle->Release();
    }

    zhuziD2DPathGeometry::zhuziD2DPathGeometry() : m_pGeometry(nullptr), m_pSink(nullptr), m_open(false) {
        auto& factory = zhuziD2DFactory::instance();
        factory.getD2DFactory()->CreatePathGeometry(&m_pGeometry);
    }
    zhuziD2DPathGeometry::~zhuziD2DPathGeometry() {
        if (m_pGeometry) m_pGeometry->Release();
        if (m_pSink) m_pSink->Release();
    }
    bool zhuziD2DPathGeometry::begin() {
        if (m_open) return false;
        HRESULT hr = m_pGeometry->Open(&m_pSink);
        if (SUCCEEDED(hr)) { m_open = true; return true; }
        return false;
    }
    void zhuziD2DPathGeometry::moveTo(float x, float y) { if (m_open) m_pSink->BeginFigure(D2D1::Point2F(x, y), D2D1_FIGURE_BEGIN_FILLED); }
    void zhuziD2DPathGeometry::lineTo(float x, float y) { if (m_open) m_pSink->AddLine(D2D1::Point2F(x, y)); }
    void zhuziD2DPathGeometry::cubicBezierTo(float x1, float y1, float x2, float y2, float x3, float y3) {
        if (m_open) m_pSink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), D2D1::Point2F(x3, y3)));
    }
    bool zhuziD2DPathGeometry::end() {
        if (!m_open) return false;
        m_pSink->EndFigure(D2D1_FIGURE_END_OPEN);
        HRESULT hr = m_pSink->Close();
        m_pSink->Release(); m_pSink = nullptr;
        m_open = false;
        return SUCCEEDED(hr);
    }

    zhuziD2DRenderTarget::zhuziD2DRenderTarget(HWND hwnd)
        : m_hwnd(hwnd), m_pRT(nullptr), m_drawing(false) {
        auto& factory = zhuziD2DFactory::instance();
        m_pFactory = factory.getD2DFactory();
        m_pDWriteFactory = factory.getDWriteFactory();
        resize();
    }
    zhuziD2DRenderTarget::~zhuziD2DRenderTarget() { if (m_pRT) m_pRT->Release(); }
    void zhuziD2DRenderTarget::resize() {
        RECT rc;
        GetClientRect(m_hwnd, &rc);
        m_size.width = max(1, rc.right - rc.left);
        m_size.height = max(1, rc.bottom - rc.top);
        if (m_pRT) {
            m_pRT->Resize(m_size);
        }
        else {
            m_pFactory->CreateHwndRenderTarget(
                D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
                    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)),
                D2D1::HwndRenderTargetProperties(m_hwnd, m_size),
                &m_pRT);
            if (m_pRT) {
                m_pRT->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                m_pRT->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
            }
        }
    }
    bool zhuziD2DRenderTarget::beginDraw() {
        if (m_drawing || !m_pRT) return false;
        m_pRT->BeginDraw();
        m_drawing = true;
        return true;
    }
    bool zhuziD2DRenderTarget::endDraw() {
        if (!m_drawing || !m_pRT) return false;
        HRESULT hr = m_pRT->EndDraw();
        m_drawing = false;
        if (hr == D2DERR_RECREATE_TARGET) resize();
        return SUCCEEDED(hr);
    }
    void zhuziD2DRenderTarget::clear(COLORREF color) {
        if (!m_pRT) return;
        D2D1_COLOR_F d2dColor = {
            GetRValue(color) / 255.0f,
            GetGValue(color) / 255.0f,
            GetBValue(color) / 255.0f,
            ((color >> 24) & 0xFF) / 255.0f
        };
        m_pRT->Clear(d2dColor);
    }
    void zhuziD2DRenderTarget::getSize(int& width, int& height) const { width = m_size.width; height = m_size.height; }

    void zhuziD2DRenderTarget::drawLine(float x1, float y1, float x2, float y2, const zhuziD2DPen& pen) {
        if (m_pRT) m_pRT->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), pen.getBrush(), pen.getWidth(), pen.getStrokeStyle());
    }
    void zhuziD2DRenderTarget::drawRectangle(float left, float top, float right, float bottom, const zhuziD2DPen& pen, const zhuziD2DBrush* fill) {
        if (!m_pRT) return;
        D2D1_RECT_F rect = D2D1::RectF(left, top, right, bottom);
        if (fill) m_pRT->FillRectangle(rect, fill->get());
        m_pRT->DrawRectangle(rect, pen.getBrush(), pen.getWidth(), pen.getStrokeStyle());
    }
    void zhuziD2DRenderTarget::fillRectangle(float left, float top, float right, float bottom, const zhuziD2DBrush& brush) {
        if (m_pRT) m_pRT->FillRectangle(D2D1::RectF(left, top, right, bottom), brush.get());
    }
    void zhuziD2DRenderTarget::drawEllipse(float cx, float cy, float rx, float ry, const zhuziD2DPen& pen, const zhuziD2DBrush* fill) {
        if (!m_pRT) return;
        D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(cx, cy), rx, ry);
        if (fill) m_pRT->FillEllipse(ellipse, fill->get());
        m_pRT->DrawEllipse(ellipse, pen.getBrush(), pen.getWidth(), pen.getStrokeStyle());
    }
    void zhuziD2DRenderTarget::fillEllipse(float cx, float cy, float rx, float ry, const zhuziD2DBrush& brush) {
        if (m_pRT) m_pRT->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cx, cy), rx, ry), brush.get());
    }
    void zhuziD2DRenderTarget::drawRoundedRectangle(float left, float top, float right, float bottom, float rx, float ry, const zhuziD2DPen& pen, const zhuziD2DBrush* fill) {
        if (!m_pRT) return;
        D2D1_ROUNDED_RECT rrect = D2D1::RoundedRect(D2D1::RectF(left, top, right, bottom), rx, ry);
        if (fill) m_pRT->FillRoundedRectangle(rrect, fill->get());
        m_pRT->DrawRoundedRectangle(rrect, pen.getBrush(), pen.getWidth(), pen.getStrokeStyle());
    }
    void zhuziD2DRenderTarget::fillRoundedRectangle(float left, float top, float right, float bottom, float rx, float ry, const zhuziD2DBrush& brush) {
        if (m_pRT) m_pRT->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(left, top, right, bottom), rx, ry), brush.get());
    }
    void zhuziD2DRenderTarget::drawGeometry(const zhuziD2DPathGeometry& geom, const zhuziD2DPen& pen, const zhuziD2DBrush* fill) {
        if (!m_pRT) return;
        if (fill) m_pRT->FillGeometry(geom.get(), fill->get());
        m_pRT->DrawGeometry(geom.get(), pen.getBrush(), pen.getWidth(), pen.getStrokeStyle());
    }
    void zhuziD2DRenderTarget::fillGeometry(const zhuziD2DPathGeometry& geom, const zhuziD2DBrush& brush) {
        if (m_pRT) m_pRT->FillGeometry(geom.get(), brush.get());
    }

    void zhuziD2DRenderTarget::drawText(const zhuziString& text, float x, float y, const zhuziD2DBrush& brush, const zhuziFont& font) {
        if (!m_pRT || !m_pDWriteFactory) return;
        IDWriteTextFormat* pFormat = nullptr;
        m_pDWriteFactory->CreateTextFormat(font.getFontFamily(), nullptr,
            font.isbold() ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
            font.isitalic() ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, (float)font.getSize(), L"en-us", &pFormat);
        if (pFormat) {
            D2D1_RECT_F rect = D2D1::RectF(x, y, x + 1000.0f, y + 100.0f);
            m_pRT->DrawText(text.c_str(), (UINT32)text.length(), pFormat, rect, brush.get());
            pFormat->Release();
        }
    }

    void zhuziD2DRenderTarget::drawText(const zhuziString& text, const RECT& rect, const zhuziD2DBrush& brush, const zhuziFont& font, DWORD format) {
        if (!m_pRT || !m_pDWriteFactory) return;
        IDWriteTextFormat* pFormat = nullptr;
        m_pDWriteFactory->CreateTextFormat(font.getFontFamily(), nullptr,
            font.isbold() ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
            font.isitalic() ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, (float)font.getSize(), L"en-us", &pFormat);
        if (pFormat) {
            DWRITE_TEXT_ALIGNMENT align = DWRITE_TEXT_ALIGNMENT_LEADING;
            if (format & DT_CENTER) align = DWRITE_TEXT_ALIGNMENT_CENTER;
            else if (format & DT_RIGHT) align = DWRITE_TEXT_ALIGNMENT_TRAILING;
            pFormat->SetTextAlignment(align);
            DWRITE_PARAGRAPH_ALIGNMENT paraAlign = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
            if (format & DT_VCENTER) paraAlign = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
            else if (format & DT_BOTTOM) paraAlign = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
            pFormat->SetParagraphAlignment(paraAlign);
            D2D1_RECT_F d2dRect = D2D1::RectF((float)rect.left, (float)rect.top, (float)rect.right, (float)rect.bottom);
            m_pRT->DrawText(text.c_str(), (UINT32)text.length(), pFormat, d2dRect, brush.get());
            pFormat->Release();
        }
    }

    void zhuziD2DRenderTarget::pushTransform(float ox, float oy, float angleDeg, float sx, float sy) {
        if (!m_pRT) return;
        D2D1_MATRIX_3X2_F transform = D2D1::Matrix3x2F::Identity();
        transform = transform * D2D1::Matrix3x2F::Translation(-ox, -oy);
        if (angleDeg != 0) transform = transform * D2D1::Matrix3x2F::Rotation(angleDeg);
        if (sx != 1.0f || sy != 1.0f) transform = transform * D2D1::Matrix3x2F::Scale(sx, sy);
        transform = transform * D2D1::Matrix3x2F::Translation(ox, oy);
        m_pRT->SetTransform(transform);
    }
    void zhuziD2DRenderTarget::popTransform() {
        if (m_pRT) m_pRT->SetTransform(D2D1::Matrix3x2F::Identity());
    }

} // namespace zhuzi