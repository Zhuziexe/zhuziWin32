#pragma once
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include "zhuziString.h"
#include "zhuziFont.h"
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

namespace zhuzi {

    class zhuziD2DFactory {
    public:
        static zhuziD2DFactory& instance();
        ID2D1Factory* getD2DFactory() const { return m_pD2DFactory; }
        IDWriteFactory* getDWriteFactory() const { return m_pDWriteFactory; }
        IWICImagingFactory* getWICFactory() const { return m_pWICFactory; }
    private:
        zhuziD2DFactory();
        ~zhuziD2DFactory();
        zhuziD2DFactory(const zhuziD2DFactory&) = delete;
        zhuziD2DFactory& operator=(const zhuziD2DFactory&) = delete;
        ID2D1Factory* m_pD2DFactory;
        IDWriteFactory* m_pDWriteFactory;
        IWICImagingFactory* m_pWICFactory;
    };

    class zhuziD2DBrush {
    public:
        zhuziD2DBrush(ID2D1RenderTarget* pRT, COLORREF color, float opacity = 1.0f);
        ~zhuziD2DBrush();
        ID2D1SolidColorBrush* get() const { return m_pBrush; }
    private:
        ID2D1SolidColorBrush* m_pBrush;
    };

    class zhuziD2DPen {
    public:
        zhuziD2DPen(ID2D1RenderTarget* pRT, COLORREF color, float width = 1.0f,
            D2D1_DASH_STYLE dashStyle = D2D1_DASH_STYLE_SOLID);
        ~zhuziD2DPen();
        ID2D1SolidColorBrush* getBrush() const { return m_pBrush; }
        float getWidth() const { return m_width; }
        ID2D1StrokeStyle* getStrokeStyle() const { return m_pStrokeStyle; }
    private:
        ID2D1SolidColorBrush* m_pBrush;
        float m_width;
        ID2D1StrokeStyle* m_pStrokeStyle;
    };

    class zhuziD2DPathGeometry {
    public:
        zhuziD2DPathGeometry();
        ~zhuziD2DPathGeometry();
        bool begin();
        void moveTo(float x, float y);
        void lineTo(float x, float y);
        void cubicBezierTo(float x1, float y1, float x2, float y2, float x3, float y3);
        bool end();
        ID2D1Geometry* get() const { return m_pGeometry; }
    private:
        ID2D1PathGeometry* m_pGeometry;
        ID2D1GeometrySink* m_pSink;
        bool m_open;
    };

    class zhuziD2DRenderTarget {
    public:
        zhuziD2DRenderTarget(HWND hwnd);
        ~zhuziD2DRenderTarget();

        bool beginDraw();
        bool endDraw();
        void clear(COLORREF color);
        void resize();
        ID2D1HwndRenderTarget* get() const { return m_pRT; }

        void drawLine(float x1, float y1, float x2, float y2, const zhuziD2DPen& pen);
        void drawRectangle(float left, float top, float right, float bottom, const zhuziD2DPen& pen, const zhuziD2DBrush* fill = nullptr);
        void fillRectangle(float left, float top, float right, float bottom, const zhuziD2DBrush& brush);
        void drawEllipse(float cx, float cy, float rx, float ry, const zhuziD2DPen& pen, const zhuziD2DBrush* fill = nullptr);
        void fillEllipse(float cx, float cy, float rx, float ry, const zhuziD2DBrush& brush);
        void drawRoundedRectangle(float left, float top, float right, float bottom, float radiusX, float radiusY,
            const zhuziD2DPen& pen, const zhuziD2DBrush* fill = nullptr);
        void fillRoundedRectangle(float left, float top, float right, float bottom, float radiusX, float radiusY,
            const zhuziD2DBrush& brush);
        void drawGeometry(const zhuziD2DPathGeometry& geometry, const zhuziD2DPen& pen, const zhuziD2DBrush* fill = nullptr);
        void fillGeometry(const zhuziD2DPathGeometry& geometry, const zhuziD2DBrush& brush);

        void drawText(const zhuziString& text, float x, float y, const zhuziD2DBrush& brush, const zhuziFont& font);
        void drawText(const zhuziString& text, const RECT& rect, const zhuziD2DBrush& brush, const zhuziFont& font,
            DWORD format = DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        void pushTransform(float offsetX, float offsetY, float angleDeg = 0, float scaleX = 1.0f, float scaleY = 1.0f);
        void popTransform();
        void getSize(int& width, int& height) const;

    private:
        HWND m_hwnd;
        ID2D1HwndRenderTarget* m_pRT;
        ID2D1Factory* m_pFactory;
        IDWriteFactory* m_pDWriteFactory;
        D2D1_SIZE_U m_size;
        bool m_drawing;
    };

} // namespace zhuzi