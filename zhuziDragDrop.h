#pragma once
#include <windows.h>
#include <functional>
#include <vector>
#include "zhuziString.h"
#include "zhuziControl.h"

namespace zhuzi {

    const UINT WM_ZHUZI_DROP = WM_APP + 0x1234;

    struct DragData {
        std::vector<zhuziString> files;
        zhuziString text;
        zhuziString html;
        bool isFileDrop() const { return !files.empty(); }
        bool isTextDrop() const { return !text.empty(); }
    };

    class DragSourceData {
    public:
        DragSourceData() = default;
        DragSourceData& addFile(const zhuziString& path);
        DragSourceData& addFiles(const std::vector<zhuziString>& paths);
        DragSourceData& setText(const zhuziString& text);
        DragSourceData& setHtml(const zhuziString& html);
        const std::vector<zhuziString>& getFiles() const { return m_files; }
        const zhuziString& getText() const { return m_text; }
        const zhuziString& getHtml() const { return m_html; }
        IDataObject* createDataObject() const;
    private:
        std::vector<zhuziString> m_files;
        zhuziString m_text;
        zhuziString m_html;
    };

    class zhuziDropTarget {
    public:
        zhuziDropTarget(zhuziControl* targetCtrl, std::function<void(const DragData&)> callback, bool async = true);
        ~zhuziDropTarget();
        zhuziDropTarget(const zhuziDropTarget&) = delete;
        zhuziDropTarget& operator=(const zhuziDropTarget&) = delete;
        zhuziDropTarget(zhuziDropTarget&& other) noexcept;
        zhuziDropTarget& operator=(zhuziDropTarget&& other) noexcept;
        bool isValid() const;
        void revoke();
    private:
        HWND m_targetHwnd;
        zhuziControl* m_targetCtrl;
        void* m_pTarget;
        bool m_registered;
        bool m_async;
        std::function<void(const DragData&)> m_callback;
    };

    class zhuziDragSource {
    public:
        zhuziDragSource(zhuziControl* sourceCtrl,
            std::function<DragSourceData()> dataProvider,
            DWORD allowedEffects = DROPEFFECT_COPY | DROPEFFECT_MOVE,
            bool showDragImage = false);
        ~zhuziDragSource();
        zhuziDragSource(const zhuziDragSource&) = delete;
        zhuziDragSource& operator=(const zhuziDragSource&) = delete;
        zhuziDragSource(zhuziDragSource&& other) noexcept;
        zhuziDragSource& operator=(zhuziDragSource&& other) noexcept;
        void revoke();
    private:
        HWND m_hwnd;
        bool m_subclassed;
        POINT m_ptDown;
        bool m_draggingPossible;
        bool m_dragging;
        std::function<DragSourceData()> m_dataProvider;
        DWORD m_allowedEffects;
        bool m_showDragImage;
        static LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
            UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
        void onLButtonDown(LPARAM lParam);
        void onMouseMove(LPARAM lParam);
        void onLButtonUp();
        bool checkThreshold(int x, int y) const;
    };

    DWORD DoDrag(const DragSourceData& data, DWORD allowedEffects = DROPEFFECT_COPY | DROPEFFECT_MOVE);
    DWORD DoDragFiles(const std::vector<zhuziString>& files, DWORD allowedEffects = DROPEFFECT_COPY | DROPEFFECT_MOVE);
    DWORD DoDragText(const zhuziString& text, DWORD allowedEffects = DROPEFFECT_COPY);

    inline const DragData* GetDragDataFromLParam(LPARAM lParam) {
        return reinterpret_cast<const DragData*>(lParam);
    }
    inline void FreeDragData(LPARAM lParam) {
        delete reinterpret_cast<DragData*>(lParam);
    }

} // namespace zhuzi