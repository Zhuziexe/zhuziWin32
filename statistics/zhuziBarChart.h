#pragma once
#include "../zhuziControl.h"
#include "../zhuziPaint.h"
#include "../zhuziInstance.h"
#include <vector>
#include <cstdint>

namespace zhuzi {

    class zhuziBarChart : public zhuziControl {
    public:
        enum class ValueDisplayMode : uint8_t { None, OnBar, AboveBar };

        struct BarItem {
            zhuziString name;
            int32_t value;
            zhuziColor color;
            BarItem() : name(L""), value(0), color(0, 0, 0) {}
            BarItem(const zhuziString& n, int32_t v, const zhuziColor& c) : name(n), value(v), color(c) {}
        };

        zhuziBarChart(zhuziControl* parent = nullptr);
        virtual ~zhuziBarChart();

        virtual bool onCreate(DWORD style) override;
        virtual void onPaint(zhuziPaint& paint) override;
        virtual void onParentResize(int parentWidth, int parentHeight) override;

        int32_t addItem(const zhuziString& itemName, int32_t height, const zhuziColor& color = zhuziColor(0, 0, 255), bool bRedraw = true);
        void removeItem(int32_t index);
        void clearItems();
        void updateItem(int32_t index, const zhuziString& itemName, int32_t height, const zhuziColor& color);
        int32_t getItemCount() const { return (int32_t)m_items.size(); }
        const BarItem& getItem(int32_t index) const { return m_items[index]; }

        void setYAxisRange(int32_t minValue, int32_t maxValue);
        void setYAxisMinOmit(int32_t omitValue);
        void setYAxisStep(int32_t step);
        void setBarWidth(uint16_t width);
        void setBarSpacing(uint16_t spacing);
        void setDefaultBarColor(const zhuziColor& color);
        void setBackgroundColor(const zhuziColor& color);
        void setValueDisplayMode(ValueDisplayMode mode);
        void setAutoScrollBars(bool enable);
        void setZoom(float zoomX, float zoomY);
        void setMargins(uint16_t left, uint16_t top, uint16_t right, uint16_t bottom);
        void setYAxisSpacing(int16_t spacing);

        int32_t getScrollX() const { return m_scrollX; }
        int32_t getScrollY() const { return m_scrollY; }
        void setScrollOffset(int32_t x, int32_t y);
        void setHorizontalScrollBarPolicy(bool alwaysOn);
        void setVerticalScrollBarPolicy(bool alwaysOn);
        void setDrawBreakLine(bool draw);

        void redraw() { Invalidate(); }

    protected:
        void updateScrollBars();
        void updateContentSize();
        void drawAxesAndLabels(zhuziPaint& paint);
        void drawBars(zhuziPaint& paint);
        void drawValueTexts(zhuziPaint& paint);
        void drawBreakSymbol(zhuziPaint& paint, int32_t x, int32_t y);

        int32_t getBarX(int32_t index) const;
        int32_t getValueY(int32_t value) const;
        int32_t getClientWidth() const;
        int32_t getClientHeight() const;
        int32_t getContentPlotHeight() const;

        static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

        // Î»Óò·ÃÎÊÆ÷£¨m_flag2 ÊÇ int8_t£©
        bool getAutoYRange() const { return (m_flag2 & 0x01) != 0; }
        void setAutoYRange(bool v) { if (v) m_flag2 |= 0x01; else m_flag2 &= ~0x01; }
        bool getHScrollAlways() const { return (m_flag2 & 0x02) != 0; }
        void setHScrollAlways(bool v) { if (v) m_flag2 |= 0x02; else m_flag2 &= ~0x02; }
        bool getVScrollAlways() const { return (m_flag2 & 0x04) != 0; }
        void setVScrollAlways(bool v) { if (v) m_flag2 |= 0x04; else m_flag2 &= ~0x04; }
        bool getAutoScrollBars() const { return (m_flag2 & 0x08) != 0; }
        void setAutoScrollBarsFlag(bool v) { if (v) m_flag2 |= 0x08; else m_flag2 &= ~0x08; }
        bool getDrawBreakLine() const { return (m_flag2 & 0x10) != 0; }
        void setDrawBreakLineFlag(bool v) { if (v) m_flag2 |= 0x10; else m_flag2 &= ~0x10; }
        ValueDisplayMode getValueDisplayMode() const { return (ValueDisplayMode)((m_flag2 >> 6) & 0x03); }
        void setValueDisplayModeFlag(ValueDisplayMode mode) {
            m_flag2 = (m_flag2 & ~0xC0) | (((uint8_t)mode & 0x03) << 6);
        }

    private:
        std::vector<BarItem> m_items;
        zhuziColor m_defaultBarColor;
        zhuziColor m_backgroundColor;

        int32_t m_yMin, m_yMax;
        int32_t m_yMinOmit;
        int32_t m_yStep;

        uint16_t m_barWidth;
        uint16_t m_barSpacing;
        uint16_t m_marginLeft, m_marginTop, m_marginRight, m_marginBottom;
        int32_t m_scrollX, m_scrollY;
        int32_t m_contentWidth, m_contentHeight;
        float m_zoomX, m_zoomY;

        static bool s_classRegistered;
        static const wchar_t* WINDOW_CLASS_NAME;
    };
}