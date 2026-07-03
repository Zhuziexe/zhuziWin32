#pragma once
#include "zhuziControl.h"
#include <vector>
#include <functional>
#include "zhuziImageList.h"

namespace zhuzi {

    // ��ť
    class zhuziButton : public zhuziControl {
    public:
        zhuziButton(zhuziControl* parent = nullptr);
        ~zhuziButton();

        virtual bool onCreate(DWORD style) override;
        void setOnClick(std::function<void()> callback);
    };

    // ��̬�ı�
    class zhuziLabel : public zhuziControl {
    public:
        zhuziLabel(zhuziControl* parent = nullptr);
        ~zhuziLabel();
        void setTextAlign(AlignHorizontal align);
        void setVerticalAlign(AlignVertical align);

        // ��ȡ��ǰ���뷽ʽ
        AlignHorizontal getTextAlign() const { return m_hAlign; }
        AlignVertical getVerticalAlign() const { return m_vAlign; }
        virtual bool onCreate(DWORD style) override;

        void onPaint(zhuziPaint& paint) override;
        void setSingleLine(bool enable);
        bool getTransparent() const override { return 1; } // ��ǩĬ��͸������
    private:
        AlignHorizontal m_hAlign = AlignHorizontal::Left;
        AlignVertical   m_vAlign = AlignVertical::Top;
        DWORD           m_baseStyle;      // �洢������ʽ����������λ��
        bool m_singleline = false;
        void updateStyle();
    };

    // �б���
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

    // ��Ͽ�
    class zhuziComboBox : public zhuziControl {
    public:
        zhuziComboBox(zhuziControl* parent = nullptr);
        ~zhuziComboBox();

        virtual bool onCreate(DWORD style) override;

        // ����������ʽ��true: �����б� CBS_DROPDOWNLIST��false: �ɱ༭���� CBS_DROPDOWN��
        void setDropdownList(bool dropdownList);
        // �����Ƿ�ɱ༭����ͬ�� setDropdownList(false)��
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

    // ��ѡ��
    class zhuziCheckButton : public zhuziControl {
    public:
        zhuziCheckButton(zhuziControl* parent = nullptr);
        ~zhuziCheckButton();

        virtual bool onCreate(DWORD style) override;

        void setChecked(bool checked);
        bool isChecked() const;
        void setOnCheck(std::function<void(bool)> callback);
    };

    // ��ѡ��ť
    class zhuziRadioButton : public zhuziControl {
    public:
        zhuziRadioButton(zhuziControl* parent = nullptr);
        ~zhuziRadioButton();

        virtual bool onCreate(DWORD style) override;

        void setChecked(bool checked);
        bool isChecked() const;
        void setOnCheck(std::function<void(bool)> callback);
    };

    // �������
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

        virtual void onPaint(zhuziPaint& paint) override;
    private:
        HBRUSH m_hBrush;
    };
} // namespace zhuzi