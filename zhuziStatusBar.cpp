#include "zhuziStatusBar.h"
#include "zhuziInstance.h"
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")

namespace zhuzi {

    zhuziStatusBar::zhuziStatusBar(zhuziControl* parent)
        : zhuziControl(parent), m_isAutoBottom(false) {
    }

    zhuziStatusBar::~zhuziStatusBar() {
        for (HICON hIcon : m_partIcons) {
            if (hIcon) DestroyIcon(hIcon);
        }
        destroy();
    }

    bool zhuziStatusBar::create() {
        m_isAutoBottom = true;
        if (m_hwnd) return false;
        if (m_id == -1) {
            try { m_id = allocateId(); }
            catch (...) { return false; }
        }
        HWND hParent = m_parent ? m_parent->getHandle() : nullptr;
        m_hwnd = CreateWindowExW(0, STATUSCLASSNAMEW, L"",
            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
            0, 0, 0, 0, hParent,
            (HMENU)(INT_PTR)m_id, zhuziInstance::getHandle(), nullptr);
        if (!m_hwnd) {
            releaseId(m_id); m_id = -1;
            return false;
        }
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
        return true;
    }

    bool zhuziStatusBar::onCreate(DWORD style) {
        if (m_hwnd) return false;
        if (m_id == -1) {
            try { m_id = allocateId(); }
            catch (...) { return false; }
        }
        HWND hParent = m_parent ? m_parent->getHandle() : nullptr;
        DWORD finalStyle = style | WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP;
        m_hwnd = CreateWindowExW(0, STATUSCLASSNAMEW, L"", finalStyle,
            0, 0, 0, 0, hParent,
            (HMENU)(INT_PTR)m_id, zhuziInstance::getHandle(), nullptr);
        if (!m_hwnd) {
            releaseId(m_id); m_id = -1;
            return false;
        }
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
        if (m_partWidths.empty()) {
            int parts[] = { -1 };
            SendMessageW(m_hwnd, SB_SETPARTS, 1, (LPARAM)parts);
        }
        return true;
    }

    void zhuziStatusBar::setText(const zhuziString& text) {
        if (m_hwnd) SendMessageW(m_hwnd, SB_SETTEXTW, 0, (LPARAM)text.c_str());
    }

    zhuziString zhuziStatusBar::getText() const {
        if (!m_hwnd) return L"";
        wchar_t buf[256] = { 0 };
        SendMessageW(m_hwnd, SB_GETTEXTW, 0, (LPARAM)buf);
        return zhuziString(buf);
    }

    void zhuziStatusBar::setParts(const std::vector<int>& partWidths) {
        m_partWidths = partWidths;
        updateParts();
    }

    int zhuziStatusBar::getPartCount() const {
        if (!m_hwnd) return 0;
        return (int)SendMessageW(m_hwnd, SB_GETPARTS, 0, 0);
    }

    void zhuziStatusBar::setPartText(int partIndex, const zhuziString& text) {
        if (m_hwnd) {
            SendMessageW(m_hwnd, SB_SETTEXTW, partIndex, (LPARAM)text.c_str());
        }
    }

    zhuziString zhuziStatusBar::getPartText(int partIndex) const {
        if (!m_hwnd) return L"";
        wchar_t buf[256] = { 0 };
        SendMessageW(m_hwnd, SB_GETTEXTW, partIndex, (LPARAM)buf);
        return zhuziString(buf);
    }

    void zhuziStatusBar::setPartIcon(int partIndex, HICON hIcon) {
        if (!m_hwnd || !hIcon) return;

        // 获取状态栏高度
        RECT rc;
        GetWindowRect(m_hwnd, &rc);
        int targetHeight = rc.bottom - rc.top;
        if (targetHeight <= 0) targetHeight = 20;

        // 获取原始图标尺寸并缩放
        ICONINFO iconInfo = { 0 };
        if (GetIconInfo(hIcon, &iconInfo)) {
            BITMAP bm = { 0 };
            int origWidth = 16, origHeight = 16;
            if (iconInfo.hbmColor) {
                GetObject(iconInfo.hbmColor, sizeof(bm), &bm);
                origWidth = bm.bmWidth;
                origHeight = bm.bmHeight;
                DeleteObject(iconInfo.hbmColor);
            }
            if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);

            // 如果高度不同，进行缩放
            if (origHeight != targetHeight) {
                int newWidth = targetHeight * origWidth / origHeight;
                HICON hScaled = (HICON)CopyImage(hIcon, IMAGE_ICON, newWidth, targetHeight, LR_COPYDELETEORG);
                if (hScaled) {
                    // 缩放成功，销毁原图标（原图标来自 extractIcon，可安全销毁）
                    DestroyIcon(hIcon);
                    hIcon = hScaled;
                }
            }
        }

        // 保存图标句柄并设置到状态栏
        if (partIndex >= (int)m_partIcons.size())
            m_partIcons.resize(partIndex + 1, nullptr);
        if (m_partIcons[partIndex])
            DestroyIcon(m_partIcons[partIndex]);
        m_partIcons[partIndex] = hIcon;
        SendMessageW(m_hwnd, SB_SETICON, partIndex, (LPARAM)hIcon);
    }

    void zhuziStatusBar::setPartIcon(int partIndex, zhuziImageList& imageList, int imageIndex) {
        HICON hIcon = imageList.extractIcon(imageIndex);
        if (hIcon) {
            setPartIcon(partIndex, hIcon);
        }
    }

    void zhuziStatusBar::updateParts() {
        if (!m_hwnd || m_partWidths.empty()) return;
        std::vector<int> widths = m_partWidths;
        if (widths.back() != -1) widths.push_back(-1);
        SendMessageW(m_hwnd, SB_SETPARTS, (WPARAM)widths.size(), (LPARAM)widths.data());
    }

    void zhuziStatusBar::refresh() {
        if (m_hwnd) {
            InvalidateRect(m_hwnd, nullptr, TRUE);
            UpdateWindow(m_hwnd);
        }
    }

    void zhuziStatusBar::onParentResize(int parentWidth, int parentHeight) {
        if (!m_hwnd) return;
        if (m_isAutoBottom) {
            RECT rc;
            GetWindowRect(m_hwnd, &rc);
            int height = rc.bottom - rc.top;
            if (height == 0) height = 20;
            int y = parentHeight - height;
            SetWindowPos(m_hwnd, nullptr, 0, y, parentWidth, height, SWP_NOZORDER);
        }
        // 注意：缩放时状态栏高度可能改变，但图标不会自动重新缩放，如需动态调整请重新调用 setPartIcon
    }

} // namespace zhuzi