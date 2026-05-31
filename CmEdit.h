#pragma once
#include "zhuziControl.h"
#include "zhuziPaint.h"
#include <functional>
#include <vector>

namespace zhuzi {

    enum EditCallbackIndex {
        EDCB_TEXTCHANGED = 0,
        EDCB_RETURNPRESSED,
        EDCB_FOCUS,
        EDCB_CHARINPUT,
        EDCB_MAX
    };

    class CmEdit : public zhuziControl {
    public:
        CmEdit(zhuziControl* parent = nullptr);
        virtual ~CmEdit();

        virtual bool onCreate(DWORD style) override;
        virtual void destroy() override;

        // 基本属性
        void setReadOnly(bool readOnly);
        bool isReadOnly() const;

        void setPasswordChar(wchar_t ch = L'*');
        void disablePassword();

        void setNumberOnly(bool enable);
        bool isNumberOnly() const;

        void setMultiLine(bool enable);
        bool isMultiLine() const;

        void setMaxLength(int max);
        int getMaxLength() const;

        // 颜色
        void setBackgroundColor(const zhuziColor& color);
        void setTextColor(const zhuziColor& color);
        void resetColors();

        // 对齐
        enum class Alignment { Left, Center, Right };
        void setAlignment(Alignment align);
        Alignment getAlignment() const;

        // 文本操作
        void appendText(const zhuziString& text);
        void clear();
        void cut();
        void copy();
        void paste();
        bool canUndo() const;
        void undo();
        bool canRedo() const;
        void redo();

        // 选择操作
        void setSelection(int start, int end);
        void setSelectionAll();
        void setCursorPos(int pos);
        std::pair<int, int> getSelection() const;
        zhuziString getSelectedText() const;
        void replaceSelection(const zhuziString& text);

        // 指定范围操作
        void setRangeText(int start, int end, const zhuziString& text);
        void setRangeColor(int start, int end, const zhuziColor& color);
        void setRangeFont(int start, int end, const zhuziFont& font);

        // 多行扩展
        int getLineCount() const;
        int getLineIndex(int line) const;
        int getLineLength(int line) const;
        zhuziString getLineText(int line) const;
        int getCurrentLine() const;
        void scrollToLine(int line);
        void scrollToCaret();

        // 富文本扩展
        void setFont(const zhuziFont& font);
        void setSelectionFont(const zhuziFont& font);
        void setSelectionColor(const zhuziColor& color);

        // 事件回调（原有）
        void setOnTextChanged(std::function<void()> callback);
        void setOnReturnPressed(std::function<void()> callback);
        void setOnFocus(std::function<void(bool)> callback);
        void setOnCharInput(std::function<bool(wchar_t)> callback);

        bool loadRTF(const zhuziString& filePath);
        bool saveRTF(const zhuziString& filePath);

        // 获取当前选中区域第一个字符的字体（若没有选中，则获取光标处字体）
        zhuziFont getSelectionFont() const;
        // 获取指定范围内第一个字符的字体
        zhuziFont getRangeFont(int start, int end) const;

        // 获取当前选中文本的颜色
        zhuziColor getSelectionColor() const;
        // 获取指定范围内第一个字符的颜色
        zhuziColor getRangeColor(int start, int end) const;
    protected:
        static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        LRESULT handleSubclassMessage(UINT msg, WPARAM wParam, LPARAM lParam);

        void updateAlignment();
        void applyDefaultFormat();
        void ensureRichEditLoaded();
        bool recreateWithCurrentSettings();

        // 新：自动注册/注销颜色处理器
        void installAutoColorHandler();
        void uninstallAutoColorHandler();

    private:
        // 位域布尔成员
        unsigned m_isNumberOnly : 1;
        unsigned m_isMultiLine : 1;
        unsigned m_isPassword : 1;
        unsigned m_isReadOnly : 1;
        unsigned m_fontSet : 1;
        unsigned m_isSubclassed : 1;

        int m_maxLength;
        Alignment m_alignment;
        zhuziColor m_bgColor;
        zhuziColor m_textColor;
        HBRUSH   m_hBgBrush;
        zhuziFont m_defaultFont;

        std::vector<void*> m_callbacks;  // 存储旧的回调函数

        int m_colorHandlerId;   // 从父窗口 Bind 返回的 ID，用于 Unbind

        static bool s_richeditLoaded;
    };

} // namespace zhuzi