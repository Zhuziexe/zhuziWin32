#include "zhuziFont.h"
#include "zhuziControl.h"
#include "zhuziInstance.h"
#include <gdiplus.h>

namespace zhuzi {

    static int pointSizeToLogPixelsY(int pointSize) {
        HDC hdc = GetDC(nullptr);
        int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
        ReleaseDC(nullptr, hdc);
        return -MulDiv(pointSize, dpiY, 72);
    }

    zhuziFont::zhuziFont(const zhuziString& faceName, int pointSize, bool bold, bool italic, bool underline)
        : m_hFont(nullptr), m_faceName(faceName), m_pointSize(pointSize), m_bold(bold), m_italic(italic), m_underline(underline) {
        LOGFONTW lf = {};
        lf.lfHeight = pointSizeToLogPixelsY(pointSize);
        lf.lfWeight = bold ? FW_BOLD : FW_NORMAL;
        lf.lfItalic = italic ? TRUE : FALSE;
        lf.lfUnderline = underline ? TRUE : FALSE;
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
        lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
        lf.lfQuality = DEFAULT_QUALITY;
        lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        wcscpy_s(lf.lfFaceName, faceName.c_str());
        m_hFont = CreateFontIndirectW(&lf);
    }

    zhuziFont::~zhuziFont() {
        if (m_hFont) DeleteObject(m_hFont);
    }

    zhuziFont::zhuziFont(zhuziFont&& other) noexcept
        : m_hFont(other.m_hFont), m_faceName(std::move(other.m_faceName)), m_pointSize(other.m_pointSize),
        m_bold(other.m_bold), m_italic(other.m_italic), m_underline(other.m_underline) {
        other.m_hFont = nullptr;
    }

    zhuziFont& zhuziFont::operator=(zhuziFont&& other) noexcept {
        if (this != &other) {
            if (m_hFont) DeleteObject(m_hFont);
            m_hFont = other.m_hFont;
            m_faceName = std::move(other.m_faceName);
            m_pointSize = other.m_pointSize;
            m_bold = other.m_bold;
            m_italic = other.m_italic;
            m_underline = other.m_underline;
            other.m_hFont = nullptr;
        }
        return *this;
    }

    void zhuziFont::applyTo(zhuziControl* ctrl) const {
        if (ctrl && ctrl->getHandle() && m_hFont)
            SendMessageW(ctrl->getHandle(), WM_SETFONT, (WPARAM)m_hFont, TRUE);
    }

    int zhuziFont::getStyle() const {
        int style = 0;
        if (m_bold) style |= Gdiplus::FontStyleBold;
        if (m_italic) style |= Gdiplus::FontStyleItalic;
        if (m_underline) style |= Gdiplus::FontStyleUnderline;
        return style;
    }

    zhuziFont::zhuziFont(const zhuziFont& other)
        : m_faceName(other.m_faceName), m_pointSize(other.m_pointSize),
        m_bold(other.m_bold), m_italic(other.m_italic), m_underline(other.m_underline) {
        LOGFONTW lf = {};
        lf.lfHeight = pointSizeToLogPixelsY(m_pointSize);
        lf.lfWeight = m_bold ? FW_BOLD : FW_NORMAL;
        lf.lfItalic = m_italic ? TRUE : FALSE;
        lf.lfUnderline = m_underline ? TRUE : FALSE;
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
        lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
        lf.lfQuality = DEFAULT_QUALITY;
        lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
        wcscpy_s(lf.lfFaceName, m_faceName.c_str());
        m_hFont = CreateFontIndirectW(&lf);
    }

    zhuziFont& zhuziFont::operator=(const zhuziFont& other) {
        if (this != &other) {
            if (m_hFont) DeleteObject(m_hFont);
            m_faceName = other.m_faceName;
            m_pointSize = other.m_pointSize;
            m_bold = other.m_bold;
            m_italic = other.m_italic;
            m_underline = other.m_underline;
            LOGFONTW lf = {};
            lf.lfHeight = pointSizeToLogPixelsY(m_pointSize);
            lf.lfWeight = m_bold ? FW_BOLD : FW_NORMAL;
            lf.lfItalic = m_italic ? TRUE : FALSE;
            lf.lfUnderline = m_underline ? TRUE : FALSE;
            wcscpy_s(lf.lfFaceName, m_faceName.c_str());
            m_hFont = CreateFontIndirectW(&lf);
        }
        return *this;
    }

} // namespace zhuzi