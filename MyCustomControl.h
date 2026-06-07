#pragma once
#include "zhuziControl.h"

class MyCustomControl : public zhuzi::zhuziControl {
public:
    MyCustomControl(zhuziControl* parent) : zhuziControl(parent) {
        setUseD2D(true);
        m_isCustomDraw = true;
    }
    virtual ~MyCustomControl() = default;

    virtual bool onCreate(DWORD style) override {
        return createControl(L"STATIC", 0, 0, 0, 0, style | WS_CHILD | WS_VISIBLE);
    }

    virtual void onPaintD2D(zhuzi::zhuziD2DRenderTarget& d2d) override {
        d2d.clear(RGBA(0, 0, 0, 0));  // Í¸Ã÷±³¾°

        int w, h;
        d2d.getSize(w, h);
        float cx = w / 2.0f;
        float cy = h / 2.0f;

        zhuzi::zhuziD2DBrush whiteBrush(d2d.get(), RGB(255, 255, 255));
		d2d.fillRectangle(0,0,w,h, whiteBrush);

		zhuzi::zhuziD2DBrush blackBrush(d2d.get(), RGB(0, 0, 0));
        zhuzi::zhuziFont font(L"Segoe UI", 16);
        d2d.drawText(L"Text From Direct2D", cx - 40, cy - 10, blackBrush, font);
    }
};