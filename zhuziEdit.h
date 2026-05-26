#pragma once
#include "zhuziControl.h"
#include <functional>
#include <memory>

namespace zhuzi {

    class zhuziEdit : public zhuziControl {
    public:
        zhuziEdit(zhuziControl* parent = nullptr);
        ~zhuziEdit();

        virtual bool onCreate(DWORD style) override;

        void setText(const zhuziString& text);
        zhuziString getText() const;

        void setPassword(bool enable);
        void setOnlyNumber(bool enable);

        void setOnEnter(std::function<void()> callback);

    private:
        std::function<void()> m_onEnter;
        bool m_isPassword;
        bool m_isOnlyNumber;

        static LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        void onEnter();
        static std::shared_ptr<zhuziFont> font_tahoma;
    };

    class zhuziText : public zhuziControl {
    public:
        zhuziText(zhuziControl* parent = nullptr);
        ~zhuziText();

        virtual bool onCreate(DWORD style) override;

        void setText(const zhuziString& text);
        zhuziString getText() const;
        void appendText(const zhuziString& text);
        void clear();

        void scrollToTop();
        void scrollToBottom();
        void scrollToLine(int lineIndex);
        void selectAll();
        void setSelection(int start, int end);
        void getSelection(int& start, int& end) const;
        void setReadOnly(bool readOnly);
        bool isReadOnly() const;

        void setOnTextChange(std::function<void()> callback);

    private:
        std::function<void()> m_onTextChange;
        static LRESULT CALLBACK TextProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        void onTextChanged();
    };

} // namespace zhuzi