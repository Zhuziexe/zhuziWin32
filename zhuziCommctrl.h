#pragma once
#include "zhuziControl.h"
#include <vector>
#include <functional>
#include "zhuziImageList.h"

namespace zhuzi {

    // 按钮
    class zhuziButton : public zhuziControl {
    public:
        zhuziButton(zhuziControl* parent = nullptr);
        ~zhuziButton();

        virtual bool onCreate(DWORD style) override;
        void setOnClick(std::function<void()> callback);
    };

    // 静态文本
    class zhuziLabel : public zhuziControl {
    public:
        zhuziLabel(zhuziControl* parent = nullptr);
        ~zhuziLabel();
        enum class AlignHorizontal {
            Left,   // 左对齐
            Center, // 水平居中
            Right   // 右对齐
        };

        enum class AlignVertical {
            Top,    // 顶部对齐
            Center  // 垂直居中
        };
        void setTextAlign(AlignHorizontal align);
        void setVerticalAlign(AlignVertical align);

        // 获取当前对齐方式
        AlignHorizontal getTextAlign() const { return m_hAlign; }
        AlignVertical getVerticalAlign() const { return m_vAlign; }
        virtual bool onCreate(DWORD style) override;
    private:
        AlignHorizontal m_hAlign;
        AlignVertical   m_vAlign;
        DWORD           m_baseStyle;      // 存储基础样式（不含对齐位）
        void updateStyle();
    };

    // 列表框
    class zhuziListBox : public zhuziControl {
    public:
        zhuziListBox(zhuziControl* parent = nullptr);
        ~zhuziListBox();

        virtual bool onCreate(DWORD style) override;

        void addItem(const zhuziString& item);
        void addItems(const std::vector<zhuziString>& items);
        void deleteItem(int index);
        void clear();
        int getSelectedIndex() const;
        zhuziString getSelectedText() const;
        void setSelectedIndex(int index);
        int getCount() const;
        zhuziString getItemText(int index) const;
        void setItemText(int index, const zhuziString& text);
        void setOnSelChange(std::function<void()> callback);
        void setOnDoubleClick(std::function<void()> callback);
    };

    // 组合框
    class zhuziComboBox : public zhuziControl {
    public:
        zhuziComboBox(zhuziControl* parent = nullptr);
        ~zhuziComboBox();

        virtual bool onCreate(DWORD style) override;

        // 设置下拉样式（true: 下拉列表 CBS_DROPDOWNLIST，false: 可编辑下拉 CBS_DROPDOWN）
        void setDropdownList(bool dropdownList);
        // 设置是否可编辑（等同于 setDropdownList(false)）
        void setEditable(bool editable);

        void addItem(const zhuziString& item);
        void addItems(const std::vector<zhuziString>& items);
        void deleteItem(int index);
        void clear();
        int getSelectedIndex() const;
        zhuziString getSelectedText() const;
        void setSelectedIndex(int index);
        int getCount() const;
        zhuziString getItemText(int index) const;
        void setItemText(int index, const zhuziString& text);
        void setOnSelChange(std::function<void()> callback);
        void setOnDoubleClick(std::function<void()> callback);

    private:
        bool m_isDropdownList;  // true: CBS_DROPDOWNLIST, false: CBS_DROPDOWN
    };

    // 复选框
    class zhuziCheckButton : public zhuziControl {
    public:
        zhuziCheckButton(zhuziControl* parent = nullptr);
        ~zhuziCheckButton();

        virtual bool onCreate(DWORD style) override;

        void setChecked(bool checked);
        bool isChecked() const;
        void setOnCheck(std::function<void(bool)> callback);
    };

    // 单选按钮
    class zhuziRadioButton : public zhuziControl {
    public:
        zhuziRadioButton(zhuziControl* parent = nullptr);
        ~zhuziRadioButton();

        virtual bool onCreate(DWORD style) override;

        void setChecked(bool checked);
        bool isChecked() const;
        void setOnCheck(std::function<void(bool)> callback);
    };

    // 框架容器
    class zhuziFrame : public zhuziControl {
    public:
        enum BorderStyle { None, Simple, Sunken, Raised };

        zhuziFrame(zhuziControl* parent = nullptr);
        virtual ~zhuziFrame();

        virtual bool onCreate(DWORD style) override;

        void setBackgroundColor(COLORREF color);
        void setBackgroundSysColor(int sysColorIndex);
        void setBorderStyle(BorderStyle style);
        void enable(bool enabled) override;

        virtual void onParentResize(int parentWidth, int parentHeight) override;

        HBRUSH getBackgroundBrush() const;

    private:
        HBRUSH m_hBrush;
    };
} // namespace zhuzi