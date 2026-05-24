#pragma once
#include "zhuziControl.h"
#include "zhuziImageList.h"
#include <vector>

namespace zhuzi {

    class zhuziStatusBar : public zhuziControl {
    public:
        zhuziStatusBar(zhuziControl* parent = nullptr);
        virtual ~zhuziStatusBar();

        bool create();
        virtual bool onCreate(DWORD style) override;

        void setText(const zhuziString& text);
        zhuziString getText() const;

        void setParts(const std::vector<int>& partWidths);
        int getPartCount() const;
        void setPartText(int partIndex, const zhuziString& text);
        zhuziString getPartText(int partIndex) const;

        void setPartIcon(int partIndex, HICON hIcon);
        void setPartIcon(int partIndex, zhuziImageList& imageList, int imageIndex);

        virtual void onParentResize(int parentWidth, int parentHeight) override;

    private:
        std::vector<int> m_partWidths;
        bool m_isAutoBottom;
        std::vector<HICON> m_partIcons;   // 保存每个部分的图标句柄，用于自动释放

        void updateParts();
        void refresh();
    };

} // namespace zhuzi