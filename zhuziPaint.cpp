#include "zhuziPaint.h"

namespace zhuzi {

    // ==================== zhuziColor ====================
    zhuziColor::zhuziColor() : m_r(0), m_g(0), m_b(0), m_alpha(255) {}
    zhuziColor::zhuziColor(BYTE r, BYTE g, BYTE b, BYTE a)
        : m_r(r), m_g(g), m_b(b), m_alpha(a) {
    }
    zhuziColor::zhuziColor(COLORREF cr, BYTE a)
        : m_r(GetRValue(cr)), m_g(GetGValue(cr)), m_b(GetBValue(cr)), m_alpha(a) {
    }
    zhuziColor::zhuziColor(const Gdiplus::Color& color)
        : m_r(color.GetR()), m_g(color.GetG()), m_b(color.GetB()), m_alpha(color.GetA()) {
    }

    Gdiplus::Color zhuziColor::toGdiplusColor() const {
        return Gdiplus::Color(m_alpha, m_r, m_g, m_b);
    }
    zhuziColor::operator Gdiplus::Color() const { return toGdiplusColor(); }

    COLORREF zhuziColor::toCOLORREF() const { return RGB(m_r, m_g, m_b); }
    zhuziColor::operator COLORREF() const { return toCOLORREF(); }

    BYTE zhuziColor::getR() const { return m_r; }
    BYTE zhuziColor::getG() const { return m_g; }
    BYTE zhuziColor::getB() const { return m_b; }
    BYTE zhuziColor::getA() const { return m_alpha; }
    void zhuziColor::setAlpha(BYTE a) { m_alpha = a; }

    // ==================== zhuziPen ====================
    zhuziPen::zhuziPen(const zhuziColor& color, float width, Gdiplus::DashStyle style)
        : m_pen(new Gdiplus::Pen(color, width)) {
        m_pen->SetDashStyle(style);
    }
    zhuziPen::~zhuziPen() { delete m_pen; }

    zhuziPen::zhuziPen(zhuziPen&& other) noexcept : m_pen(other.m_pen) {
        other.m_pen = nullptr;
    }
    zhuziPen& zhuziPen::operator=(zhuziPen&& other) noexcept {
        if (this != &other) {
            delete m_pen;
            m_pen = other.m_pen;
            other.m_pen = nullptr;
        }
        return *this;
    }
    Gdiplus::Pen* zhuziPen::get() const { return m_pen; }

    // ==================== zhuziBrush ====================
    zhuziBrush::zhuziBrush(const zhuziColor& color)
        : m_brush(new Gdiplus::SolidBrush(color)) {
    }

    zhuziBrush::zhuziBrush(HBRUSH hBrush) {
        LOGBRUSH lb;
        if (GetObject(hBrush, sizeof(LOGBRUSH), &lb) && lb.lbStyle == BS_SOLID) {
            COLORREF cr = lb.lbColor;
            m_brush = new Gdiplus::SolidBrush(Gdiplus::Color(GetRValue(cr), GetGValue(cr), GetBValue(cr)));
        }
        else {
            m_brush = new Gdiplus::SolidBrush(Gdiplus::Color(0, 0, 0));
        }
    }

    zhuziBrush::zhuziBrush(const zhuziColor& color1, const zhuziColor& color2,
        const POINT& point1, const POINT& point2)
        : m_brush(new Gdiplus::LinearGradientBrush(
            Gdiplus::Point(point1.x, point1.y),
            Gdiplus::Point(point2.x, point2.y),
            color1, color2)) {
    }

    zhuziBrush::zhuziBrush(const zhuziColor& color1, const zhuziColor& color2,
        const RECT& rect, float angle, bool isAngleScalable)
        : m_brush(new Gdiplus::LinearGradientBrush(
            Gdiplus::Rect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top),
            color1, color2, angle, isAngleScalable)) {
    }

    zhuziBrush::~zhuziBrush() { delete m_brush; }

    zhuziBrush::zhuziBrush(zhuziBrush&& other) noexcept : m_brush(other.m_brush) {
        other.m_brush = nullptr;
    }
    zhuziBrush& zhuziBrush::operator=(zhuziBrush&& other) noexcept {
        if (this != &other) {
            delete m_brush;
            m_brush = other.m_brush;
            other.m_brush = nullptr;
        }
        return *this;
    }
    Gdiplus::Brush* zhuziBrush::get() const { return m_brush; }

    // ==================== zhuziPath ====================
    zhuziPath::zhuziPath() : m_path(new Gdiplus::GraphicsPath()) {}
    zhuziPath::~zhuziPath() { delete m_path; }

    zhuziPath::zhuziPath(zhuziPath&& other) noexcept : m_path(other.m_path) {
        other.m_path = nullptr;
    }
    zhuziPath& zhuziPath::operator=(zhuziPath&& other) noexcept {
        if (this != &other) {
            delete m_path;
            m_path = other.m_path;
            other.m_path = nullptr;
        }
        return *this;
    }

    Gdiplus::GraphicsPath* zhuziPath::getNative() const { return m_path; }

    // ---------- 修正：使用 Gdiplus::Point 避免 LONG 歧义 ----------
    void zhuziPath::addLine(const POINT& p1, const POINT& p2) {
        m_path->AddLine(Gdiplus::Point(p1.x, p1.y), Gdiplus::Point(p2.x, p2.y));
    }

    void zhuziPath::addRectangle(const RECT& rect) {
        m_path->AddRectangle(Gdiplus::Rect(rect.left, rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top));
    }

    void zhuziPath::addEllipse(const RECT& rect) {
        m_path->AddEllipse(Gdiplus::Rect(rect.left, rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top));
    }

    void zhuziPath::addArc(const RECT& rect, float startAngle, float sweepAngle) {
        m_path->AddArc(Gdiplus::Rect(rect.left, rect.top,
            rect.right - rect.left,
            rect.bottom - rect.top),
            startAngle, sweepAngle);
    }

    // ---------- 修正：使用 Gdiplus::Point 避免 LONG 歧义 ----------
    void zhuziPath::addBezier(const POINT& p1, const POINT& p2, const POINT& p3, const POINT& p4) {
        m_path->AddBezier(Gdiplus::Point(p1.x, p1.y),
            Gdiplus::Point(p2.x, p2.y),
            Gdiplus::Point(p3.x, p3.y),
            Gdiplus::Point(p4.x, p4.y));
    }

    void zhuziPath::addPolygon(const POINT* points, int count) {
        Gdiplus::Point* gdipPoints = new Gdiplus::Point[count];
        for (int i = 0; i < count; ++i) {
            gdipPoints[i] = Gdiplus::Point(points[i].x, points[i].y);
        }
        m_path->AddPolygon(gdipPoints, count);
        delete[] gdipPoints;
    }

    void zhuziPath::addCurve(const POINT* points, int count) {
        Gdiplus::Point* gdipPoints = new Gdiplus::Point[count];
        for (int i = 0; i < count; ++i) {
            gdipPoints[i] = Gdiplus::Point(points[i].x, points[i].y);
        }
        m_path->AddCurve(gdipPoints, count);
        delete[] gdipPoints;
    }

    void zhuziPath::addString(const zhuziString& text, const zhuziFont& font, const POINT& origin) {
        Gdiplus::FontFamily family(font.getFontFamily());
        Gdiplus::StringFormat format;
        m_path->AddString(text.c_str(), (int)text.length(),
            &family, font.getStyle(), (Gdiplus::REAL)font.getSize(),
            Gdiplus::PointF((float)origin.x, (float)origin.y), &format);
    }

    void zhuziPath::addString(const zhuziString& text, const zhuziFont& font, const RECT& layoutRect) {
        Gdiplus::FontFamily family(font.getFontFamily());
        Gdiplus::StringFormat format;
        m_path->AddString(text.c_str(), (int)text.length(),
            &family, font.getStyle(), (Gdiplus::REAL)font.getSize(),
            Gdiplus::RectF((float)layoutRect.left, (float)layoutRect.top,
                (float)(layoutRect.right - layoutRect.left),
                (float)(layoutRect.bottom - layoutRect.top)),
            &format);
    }

    void zhuziPath::closeFigure() {
        m_path->CloseFigure();
    }

    void zhuziPath::reset() {
        m_path->Reset();
    }

    // ==================== zhuziDC ====================
    zhuziDC::zhuziDC(HWND hwnd)
        : m_hwnd(hwnd), m_hdc(GetDC(hwnd)), m_graphics(m_hdc) {
        m_graphics.SetSmoothingMode(Gdiplus::SmoothingModeDefault);
    }
    zhuziDC::~zhuziDC() {
        if (m_hdc) ReleaseDC(m_hwnd, m_hdc);
    }
    Gdiplus::Graphics& zhuziDC::getGraphics() { return m_graphics; }
    HDC zhuziDC::getHDC() const { return m_hdc; }
    HWND zhuziDC::getHwnd() const { return m_hwnd; }

    // ==================== zhuziPaint ====================
    zhuziPaint::zhuziPaint(HDC hdc, const RECT& clientRect)
        : m_graphics(hdc), m_clientRect(clientRect) {
        m_graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        m_graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        m_graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
    }

    void zhuziPaint::clear(const zhuziColor& color) {
        zhuziBrush brush(color);
        m_graphics.FillRectangle(brush.get(), 0, 0, getWidth(), getHeight());
    }

    void zhuziPaint::drawLine(int x1, int y1, int x2, int y2, const zhuziPen& pen) {
        m_graphics.DrawLine(pen.get(), x1, y1, x2, y2);
    }

    void zhuziPaint::drawRect(int x, int y, int width, int height, const zhuziPen& pen, const zhuziBrush* brush) {
        if (brush) m_graphics.FillRectangle(brush->get(), x, y, width, height);
        m_graphics.DrawRectangle(pen.get(), x, y, width, height);
    }

    void zhuziPaint::fillRect(int x, int y, int width, int height, const zhuziBrush& brush) {
        m_graphics.FillRectangle(brush.get(), x, y, width, height);
    }

    void zhuziPaint::drawCircle(int cx, int cy, int radius, const zhuziPen& pen, const zhuziBrush* brush) {
        if (brush) m_graphics.FillEllipse(brush->get(), cx - radius, cy - radius, radius * 2, radius * 2);
        m_graphics.DrawEllipse(pen.get(), cx - radius, cy - radius, radius * 2, radius * 2);
    }

    void zhuziPaint::drawRoundRect(int x, int y, int width, int height, int radius, const zhuziPen& pen, const zhuziBrush* brush) {
        if (radius <= 0) {
            drawRect(x, y, width, height, pen, brush);
            return;
        }
        Gdiplus::GraphicsPath path;
        path.AddArc(x, y, radius * 2, radius * 2, 180, 90);
        path.AddArc(x + width - radius * 2, y, radius * 2, radius * 2, 270, 90);
        path.AddArc(x + width - radius * 2, y + height - radius * 2, radius * 2, radius * 2, 0, 90);
        path.AddArc(x, y + height - radius * 2, radius * 2, radius * 2, 90, 90);
        path.CloseFigure();
        if (brush) m_graphics.FillPath(brush->get(), &path);
        m_graphics.DrawPath(pen.get(), &path);
    }

    void zhuziPaint::fillRoundRect(int x, int y, int width, int height, int radius, const zhuziBrush& brush) {
        if (radius <= 0) {
            fillRect(x, y, width, height, brush);
            return;
        }
        Gdiplus::GraphicsPath path;
        path.AddArc(x, y, radius * 2, radius * 2, 180, 90);
        path.AddArc(x + width - radius * 2, y, radius * 2, radius * 2, 270, 90);
        path.AddArc(x + width - radius * 2, y + height - radius * 2, radius * 2, radius * 2, 0, 90);
        path.AddArc(x, y + height - radius * 2, radius * 2, radius * 2, 90, 90);
        path.CloseFigure();
        m_graphics.FillPath(brush.get(), &path);
    }

    void zhuziPaint::drawText(const zhuziString& text, int x, int y, const zhuziBrush& brush, const zhuziFont& font) {
        Gdiplus::Font gdiFont(font.getFontFamily(), (Gdiplus::REAL)font.getSize(), font.getStyle());
        Gdiplus::PointF point((float)x, (float)y);
        m_graphics.DrawString(text.c_str(), (int)text.length(), &gdiFont, point, brush.get());
    }

    void zhuziPaint::drawText(const zhuziString& text, const zhuziFont& font, const zhuziBrush& brush,
        const RECT& rect, DWORD format) {
        float fontSize = (float)font.getSize();
        if (fontSize <= 0) fontSize = 16.0f;
        Gdiplus::Font gdiFont(font.getFontFamily(), fontSize, font.getStyle(), Gdiplus::UnitPixel);
        Gdiplus::StringFormat stringFormat;

        if (format & DT_CENTER)
            stringFormat.SetAlignment(Gdiplus::StringAlignmentCenter);
        else if (format & DT_RIGHT)
            stringFormat.SetAlignment(Gdiplus::StringAlignmentFar);
        else
            stringFormat.SetAlignment(Gdiplus::StringAlignmentNear);

        if (format & DT_VCENTER)
            stringFormat.SetLineAlignment(Gdiplus::StringAlignmentCenter);
        else if (format & DT_BOTTOM)
            stringFormat.SetLineAlignment(Gdiplus::StringAlignmentFar);
        else
            stringFormat.SetLineAlignment(Gdiplus::StringAlignmentNear);

        if (format & DT_SINGLELINE)
            stringFormat.SetFormatFlags(stringFormat.GetFormatFlags() | Gdiplus::StringFormatFlagsNoWrap);

        Gdiplus::RectF layoutRect((float)rect.left, (float)rect.top,
            (float)(rect.right - rect.left),
            (float)(rect.bottom - rect.top));
        m_graphics.DrawString(text.c_str(), (int)text.length(), &gdiFont, layoutRect, &stringFormat, brush.get());
    }

    void zhuziPaint::measureText(const zhuziString& text, const zhuziFont& font, SIZE& size) const {
        Gdiplus::Font gdiFont(font.getFontFamily(), (Gdiplus::REAL)font.getSize(), font.getStyle(), Gdiplus::UnitPixel);
        Gdiplus::RectF bounds;
        m_graphics.MeasureString(text.c_str(), (int)text.length(), &gdiFont, Gdiplus::PointF(0, 0), &bounds);
        size.cx = (int)ceil(bounds.Width);
        size.cy = (int)ceil(bounds.Height);
    }

    void zhuziPaint::drawPath(const zhuziPath& path, const zhuziPen& pen, const zhuziBrush* brush) {
        if (brush) m_graphics.FillPath(brush->get(), path.getNative());
        m_graphics.DrawPath(pen.get(), path.getNative());
    }

    void zhuziPaint::fillPath(const zhuziPath& path, const zhuziBrush& brush) {
        m_graphics.FillPath(brush.get(), path.getNative());
    }

    int zhuziPaint::getWidth() const {
        return m_clientRect.right - m_clientRect.left;
    }
    int zhuziPaint::getHeight() const {
        return m_clientRect.bottom - m_clientRect.top;
    }

    Gdiplus::Graphics& zhuziPaint::getGraphics() {
        return m_graphics;
    }

    HDC zhuziPaint::getHDC() {
        return m_graphics.GetHDC();
    }

    void zhuziPaint::releaseHDC(HDC hdc) {
        m_graphics.ReleaseHDC(hdc);
    }

} // namespace zhuzi