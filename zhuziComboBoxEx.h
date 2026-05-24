#pragma once
#include "zhuziControl.h"
#include "zhuziImageList.h"
#include <vector>
#include <functional>

namespace zhuzi {

    class zhuziComboBoxEx : public zhuziControl {
    public:
        zhuziComboBoxEx(zhuziControl* parent = nullptr);
        virtual ~zhuziComboBoxEx();

        virtual bool onCreate(DWORD style) override;
        virtual void destroy() override;
        virtual void onParentResize(int parentWidth, int parentHeight) override;

        void setImageList(zhuziImageList* imageList);
        zhuziImageList* getImageList() const { return m_pImageList; }

        int addItem(const zhuziString& text, int imageIndex = -1);
        void addItems(const std::vector<zhuziString>& items);
        void clear();
        bool deleteItem(int index);

        int getSelectedIndex() const;
        zhuziString getSelectedText() const;
        void setSelectedIndex(int index);

        int getCount() const;

        void setItemData(int index, LPARAM data);
        LPARAM getItemData(int index) const;
        LPARAM getSelectedItemData() const;

        void setOnSelChange(std::function<void()> callback);

    private:
        zhuziImageList* m_pImageList;
        HIMAGELIST m_ownImageList;
        std::function<void()> m_onSelChange;

        void ensureImageListCreated();
        void updateItemHeight();

        static LRESULT CALLBACK ComboBoxExSubclassProc(HWND hwnd, UINT msg,
            WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    };
    
} // namespace zhuzi