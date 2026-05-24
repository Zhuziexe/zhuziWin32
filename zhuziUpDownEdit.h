#pragma once
#include "zhuziControl.h"
#include <commctrl.h>
#include <functional>

namespace zhuzi {

    class zhuziUpDownEdit : public zhuziControl {
    public:
        zhuziUpDownEdit(zhuziControl* parent = nullptr);
        virtual ~zhuziUpDownEdit();

        virtual bool onCreate(DWORD style) override;
        virtual void destroy() override;

        void setValue(int value);
        int getValue() const;
        void setRange(int min, int max);
        void getRange(int& min, int& max) const;
        void setStep(int step);
        int getStep() const;
        void setHexMode(bool hex);
        bool isHexMode() const;
        void setThousandsSeparator(bool enable);
        bool hasThousandsSeparator() const;
        void setReadOnly(bool readOnly);
        HWND getEditHandle() const { return m_hwndEdit; }
        HWND getUpDownHandle() const { return m_hwndUpDown; }
        void setOnValueChanged(std::function<void(int newValue)> callback);
        void setControlText(const zhuziString& text);
        zhuziString getControlText() const;
        virtual void enable(bool enabled = true) override;
        virtual void onParentResize(int parentWidth, int parentHeight) override;

    protected:
        bool createControls();
        void updateEditText();
        void updateValueFromEdit();
        bool isAllowedChar(wchar_t ch) const;

        static LRESULT CALLBACK EditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        LRESULT handleEditMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    private:
        HWND m_hwndEdit;
        HWND m_hwndUpDown;
        int m_value;
        int m_min;
        int m_max;
        int m_step;
        bool m_hexMode;
        bool m_thousandsSep;
        bool m_isSubclassed;
        std::function<void(int)> m_onValueChanged;
    };

}