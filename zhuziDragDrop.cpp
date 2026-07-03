#include "zhuziDragDrop.h"
#include "zhuziControl.h"
#include <shlobj.h>
#include <shellapi.h>
#include <oleidl.h>
#include <objbase.h>
#include <unordered_map>
#include <windowsx.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")

#ifndef CFSTR_FILEDESCRIPTORW
#define CFSTR_FILEDESCRIPTORW L"FileGroupDescriptorW"
#endif

namespace zhuzi {

    // ---------- OLE 初始化 ----------
    class OleInitializer {
    public:
        OleInitializer() {
            if (++m_refCount == 1) {
                HRESULT hr = OleInitialize(nullptr);
                m_initialized = SUCCEEDED(hr);
            }
        }
        ~OleInitializer() {
            if (--m_refCount == 0 && m_initialized) {
                OleUninitialize();
                m_initialized = false;
            }
        }
        static OleInitializer& instance() {
            static OleInitializer inst;
            return inst;
        }
        bool isInitialized() const { return m_initialized; }
    private:
        static int m_refCount;
        bool m_initialized = false;
    };
    int OleInitializer::m_refCount = 0;

    // ---------- 全局映射 ----------
    namespace {
        std::unordered_map<HWND, std::function<void(const DragData&)>> g_controlCallbacks;
        std::unordered_map<HWND, bool> g_windowBound;
    }

    static LRESULT GlobalDropMessageHandler(WPARAM wParam, LPARAM lParam) {
        HWND hwndCtrl = reinterpret_cast<HWND>(wParam);
        auto it = g_controlCallbacks.find(hwndCtrl);
        if (it != g_controlCallbacks.end()) {
            DragData* pData = reinterpret_cast<DragData*>(lParam);
            if (pData) {
                it->second(*pData);
                delete pData;
            }
            return TRUE;
        }
        delete reinterpret_cast<DragData*>(lParam);
        return FALSE;
    }

    // ---------- IDropTarget 实现 ----------
    class DropTargetImpl : public IDropTarget {
    public:
        DropTargetImpl(HWND targetHwnd, std::function<void(const DragData&)> syncCallback, bool async)
            : m_refCount(1), m_targetHwnd(targetHwnd), m_syncCallback(syncCallback), m_allowDrop(false), m_async(async) {
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
            if (riid == IID_IUnknown || riid == IID_IDropTarget) {
                *ppv = static_cast<IDropTarget*>(this);
                AddRef();
                return S_OK;
            }
            *ppv = nullptr;
            return E_NOINTERFACE;
        }
        ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refCount); }
        ULONG STDMETHODCALLTYPE Release() override {
            LONG ref = InterlockedDecrement(&m_refCount);
            if (ref == 0) delete this;
            return ref;
        }

        HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
            m_allowDrop = (HasFiles(pDataObj) || HasText(pDataObj));
            *pdwEffect = m_allowDrop ? DROPEFFECT_COPY : DROPEFFECT_NONE;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
            *pdwEffect = m_allowDrop ? DROPEFFECT_COPY : DROPEFFECT_NONE;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE DragLeave() override {
            m_allowDrop = false;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
            *pdwEffect = DROPEFFECT_NONE;
            if (!m_allowDrop) return S_OK;

            DragData data;
            data.files = ExtractFiles(pDataObj);
            data.text = ExtractText(pDataObj, CF_UNICODETEXT);
            if (data.text.empty())
                data.text = ExtractText(pDataObj, CF_TEXT);
            data.html = ExtractText(pDataObj, static_cast<CLIPFORMAT>(RegisterClipboardFormatW(L"HTML Format")));

            // 修改：始终使用 SendMessage 同步，避免 RPC 0x80010001 错误
            if (m_targetHwnd && IsWindow(m_targetHwnd)) {
                HWND topParent = GetAncestor(m_targetHwnd, GA_ROOT);
                if (!topParent) topParent = m_targetHwnd;
                DragData* pData = new DragData(std::move(data));
                // 使用 SendMessage 确保消息被处理完成
                SendMessageW(topParent, WM_ZHUZI_DROP, reinterpret_cast<WPARAM>(m_targetHwnd), reinterpret_cast<LPARAM>(pData));
            }
            else if (m_syncCallback) {
                m_syncCallback(data);
            }

            *pdwEffect = DROPEFFECT_COPY;
            return S_OK;
        }

    private:
        static bool HasFiles(IDataObject* pDataObj) {
            FORMATETC fmte = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
            return pDataObj->QueryGetData(&fmte) == S_OK;
        }
        static bool HasText(IDataObject* pDataObj) {
            FORMATETC fmte = { CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
            if (pDataObj->QueryGetData(&fmte) == S_OK) return true;
            fmte.cfFormat = CF_TEXT;
            return pDataObj->QueryGetData(&fmte) == S_OK;
        }
        static std::vector<zhuziString> ExtractFiles(IDataObject* pDataObj) {
            std::vector<zhuziString> result;
            FORMATETC fmte = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
            STGMEDIUM stg;
            if (SUCCEEDED(pDataObj->GetData(&fmte, &stg))) {
                HDROP hDrop = (HDROP)GlobalLock(stg.hGlobal);
                if (hDrop) {
                    UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
                    result.reserve(count);
                    for (UINT i = 0; i < count; ++i) {
                        wchar_t path[MAX_PATH];
                        DragQueryFileW(hDrop, i, path, MAX_PATH);
                        result.emplace_back(path);
                    }
                    GlobalUnlock(stg.hGlobal);
                }
                ReleaseStgMedium(&stg);
            }
            return result;
        }
        static zhuziString ExtractText(IDataObject* pDataObj, CLIPFORMAT format) {
            zhuziString result;
            FORMATETC fmte = { format, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
            STGMEDIUM stg;
            if (SUCCEEDED(pDataObj->GetData(&fmte, &stg))) {
                if (format == CF_UNICODETEXT || format == RegisterClipboardFormatW(L"HTML Format")) {
                    wchar_t* pText = (wchar_t*)GlobalLock(stg.hGlobal);
                    if (pText) {
                        result = pText;
                        GlobalUnlock(stg.hGlobal);
                    }
                }
                else if (format == CF_TEXT) {
                    char* pText = (char*)GlobalLock(stg.hGlobal);
                    if (pText) {
                        int len = MultiByteToWideChar(CP_ACP, 0, pText, -1, nullptr, 0);
                        if (len > 0) {
                            std::vector<wchar_t> buf(len);
                            MultiByteToWideChar(CP_ACP, 0, pText, -1, buf.data(), len);
                            result = buf.data();
                        }
                        GlobalUnlock(stg.hGlobal);
                    }
                }
                ReleaseStgMedium(&stg);
            }
            return result;
        }

        LONG m_refCount;
        HWND m_targetHwnd;
        std::function<void(const DragData&)> m_syncCallback;
        bool m_allowDrop;
        bool m_async;
    };

    // ---------- zhuziDropTarget 实现 ----------
    zhuziDropTarget::zhuziDropTarget(zhuziControl* targetCtrl, std::function<void(const DragData&)> callback, bool async)
        : m_targetHwnd(nullptr), m_targetCtrl(targetCtrl), m_pTarget(nullptr), m_registered(false), m_async(async), m_callback(callback) {
        if (!targetCtrl || !callback) return;
        if (!OleInitializer::instance().isInitialized()) return;

        m_targetHwnd = targetCtrl->getHandle();
        if (!m_targetHwnd) return;

        zhuziWindow* parentWnd = findParentWindow(targetCtrl);
        if (!parentWnd) return;

        HWND wndHandle = parentWnd->getHandle();
        if (!g_windowBound[wndHandle]) {
            // 使用全局 Bind，回调接收 zhuziMsg*
            Bind(parentWnd, WM_ZHUZI_DROP, [](zhuziMsg* msg) {
                if (GlobalDropMessageHandler(msg->wParam, msg->lParam) == TRUE) {
                    msg->handled = true;
                }
                });
            g_windowBound[wndHandle] = true;
        }

        g_controlCallbacks[m_targetHwnd] = m_callback;

        m_pTarget = new DropTargetImpl(m_targetHwnd, m_callback, m_async);
        HRESULT hr = ::RegisterDragDrop(m_targetHwnd, static_cast<IDropTarget*>(m_pTarget));
        if (SUCCEEDED(hr)) {
            m_registered = true;
        }
        else {
            static_cast<DropTargetImpl*>(m_pTarget)->Release();
            m_pTarget = nullptr;
            g_controlCallbacks.erase(m_targetHwnd);
        }
    }

    zhuziDropTarget::~zhuziDropTarget() { revoke(); }

    zhuziDropTarget::zhuziDropTarget(zhuziDropTarget&& other) noexcept
        : m_targetHwnd(other.m_targetHwnd), m_targetCtrl(other.m_targetCtrl), m_pTarget(other.m_pTarget),
        m_registered(other.m_registered), m_async(other.m_async), m_callback(std::move(other.m_callback)) {
        other.m_targetHwnd = nullptr;
        other.m_targetCtrl = nullptr;
        other.m_pTarget = nullptr;
        other.m_registered = false;
        if (m_registered && m_targetHwnd) {
            g_controlCallbacks[m_targetHwnd] = m_callback;
        }
    }

    zhuziDropTarget& zhuziDropTarget::operator=(zhuziDropTarget&& other) noexcept {
        if (this != &other) {
            revoke();
            m_targetHwnd = other.m_targetHwnd;
            m_targetCtrl = other.m_targetCtrl;
            m_pTarget = other.m_pTarget;
            m_registered = other.m_registered;
            m_async = other.m_async;
            m_callback = std::move(other.m_callback);
            other.m_targetHwnd = nullptr;
            other.m_targetCtrl = nullptr;
            other.m_pTarget = nullptr;
            other.m_registered = false;
            if (m_registered && m_targetHwnd) {
                g_controlCallbacks[m_targetHwnd] = m_callback;
            }
        }
        return *this;
    }

    bool zhuziDropTarget::isValid() const { return m_registered; }

    void zhuziDropTarget::revoke() {
        if (m_registered && m_targetHwnd) {
            ::RevokeDragDrop(m_targetHwnd);
            m_registered = false;
            g_controlCallbacks.erase(m_targetHwnd);
        }
        if (m_pTarget) {
            static_cast<DropTargetImpl*>(m_pTarget)->Release();
            m_pTarget = nullptr;
        }
        m_targetHwnd = nullptr;
        m_targetCtrl = nullptr;
    }

    // ---------- IDropSource 实现 ----------
    class SimpleDropSource : public IDropSource {
    public:
        SimpleDropSource() : m_refCount(1) {}
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
            if (riid == IID_IUnknown || riid == IID_IDropSource) {
                *ppv = static_cast<IDropSource*>(this);
                AddRef();
                return S_OK;
            }
            *ppv = nullptr;
            return E_NOINTERFACE;
        }
        ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refCount); }
        ULONG STDMETHODCALLTYPE Release() override {
            LONG ref = InterlockedDecrement(&m_refCount);
            if (ref == 0) delete this;
            return ref;
        }
        HRESULT STDMETHODCALLTYPE QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState) override {
            if (fEscapePressed) return DRAGDROP_S_CANCEL;
            if (!(grfKeyState & MK_LBUTTON)) return DRAGDROP_S_DROP;
            return S_OK;
        }
        HRESULT STDMETHODCALLTYPE GiveFeedback(DWORD dwEffect) override {
            return DRAGDROP_S_USEDEFAULTCURSORS;
        }
    private:
        LONG m_refCount;
    };

    // ---------- 基础 IDataObject ----------
    class SimpleDataObject : public IDataObject {
    public:
        SimpleDataObject(const std::vector<zhuziString>& files, const zhuziString& text, const zhuziString& html)
            : m_refCount(1), m_files(files), m_text(text), m_html(html) {
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
            if (riid == IID_IUnknown || riid == IID_IDataObject) {
                *ppv = static_cast<IDataObject*>(this);
                AddRef();
                return S_OK;
            }
            *ppv = nullptr;
            return E_NOINTERFACE;
        }
        ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_refCount); }
        ULONG STDMETHODCALLTYPE Release() override {
            LONG ref = InterlockedDecrement(&m_refCount);
            if (ref == 0) delete this;
            return ref;
        }

        HRESULT STDMETHODCALLTYPE GetData(FORMATETC* pFormatEtc, STGMEDIUM* pMedium) override {
            if (!pFormatEtc || !pMedium) return E_INVALIDARG;
            pMedium->pUnkForRelease = nullptr;
            if (pFormatEtc->cfFormat == CF_HDROP && (pFormatEtc->tymed & TYMED_HGLOBAL) && !m_files.empty()) {
                pMedium->tymed = TYMED_HGLOBAL;
                pMedium->hGlobal = CreateFileDropHandle(m_files);
                return pMedium->hGlobal ? S_OK : E_OUTOFMEMORY;
            }
            else if (pFormatEtc->cfFormat == CF_UNICODETEXT && (pFormatEtc->tymed & TYMED_HGLOBAL) && !m_text.empty()) {
                pMedium->tymed = TYMED_HGLOBAL;
                pMedium->hGlobal = CreateUnicodeTextHandle(m_text);
                return pMedium->hGlobal ? S_OK : E_OUTOFMEMORY;
            }
            else if (pFormatEtc->cfFormat == CF_TEXT && (pFormatEtc->tymed & TYMED_HGLOBAL) && !m_text.empty()) {
                pMedium->tymed = TYMED_HGLOBAL;
                pMedium->hGlobal = CreateAnsiTextHandle(m_text);
                return pMedium->hGlobal ? S_OK : E_OUTOFMEMORY;
            }
            else if (pFormatEtc->cfFormat == RegisterClipboardFormatW(L"HTML Format") && (pFormatEtc->tymed & TYMED_HGLOBAL) && !m_html.empty()) {
                pMedium->tymed = TYMED_HGLOBAL;
                pMedium->hGlobal = CreateUnicodeTextHandle(m_html);
                return pMedium->hGlobal ? S_OK : E_OUTOFMEMORY;
            }
            return DV_E_FORMATETC;
        }

        HRESULT STDMETHODCALLTYPE GetDataHere(FORMATETC*, STGMEDIUM*) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC* pFormatEtc) override {
            if (!pFormatEtc) return E_INVALIDARG;
            if (pFormatEtc->cfFormat == CF_HDROP && (pFormatEtc->tymed & TYMED_HGLOBAL) && !m_files.empty())
                return S_OK;
            if (pFormatEtc->cfFormat == CF_UNICODETEXT && (pFormatEtc->tymed & TYMED_HGLOBAL) && !m_text.empty())
                return S_OK;
            if (pFormatEtc->cfFormat == CF_TEXT && (pFormatEtc->tymed & TYMED_HGLOBAL) && !m_text.empty())
                return S_OK;
            if (pFormatEtc->cfFormat == RegisterClipboardFormatW(L"HTML Format") && (pFormatEtc->tymed & TYMED_HGLOBAL) && !m_html.empty())
                return S_OK;
            return DV_E_FORMATETC;
        }
        HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc(FORMATETC*, FORMATETC*) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE SetData(FORMATETC*, STGMEDIUM*, BOOL) override { return E_NOTIMPL; }
        HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc) override {
            *ppenumFormatEtc = nullptr;
            return E_NOTIMPL;
        }
        HRESULT STDMETHODCALLTYPE DAdvise(FORMATETC*, DWORD, IAdviseSink*, DWORD*) override { return OLE_E_ADVISENOTSUPPORTED; }
        HRESULT STDMETHODCALLTYPE DUnadvise(DWORD) override { return OLE_E_ADVISENOTSUPPORTED; }
        HRESULT STDMETHODCALLTYPE EnumDAdvise(IEnumSTATDATA**) override { return OLE_E_ADVISENOTSUPPORTED; }

    protected:
        static HGLOBAL CreateFileDropHandle(const std::vector<zhuziString>& files) {
            size_t totalSize = sizeof(DROPFILES);
            for (const auto& f : files) totalSize += (f.length() + 1) * sizeof(wchar_t);
            totalSize += sizeof(wchar_t);
            HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, totalSize);
            if (!hGlobal) return nullptr;
            DROPFILES* pDrop = (DROPFILES*)GlobalLock(hGlobal);
            pDrop->pFiles = sizeof(DROPFILES);
            pDrop->fWide = TRUE;
            wchar_t* pFiles = (wchar_t*)((BYTE*)pDrop + sizeof(DROPFILES));
            for (const auto& f : files) {
                wcscpy_s(pFiles, f.length() + 1, f.c_str());
                pFiles += f.length() + 1;
            }
            *pFiles = L'\0';
            GlobalUnlock(hGlobal);
            return hGlobal;
        }
        static HGLOBAL CreateUnicodeTextHandle(const zhuziString& text) {
            if (text.empty()) return nullptr;
            size_t size = (text.length() + 1) * sizeof(wchar_t);
            HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
            if (!hGlobal) return nullptr;
            wchar_t* p = (wchar_t*)GlobalLock(hGlobal);
            wcscpy_s(p, text.length() + 1, text.c_str());
            GlobalUnlock(hGlobal);
            return hGlobal;
        }
        static HGLOBAL CreateAnsiTextHandle(const zhuziString& text) {
            if (text.empty()) return nullptr;
            int len = WideCharToMultiByte(CP_ACP, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
            if (len <= 0) return nullptr;
            HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, len);
            if (!hGlobal) return nullptr;
            char* p = (char*)GlobalLock(hGlobal);
            WideCharToMultiByte(CP_ACP, 0, text.c_str(), -1, p, len, nullptr, nullptr);
            GlobalUnlock(hGlobal);
            return hGlobal;
        }

        LONG m_refCount;
        std::vector<zhuziString> m_files;
        zhuziString m_text;
        zhuziString m_html;
    };

    // ---------- 增强版 IDataObject ----------
    class EnhancedDataObject : public SimpleDataObject {
    public:
        EnhancedDataObject(const std::vector<zhuziString>& files, const zhuziString& text, const zhuziString& html)
            : SimpleDataObject(files, text, html) {
        }

        HRESULT STDMETHODCALLTYPE EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc) override {
            if (dwDirection != DATADIR_GET) {
                *ppenumFormatEtc = nullptr;
                return E_NOTIMPL;
            }

            std::vector<FORMATETC> formats;
            if (!m_files.empty()) {
                FORMATETC fe = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
                formats.push_back(fe);
                fe.cfFormat = static_cast<CLIPFORMAT>(RegisterClipboardFormatW(CFSTR_FILEDESCRIPTORW));
                formats.push_back(fe);
            }
            if (!m_text.empty()) {
                FORMATETC fe = { CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
                formats.push_back(fe);
                fe.cfFormat = CF_TEXT;
                formats.push_back(fe);
            }
            if (!m_html.empty()) {
                FORMATETC fe = { static_cast<CLIPFORMAT>(RegisterClipboardFormatW(L"HTML Format")), nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
                formats.push_back(fe);
            }

            if (formats.empty()) {
                *ppenumFormatEtc = nullptr;
                return E_NOTIMPL;
            }

            for (auto& fe : formats) fe.ptd = nullptr;

            HRESULT hr = SHCreateStdEnumFmtEtc((UINT)formats.size(), formats.data(), ppenumFormatEtc);
            if (FAILED(hr)) {
                *ppenumFormatEtc = nullptr;
                return hr;
            }
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE GetData(FORMATETC* pFormatEtc, STGMEDIUM* pMedium) override {
            if (!pFormatEtc || !pMedium) return E_INVALIDARG;
            pMedium->pUnkForRelease = nullptr;

            if (pFormatEtc->cfFormat == CF_HDROP && (pFormatEtc->tymed & TYMED_HGLOBAL) && !m_files.empty()) {
                pMedium->tymed = TYMED_HGLOBAL;
                pMedium->hGlobal = CreateFileDropHandle(m_files);
                return pMedium->hGlobal ? S_OK : E_OUTOFMEMORY;
            }

            CLIPFORMAT cfFileDesc = static_cast<CLIPFORMAT>(RegisterClipboardFormatW(CFSTR_FILEDESCRIPTORW));
            if (pFormatEtc->cfFormat == cfFileDesc && (pFormatEtc->tymed & TYMED_HGLOBAL) && !m_files.empty()) {
                pMedium->tymed = TYMED_HGLOBAL;
                pMedium->hGlobal = CreateFileDescriptorHandle(m_files);
                return pMedium->hGlobal ? S_OK : E_OUTOFMEMORY;
            }

            return SimpleDataObject::GetData(pFormatEtc, pMedium);
        }

        HRESULT STDMETHODCALLTYPE QueryGetData(FORMATETC* pFormatEtc) override {
            if (!pFormatEtc) return E_INVALIDARG;
            if (pFormatEtc->cfFormat == CF_HDROP && (pFormatEtc->tymed & TYMED_HGLOBAL) && !m_files.empty())
                return S_OK;
            CLIPFORMAT cfFileDesc = static_cast<CLIPFORMAT>(RegisterClipboardFormatW(CFSTR_FILEDESCRIPTORW));
            if (pFormatEtc->cfFormat == cfFileDesc && (pFormatEtc->tymed & TYMED_HGLOBAL) && !m_files.empty())
                return S_OK;
            return SimpleDataObject::QueryGetData(pFormatEtc);
        }

    private:
        static HGLOBAL CreateFileDescriptorHandle(const std::vector<zhuziString>& files) {
            UINT count = (UINT)files.size();
            SIZE_T size = sizeof(FILEGROUPDESCRIPTOR) + (count - 1) * sizeof(FILEDESCRIPTOR);
            HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
            if (!hGlobal) return nullptr;
            FILEGROUPDESCRIPTOR* pfgd = (FILEGROUPDESCRIPTOR*)GlobalLock(hGlobal);
            pfgd->cItems = count;
            for (UINT i = 0; i < count; ++i) {
                FILEDESCRIPTOR& fd = pfgd->fgd[i];
                memset(&fd, 0, sizeof(FILEDESCRIPTOR));
                fd.dwFlags = FD_ATTRIBUTES | FD_FILESIZE;
                fd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
                WIN32_FILE_ATTRIBUTE_DATA attr;
                if (GetFileAttributesExW(files[i].c_str(), GetFileExInfoStandard, &attr)) {
                    fd.nFileSizeLow = attr.nFileSizeLow;
                    fd.nFileSizeHigh = attr.nFileSizeHigh;
                    fd.dwFileAttributes = attr.dwFileAttributes;
                }
                wcscpy_s(fd.cFileName, MAX_PATH, files[i].c_str());
            }
            GlobalUnlock(hGlobal);
            return hGlobal;
        }
    };

    // ---------- DragSourceData 实现 ----------
    DragSourceData& DragSourceData::addFile(const zhuziString& path) {
        m_files.push_back(path);
        return *this;
    }
    DragSourceData& DragSourceData::addFiles(const std::vector<zhuziString>& paths) {
        m_files.insert(m_files.end(), paths.begin(), paths.end());
        return *this;
    }
    DragSourceData& DragSourceData::setText(const zhuziString& text) {
        m_text = text;
        return *this;
    }
    DragSourceData& DragSourceData::setHtml(const zhuziString& html) {
        m_html = html;
        return *this;
    }
    IDataObject* DragSourceData::createDataObject() const {
        return new SimpleDataObject(m_files, m_text, m_html);
    }

    // ---------- 全局拖出函数 ----------
    DWORD DoDrag(const DragSourceData& data, DWORD allowedEffects) {
        IDataObject* pDataObj = data.createDataObject();
        SimpleDropSource* pDropSource = new SimpleDropSource();
        DWORD dwEffect = DROPEFFECT_NONE;
        HRESULT hr = ::DoDragDrop(pDataObj, pDropSource, allowedEffects, &dwEffect);
        pDataObj->Release();
        pDropSource->Release();
        return SUCCEEDED(hr) ? dwEffect : 0;
    }

    DWORD DoDragFiles(const std::vector<zhuziString>& files, DWORD allowedEffects) {
        DragSourceData data;
        data.addFiles(files);
        return DoDrag(data, allowedEffects);
    }

    DWORD DoDragText(const zhuziString& text, DWORD allowedEffects) {
        DragSourceData data;
        data.setText(text);
        return DoDrag(data, allowedEffects);
    }

    // ---------- zhuziDragSource 实现 ----------
    zhuziDragSource::zhuziDragSource(zhuziControl* sourceCtrl,
        std::function<DragSourceData()> dataProvider,
        DWORD allowedEffects,
        bool showDragImage)
        : m_hwnd(nullptr), m_subclassed(false), m_draggingPossible(false), m_dragging(false),
        m_dataProvider(dataProvider), m_allowedEffects(allowedEffects), m_showDragImage(showDragImage) {
        if (!sourceCtrl || !dataProvider) return;
        m_hwnd = sourceCtrl->getHandle();
        if (!m_hwnd) return;
        if (!SetWindowSubclass(m_hwnd, SubclassProc, 0, reinterpret_cast<DWORD_PTR>(this))) return;
        m_subclassed = true;
    }

    zhuziDragSource::~zhuziDragSource() { revoke(); }

    zhuziDragSource::zhuziDragSource(zhuziDragSource&& other) noexcept
        : m_hwnd(other.m_hwnd), m_subclassed(other.m_subclassed), m_ptDown(other.m_ptDown),
        m_draggingPossible(other.m_draggingPossible), m_dragging(other.m_dragging),
        m_dataProvider(std::move(other.m_dataProvider)), m_allowedEffects(other.m_allowedEffects),
        m_showDragImage(other.m_showDragImage) {
        other.m_hwnd = nullptr;
        other.m_subclassed = false;
        if (m_subclassed) {
            SetWindowSubclass(m_hwnd, SubclassProc, 0, reinterpret_cast<DWORD_PTR>(this));
        }
    }

    zhuziDragSource& zhuziDragSource::operator=(zhuziDragSource&& other) noexcept {
        if (this != &other) {
            revoke();
            m_hwnd = other.m_hwnd;
            m_subclassed = other.m_subclassed;
            m_ptDown = other.m_ptDown;
            m_draggingPossible = other.m_draggingPossible;
            m_dragging = other.m_dragging;
            m_dataProvider = std::move(other.m_dataProvider);
            m_allowedEffects = other.m_allowedEffects;
            m_showDragImage = other.m_showDragImage;
            other.m_hwnd = nullptr;
            other.m_subclassed = false;
            if (m_subclassed) {
                SetWindowSubclass(m_hwnd, SubclassProc, 0, reinterpret_cast<DWORD_PTR>(this));
            }
        }
        return *this;
    }

    void zhuziDragSource::revoke() {
        if (m_subclassed && m_hwnd) {
            RemoveWindowSubclass(m_hwnd, SubclassProc, 0);
            m_subclassed = false;
        }
        m_hwnd = nullptr;
    }

    LRESULT CALLBACK zhuziDragSource::SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
        UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
        zhuziDragSource* pThis = reinterpret_cast<zhuziDragSource*>(dwRefData);
        if (pThis && pThis->m_hwnd == hwnd) {
            switch (msg) {
            case WM_LBUTTONDOWN: pThis->onLButtonDown(lParam); break;
            case WM_MOUSEMOVE:   pThis->onMouseMove(lParam); break;
            case WM_LBUTTONUP:   pThis->onLButtonUp(); break;
            }
        }
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }

    void zhuziDragSource::onLButtonDown(LPARAM lParam) {
        m_ptDown.x = GET_X_LPARAM(lParam);
        m_ptDown.y = GET_Y_LPARAM(lParam);
        m_draggingPossible = true;
        m_dragging = false;
        SetCapture(m_hwnd);
    }

    void zhuziDragSource::onMouseMove(LPARAM lParam) {
        if (!m_draggingPossible || m_dragging) return;
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        if (!checkThreshold(x, y)) return;

        m_dragging = true;
        m_draggingPossible = false;
        ReleaseCapture();

        if (m_dataProvider) {
            DragSourceData data = m_dataProvider();
            if (data.getFiles().empty() && data.getText().empty() && data.getHtml().empty()) {
                m_dragging = false;
                return;
            }

            IDataObject* pDataObj = nullptr;
            if (m_showDragImage) {
                pDataObj = new EnhancedDataObject(data.getFiles(), data.getText(), data.getHtml());
            }
            else {
                pDataObj = data.createDataObject();
            }
            SimpleDropSource* pDropSource = new SimpleDropSource();
            DWORD dwEffect = DROPEFFECT_NONE;
            HRESULT hr = ::DoDragDrop(pDataObj, pDropSource, m_allowedEffects, &dwEffect);
            pDataObj->Release();
            pDropSource->Release();
        }
        m_dragging = false;
    }

    void zhuziDragSource::onLButtonUp() {
        m_draggingPossible = false;
        m_dragging = false;
        ReleaseCapture();
    }

    bool zhuziDragSource::checkThreshold(int x, int y) const {
        int dx = abs(x - m_ptDown.x);
        int dy = abs(y - m_ptDown.y);
        int dragX = GetSystemMetrics(SM_CXDRAG);
        int dragY = GetSystemMetrics(SM_CYDRAG);
        return (dx >= dragX || dy >= dragY);
    }

} // namespace zhuzi