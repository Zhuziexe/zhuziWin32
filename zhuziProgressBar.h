#pragma once
#include "zhuziControl.h"

namespace zhuzi {

    class zhuziProgressBar : public zhuziControl {
    public:
        zhuziProgressBar(zhuziControl* parent = nullptr);
        ~zhuziProgressBar();

        virtual bool onCreate(DWORD style) override;

        void setRange(int min, int max);
        void setRange32(int min, int max);
        void getRange(int& min, int& max) const;
        int getPos() const;
        void setPos(int pos);
        void deltaPos(int delta);
        void setStep(int step);
        void stepIt();
        void setSmooth(bool smooth);
        void setVertical(bool vertical);
        void setMarquee(bool enable, DWORD timeMs = 0);
        bool isMarquee() const;
        void setBarColor(COLORREF color);
        void setBackgroundColor(COLORREF color);

    private:
        DWORD m_styleExtra;
    };

}