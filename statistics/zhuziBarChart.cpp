#include "zhuziBarChart.h"
#include "../zhuziInstance.h"
#include <windowsx.h>
#include <algorithm>
#include <sstream>
#include <cmath>

namespace zhuzi {

    bool zhuziBarChart::s_classRegistered = false;
    const wchar_t* zhuziBarChart::WINDOW_CLASS_NAME = L"zhuziBarChartClass";

    zhuziBarChart::zhuziBarChart(zhuziControl* parent)
        : zhuziControl(parent),
        m_defaultBarColor(0, 0, 255),
        m_backgroundColor(255, 255, 255),
        m_yMin(0),
        m_yMax(100),
        m_yMinOmit(0),
        m_yStep(10),
        m_barWidth(40),
        m_barSpacing(10),
        m_marginLeft(60),
        m_marginTop(20),
        m_marginRight(20),
        m_marginBottom(40),
        m_scrollX(0),
        m_scrollY(0),
        m_contentWidth(0),
        m_contentHeight(0),
        m_zoomX(1.0f),
        m_zoomY(1.0f)
    {
        setAutoYRange(true);
        setHScrollAlways(false);
        setVScrollAlways(false);
        setAutoScrollBarsFlag(true);
        setDrawBreakLineFlag(true);
        setValueDisplayModeFlag(ValueDisplayMode::AboveBar);
        m_flag3 = 0;
    }

    zhuziBarChart::~zhuziBarChart() { destroy(); }

    bool zhuziBarChart::onCreate(DWORD style) {
        if (!s_classRegistered) {
            WNDCLASSEXW wc = {};
            wc.cbSize = sizeof(WNDCLASSEXW);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = WndProc;
            wc.hInstance = zhuziInstance::getHandle();
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            wc.lpszClassName = WINDOW_CLASS_NAME;
            s_classRegistered = (RegisterClassExW(&wc) != 0);
            if (!s_classRegistered) return false;
        }
        if (m_id == -1) {
            try { m_id = allocateId(); }
            catch (...) { return false; }
        }
        HWND hParent = m_parent ? m_parent->getHandle() : nullptr;
        DWORD dwStyle = style | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;
        m_hwnd = CreateWindowExW(0, WINDOW_CLASS_NAME, L"", dwStyle,
            0, 0, 0, 0, hParent, (HMENU)(INT_PTR)m_id,
            zhuziInstance::getHandle(), this);
        if (!m_hwnd) {
            releaseId(m_id);
            m_id = -1;
            return false;
        }
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
        if (m_parent) {
            RECT rc;
            GetClientRect(m_parent->getHandle(), &rc);
            onParentResize(rc.right - rc.left, rc.bottom - rc.top);
        }
        return true;
    }

    void zhuziBarChart::onPaint(zhuziPaint& paint) {
        paint.clear(m_backgroundColor);
        drawAxesAndLabels(paint);
        drawBars(paint);
        if (getValueDisplayMode() != ValueDisplayMode::None) drawValueTexts(paint);
    }

    void zhuziBarChart::onParentResize(int parentWidth, int parentHeight) {
        if (m_hwnd && m_parent) applyLayout(parentWidth, parentHeight);
        updateContentSize();
        updateScrollBars();
        InvalidateRect(m_hwnd, nullptr, TRUE);
    }

    int32_t zhuziBarChart::addItem(const zhuziString& itemName, int32_t height, const zhuziColor& color, bool bRedraw) {
        m_items.emplace_back(itemName, height, color);
        if (getAutoYRange()) {
            int32_t maxVal = m_yMax;
            for (const auto& it : m_items) if (it.value > maxVal) maxVal = it.value;
            m_yMax = maxVal;
            if (m_yMax <= m_yMin) m_yMax = m_yMin + 1;
            if (m_yStep > 0) m_yMax = ((m_yMax + m_yStep - 1) / m_yStep) * m_yStep;
        }
        updateContentSize();
        updateScrollBars();
        if (bRedraw) Invalidate();
        return (int32_t)m_items.size() - 1;
    }

    void zhuziBarChart::removeItem(int32_t index) {
        if (index < 0 || index >= (int32_t)m_items.size()) return;
        m_items.erase(m_items.begin() + index);
        if (getAutoYRange()) {
            int32_t maxVal = m_yMin;
            for (const auto& it : m_items) if (it.value > maxVal) maxVal = it.value;
            m_yMax = maxVal;
            if (m_yMax <= m_yMin) m_yMax = m_yMin + 1;
            if (m_yStep > 0) m_yMax = ((m_yMax + m_yStep - 1) / m_yStep) * m_yStep;
        }
        updateContentSize();
        updateScrollBars();
        Invalidate();
    }

    void zhuziBarChart::clearItems() {
        m_items.clear();
        if (getAutoYRange()) m_yMax = 100;
        updateContentSize();
        updateScrollBars();
        Invalidate();
    }

    void zhuziBarChart::updateItem(int32_t index, const zhuziString& itemName, int32_t height, const zhuziColor& color) {
        if (index < 0 || index >= (int32_t)m_items.size()) return;
        m_items[index].name = itemName;
        m_items[index].value = height;
        m_items[index].color = color;
        if (getAutoYRange()) {
            int32_t maxVal = m_yMin;
            for (const auto& it : m_items) if (it.value > maxVal) maxVal = it.value;
            m_yMax = maxVal;
            if (m_yMax <= m_yMin) m_yMax = m_yMin + 1;
            if (m_yStep > 0) m_yMax = ((m_yMax + m_yStep - 1) / m_yStep) * m_yStep;
        }
        updateContentSize();
        updateScrollBars();
        Invalidate();
    }

    void zhuziBarChart::setYAxisRange(int32_t minValue, int32_t maxValue) {
        setAutoYRange(false);
        m_yMin = minValue;
        m_yMax = maxValue;
        if (m_yMax <= m_yMin) m_yMax = m_yMin + 1;
        updateContentSize();
        updateScrollBars();
        Invalidate();
    }

    void zhuziBarChart::setYAxisMinOmit(int32_t omitValue) { m_yMinOmit = omitValue; Invalidate(); }
    void zhuziBarChart::setYAxisStep(int32_t step) { m_yStep = step; Invalidate(); }
    void zhuziBarChart::setBarWidth(uint16_t width) { m_barWidth = width; updateContentSize(); updateScrollBars(); Invalidate(); }
    void zhuziBarChart::setBarSpacing(uint16_t spacing) { m_barSpacing = spacing; updateContentSize(); updateScrollBars(); Invalidate(); }
    void zhuziBarChart::setDefaultBarColor(const zhuziColor& color) { m_defaultBarColor = color; Invalidate(); }
    void zhuziBarChart::setBackgroundColor(const zhuziColor& color) { m_backgroundColor = color; Invalidate(); }
    void zhuziBarChart::setValueDisplayMode(ValueDisplayMode mode) { setValueDisplayModeFlag(mode); Invalidate(); }
    void zhuziBarChart::setAutoScrollBars(bool enable) { setAutoScrollBarsFlag(enable); updateScrollBars(); }
    void zhuziBarChart::setDrawBreakLine(bool draw) { setDrawBreakLineFlag(draw); Invalidate(); }
    void zhuziBarChart::setYAxisSpacing(int16_t spacing) { m_flag3 = spacing; updateContentSize(); updateScrollBars(); Invalidate(); }

    void zhuziBarChart::setZoom(float zoomX, float zoomY) {
        m_zoomX = zoomX;
        m_zoomY = zoomY;
        updateContentSize();
        updateScrollBars();
        Invalidate();
    }

    void zhuziBarChart::setMargins(uint16_t left, uint16_t top, uint16_t right, uint16_t bottom) {
        m_marginLeft = left; m_marginTop = top; m_marginRight = right; m_marginBottom = bottom;
        updateContentSize();
        updateScrollBars();
        Invalidate();
    }

    void zhuziBarChart::setScrollOffset(int32_t x, int32_t y) {
        int32_t maxX = (std::max)(0, m_contentWidth - (getClientWidth() - m_marginLeft - m_marginRight));
        int32_t maxY = (std::max)(0, m_contentHeight - (getClientHeight() - m_marginTop - m_marginBottom));
        int32_t newX = (std::max)(0, (std::min)(x, maxX));
        int32_t newY = (std::max)(0, (std::min)(y, maxY));
        if (newX == m_scrollX && newY == m_scrollY) return;
        m_scrollX = newX; m_scrollY = newY;
        SCROLLINFO si = { sizeof(SCROLLINFO), SIF_POS };
        si.nPos = m_scrollX; SetScrollInfo(m_hwnd, SB_HORZ, &si, TRUE);
        si.nPos = m_scrollY; SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
        Invalidate();
    }

    void zhuziBarChart::setHorizontalScrollBarPolicy(bool alwaysOn) { setHScrollAlways(alwaysOn); updateScrollBars(); }
    void zhuziBarChart::setVerticalScrollBarPolicy(bool alwaysOn) { setVScrollAlways(alwaysOn); updateScrollBars(); }

    int32_t zhuziBarChart::getClientWidth() const {
        if (!m_hwnd) return 0;
        RECT rc; GetClientRect(m_hwnd, &rc);
        return rc.right - rc.left;
    }
    int32_t zhuziBarChart::getClientHeight() const {
        if (!m_hwnd) return 0;
        RECT rc; GetClientRect(m_hwnd, &rc);
        return rc.bottom - rc.top;
    }

    int32_t zhuziBarChart::getContentPlotHeight() const {
        return m_contentHeight - m_marginTop - m_marginBottom;
    }

    int32_t zhuziBarChart::getBarX(int32_t index) const {
        return m_marginLeft + m_flag3 + index * (m_barWidth + m_barSpacing) - m_scrollX;
    }

    int32_t zhuziBarChart::getValueY(int32_t value) const {
        int32_t plotHeight = getContentPlotHeight();
        if (plotHeight <= 0) return m_marginTop - m_scrollY;
        int32_t effectiveMin = (std::max)(m_yMin, m_yMinOmit);
        int32_t effectiveMax = (m_yMax > effectiveMin) ? m_yMax : effectiveMin + 1;
        double ratio = (double)(value - effectiveMin) / (effectiveMax - effectiveMin);
        ratio = (std::max)(0.0, (std::min)(1.0, ratio));
        int32_t y = m_marginTop + (int32_t)(plotHeight * (1.0 - ratio));
        return y - m_scrollY;
    }

    void zhuziBarChart::updateContentSize() {
        int32_t barCount = (int32_t)m_items.size();
        int32_t totalBarsWidth = barCount * m_barWidth + (barCount - 1) * m_barSpacing;
        totalBarsWidth = (int32_t)(totalBarsWidth * m_zoomX);
        m_contentWidth = m_marginLeft + m_flag3 + totalBarsWidth + m_marginRight;

        int32_t clientH = getClientHeight();
        if (clientH <= 0) clientH = 400;
        int32_t plotHeight = clientH - m_marginTop - m_marginBottom;
        if (plotHeight < 0) plotHeight = 0;
        m_contentHeight = m_marginTop + (int32_t)(plotHeight * m_zoomY) + m_marginBottom;
    }

    void zhuziBarChart::updateScrollBars() {
        if (!m_hwnd) return;
        int32_t clientW = getClientWidth();
        int32_t clientH = getClientHeight();

        bool needHorz = getHScrollAlways() || (getAutoScrollBars() && m_contentWidth > clientW);
        bool needVert = (m_zoomY > 1.0f) && (getVScrollAlways() || (getAutoScrollBars() && m_contentHeight > clientH));

        ShowScrollBar(m_hwnd, SB_HORZ, needHorz);
        ShowScrollBar(m_hwnd, SB_VERT, needVert);

        if (needHorz) {
            SCROLLINFO si = { sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE | SIF_POS };
            si.nMin = 0; si.nMax = m_contentWidth; si.nPage = clientW; si.nPos = m_scrollX;
            SetScrollInfo(m_hwnd, SB_HORZ, &si, TRUE);
        }
        if (needVert) {
            SCROLLINFO si = { sizeof(SCROLLINFO), SIF_RANGE | SIF_PAGE | SIF_POS };
            si.nMin = 0; si.nMax = m_contentHeight; si.nPage = clientH; si.nPos = m_scrollY;
            SetScrollInfo(m_hwnd, SB_VERT, &si, TRUE);
        }
    }


    void zhuziBarChart::drawBreakSymbol(zhuziPaint& paint, int32_t x, int32_t y) {
        zhuziPen pen(zhuziColor(0, 0, 0), 1);
        constexpr int32_t offset = -4;
        // 之字形，总高度10像素，宽度3像素，整体应用偏移
        paint.drawLine(x, y - 6 + offset, x + 3, y - 3 + offset, pen);
        paint.drawLine(x + 3, y - 3 + offset, x, y - 1 + offset, pen);
        paint.drawLine(x, y - 1 + offset, x + 3, y + 1 + offset, pen);
        paint.drawLine(x + 3, y + 1 + offset, x, y + 4 + offset, pen);
    }

    void zhuziBarChart::drawAxesAndLabels(zhuziPaint& paint) {
        zhuziFont font(L"Microsoft YaHei", 12);
        zhuziBrush textBrush(zhuziColor(0, 0, 0));
        zhuziPen axisPen(zhuziColor(0, 0, 0), 1);

        int32_t clientW = getClientWidth();
        if (clientW <= 0) return;

        int32_t plotHeight = getContentPlotHeight();
        int32_t baseY = m_marginTop + plotHeight - m_scrollY;
        int32_t yAxisX = m_marginLeft - m_scrollX;

        // 绘制X轴
        paint.drawLine(yAxisX, baseY, clientW - m_marginRight - m_scrollX, baseY, axisPen);

        // 处理Y轴（可能断开）
        int32_t yTop = m_marginTop - m_scrollY;
        int32_t yBottom = baseY;
        bool needBreak = getDrawBreakLine() && (m_yMin < m_yMinOmit);
        int32_t breakY = 0;
        if (needBreak) {
            breakY = getValueY(m_yMinOmit);
            if (breakY < yTop) breakY = yTop;
            if (breakY > yBottom) breakY = yBottom;
            const int32_t gap = 10;
            // 上半段
            if (breakY - gap > yTop) {
                paint.drawLine(yAxisX, yTop, yAxisX, breakY - gap, axisPen);
            }
            // 下半段
            if (breakY + gap < yBottom) {
                paint.drawLine(yAxisX, breakY + gap, yAxisX, yBottom, axisPen);
            }
            // 绘制折断符号
            drawBreakSymbol(paint, yAxisX, breakY);
        }
        else {
            paint.drawLine(yAxisX, yTop, yAxisX, yBottom, axisPen);
        }

        // 绘制Y轴刻度
        int32_t effectiveMin = (std::max)(m_yMin, m_yMinOmit);
        int32_t startVal = ((effectiveMin + m_yStep - 1) / m_yStep) * m_yStep;
        for (int32_t val = startVal; val <= m_yMax; val += m_yStep) {
            int32_t y = getValueY(val);
            if (y < yTop - 5 || y > yBottom + 5) continue;

            bool isBreakVal = needBreak && (val == m_yMinOmit);
            if (!isBreakVal) {
                paint.drawLine(yAxisX, y, yAxisX + 5, y, axisPen);
            }
            std::wstringstream ss; ss << val;
            zhuziString str = ss.str().c_str();
            SIZE sz; paint.measureText(str, font, sz);
            paint.drawText(str, yAxisX - sz.cx - 8, y - sz.cy / 2, textBrush, font);
        }

        // 绘制X轴类别标签
        for (size_t i = 0; i < m_items.size(); ++i) {
            int32_t barX = getBarX((int32_t)i);
            int32_t barCenterX = barX + m_barWidth / 2;
            if (barCenterX < -50 || barCenterX > clientW + 50) continue;
            const BarItem& item = m_items[i];
            SIZE sz; paint.measureText(item.name, font, sz);
            int32_t labelX = barCenterX - sz.cx / 2;
            int32_t labelY = baseY + 5;
            paint.drawText(item.name, labelX, labelY, textBrush, font);
        }
    }

    void zhuziBarChart::drawBars(zhuziPaint& paint) {
        int32_t clientW = getClientWidth();
        if (clientW <= 0) return;

        int32_t plotHeight = getContentPlotHeight();
        int32_t baseY = m_marginTop + plotHeight - m_scrollY;

        for (size_t i = 0; i < m_items.size(); ++i) {
            const BarItem& item = m_items[i];
            int32_t barX = getBarX((int32_t)i);
            int32_t barTop = getValueY(item.value);
            int32_t barH = baseY - barTop;
            if (barH <= 0) continue;
            if (barX + m_barWidth < -100 || barX > clientW + 100) continue;
            zhuziPen pen(item.color, 1);
            zhuziBrush brush(item.color);
            paint.drawRect(barX, barTop, m_barWidth, barH, pen, &brush);
        }
    }

    void zhuziBarChart::drawValueTexts(zhuziPaint& paint) {
        zhuziFont font(L"Microsoft YaHei", 11);
        int32_t clientW = getClientWidth();
        if (clientW <= 0) return;

        int32_t plotHeight = getContentPlotHeight();
        int32_t baseY = m_marginTop + plotHeight - m_scrollY;
        ValueDisplayMode mode = getValueDisplayMode();

        for (size_t i = 0; i < m_items.size(); ++i) {
            const BarItem& item = m_items[i];
            int32_t barX = getBarX((int32_t)i);
            int32_t barTop = getValueY(item.value);
            int32_t barH = baseY - barTop;
            if (barH <= 0) continue;
            std::wstringstream ss; ss << item.value;
            zhuziString str = ss.str().c_str();
            SIZE sz; paint.measureText(str, font, sz);
            int32_t textX = barX + (m_barWidth - sz.cx) / 2;
            int32_t textY;
            if (mode == ValueDisplayMode::OnBar) {
                textY = barTop + (barH - sz.cy) / 2;
                if (textY < barTop) textY = barTop + 2;
                bool dark = (item.color.toCOLORREF() & 0x808080) < 0x808080;
                zhuziBrush txtBrush(dark ? zhuziColor(255, 255, 255) : zhuziColor(0, 0, 0));
                paint.drawText(str, textX, textY, txtBrush, font);
            }
            else if (mode == ValueDisplayMode::AboveBar) {
                textY = barTop - sz.cy - 2;
                if (textY < m_marginTop - m_scrollY) continue;
                zhuziBrush textBrush(zhuziColor(0, 0, 0));
                paint.drawText(str, textX, textY, textBrush, font);
            }
        }
    }

    LRESULT CALLBACK zhuziBarChart::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        zhuziBarChart* pThis = (zhuziBarChart*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (!pThis) return DefWindowProcW(hwnd, msg, wParam, lParam);
        return pThis->handleMessage(msg, wParam, lParam);
    }

    LRESULT zhuziBarChart::handleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(m_hwnd, &ps);
            if (hdc) {
                RECT rc; GetClientRect(m_hwnd, &rc);
                int32_t w = rc.right - rc.left, h = rc.bottom - rc.top;
                if (w > 0 && h > 0) {
                    HDC memDC = CreateCompatibleDC(hdc);
                    HBITMAP memBmp = CreateCompatibleBitmap(hdc, w, h);
                    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);
                    zhuziPaint paint(memDC, rc);
                    onPaint(paint);
                    BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);
                    SelectObject(memDC, oldBmp);
                    DeleteObject(memBmp);
                    DeleteDC(memDC);
                }
                EndPaint(m_hwnd, &ps);
            }
            return 0;
        }
        case WM_ERASEBKGND: return 1;
        case WM_HSCROLL: {
            int32_t newX = m_scrollX, code = LOWORD(wParam);
            SCROLLINFO si = { sizeof(SCROLLINFO), SIF_TRACKPOS | SIF_POS | SIF_RANGE | SIF_PAGE };
            GetScrollInfo(m_hwnd, SB_HORZ, &si);
            switch (code) {
            case SB_LINELEFT:   newX -= 20; break;
            case SB_LINERIGHT:  newX += 20; break;
            case SB_PAGELEFT:   newX -= si.nPage; break;
            case SB_PAGERIGHT:  newX += si.nPage; break;
            case SB_THUMBTRACK: newX = si.nTrackPos; break;
            case SB_THUMBPOSITION: newX = HIWORD(wParam); break;
            }
            setScrollOffset(newX, m_scrollY);
            return 0;
        }
        case WM_VSCROLL: {
            int32_t newY = m_scrollY, code = LOWORD(wParam);
            SCROLLINFO si = { sizeof(SCROLLINFO), SIF_TRACKPOS | SIF_POS | SIF_RANGE | SIF_PAGE };
            GetScrollInfo(m_hwnd, SB_VERT, &si);
            switch (code) {
            case SB_LINEUP:     newY -= 20; break;
            case SB_LINEDOWN:   newY += 20; break;
            case SB_PAGEUP:     newY -= si.nPage; break;
            case SB_PAGEDOWN:   newY += si.nPage; break;
            case SB_THUMBTRACK: newY = si.nTrackPos; break;
            case SB_THUMBPOSITION: newY = HIWORD(wParam); break;
            }
            setScrollOffset(m_scrollX, newY);
            return 0;
        }
        case WM_SIZE:
            updateContentSize();
            updateScrollBars();
            InvalidateRect(m_hwnd, nullptr, TRUE);
            return 0;
        }
        return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }
}