#include "zhuziCommDlg.h"
#include <shlobj.h>     // SHBrowseForFolder, IFileDialog
#include <commdlg.h>    // ChooseColor, ChooseFont
#include <objbase.h>
#include <vector>
#include <string>
#include <memory>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

namespace zhuzi {

    // ---------- 文件打开（单文件） ----------
    bool FileOpenDialog(HWND hwndOwner, const std::vector<FileFilter>& filters,
        zhuziString& outPath, const zhuziString& defaultExtension) {
        IFileOpenDialog* pFileOpen = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
        if (FAILED(hr)) return false;

        DWORD dwFlags;
        pFileOpen->GetOptions(&dwFlags);
        dwFlags |= FOS_FORCEFILESYSTEM;
        dwFlags &= ~FOS_PICKFOLDERS;
        pFileOpen->SetOptions(dwFlags);

        if (!filters.empty()) {
            std::vector<COMDLG_FILTERSPEC> specs(filters.size());
            for (size_t i = 0; i < filters.size(); ++i) {
                specs[i].pszName = filters[i].name.c_str();
                specs[i].pszSpec = filters[i].pattern.c_str();
            }
            pFileOpen->SetFileTypes((UINT)specs.size(), specs.data());
            pFileOpen->SetFileTypeIndex(1);
        }

        if (!defaultExtension.empty()) {
            pFileOpen->SetDefaultExtension(defaultExtension.c_str());
        }

        hr = pFileOpen->Show(hwndOwner);
        if (SUCCEEDED(hr)) {
            IShellItem* pItem = nullptr;
            hr = pFileOpen->GetResult(&pItem);
            if (SUCCEEDED(hr)) {
                wchar_t* pPath = nullptr;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pPath);
                if (SUCCEEDED(hr)) {
                    outPath = pPath;
                    CoTaskMemFree(pPath);
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
        return SUCCEEDED(hr);
    }

    // ---------- 文件打开（多文件） ----------
    bool FileOpenMultiDialog(HWND hwndOwner, const std::vector<FileFilter>& filters,
        std::vector<zhuziString>& outPaths, const zhuziString& defaultExtension) {
        IFileOpenDialog* pFileOpen = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
        if (FAILED(hr)) return false;

        DWORD dwFlags;
        pFileOpen->GetOptions(&dwFlags);
        dwFlags |= FOS_FORCEFILESYSTEM | FOS_ALLOWMULTISELECT;
        dwFlags &= ~FOS_PICKFOLDERS;
        pFileOpen->SetOptions(dwFlags);

        if (!filters.empty()) {
            std::vector<COMDLG_FILTERSPEC> specs(filters.size());
            for (size_t i = 0; i < filters.size(); ++i) {
                specs[i].pszName = filters[i].name.c_str();
                specs[i].pszSpec = filters[i].pattern.c_str();
            }
            pFileOpen->SetFileTypes((UINT)specs.size(), specs.data());
            pFileOpen->SetFileTypeIndex(1);
        }

        if (!defaultExtension.empty()) {
            pFileOpen->SetDefaultExtension(defaultExtension.c_str());
        }

        hr = pFileOpen->Show(hwndOwner);
        if (SUCCEEDED(hr)) {
            IShellItemArray* pItemArray = nullptr;
            hr = pFileOpen->GetResults(&pItemArray);
            if (SUCCEEDED(hr)) {
                DWORD count = 0;
                pItemArray->GetCount(&count);
                outPaths.clear();
                outPaths.reserve(count);
                for (DWORD i = 0; i < count; ++i) {
                    IShellItem* pItem = nullptr;
                    hr = pItemArray->GetItemAt(i, &pItem);
                    if (SUCCEEDED(hr)) {
                        wchar_t* pPath = nullptr;
                        if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pPath))) {
                            outPaths.emplace_back(pPath);
                            CoTaskMemFree(pPath);
                        }
                        pItem->Release();
                    }
                }
                pItemArray->Release();
            }
        }
        pFileOpen->Release();
        return SUCCEEDED(hr) && !outPaths.empty();
    }

    // ---------- 文件保存 ----------
    bool FileSaveDialog(HWND hwndOwner, const std::vector<FileFilter>& filters,
        zhuziString& outPath, const zhuziString& defaultExtension,
        const zhuziString& defaultFileName) {
        IFileSaveDialog* pFileSave = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL,
            IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));
        if (FAILED(hr)) return false;

        if (!defaultFileName.empty()) {
            pFileSave->SetFileName(defaultFileName.c_str());
        }

        if (!filters.empty()) {
            std::vector<COMDLG_FILTERSPEC> specs(filters.size());
            for (size_t i = 0; i < filters.size(); ++i) {
                specs[i].pszName = filters[i].name.c_str();
                specs[i].pszSpec = filters[i].pattern.c_str();
            }
            pFileSave->SetFileTypes((UINT)specs.size(), specs.data());
            pFileSave->SetFileTypeIndex(1);
        }

        if (!defaultExtension.empty()) {
            pFileSave->SetDefaultExtension(defaultExtension.c_str());
        }

        hr = pFileSave->Show(hwndOwner);
        if (SUCCEEDED(hr)) {
            IShellItem* pItem = nullptr;
            hr = pFileSave->GetResult(&pItem);
            if (SUCCEEDED(hr)) {
                wchar_t* pPath = nullptr;
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pPath);
                if (SUCCEEDED(hr)) {
                    outPath = pPath;
                    CoTaskMemFree(pPath);
                }
                pItem->Release();
            }
        }
        pFileSave->Release();
        return SUCCEEDED(hr);
    }

    // ---------- 颜色选择（使用 zhuziColor，不再需要外部 customColors 数组）----------
    bool ColorDialog(HWND hwndOwner, zhuziColor& color) {
        static COLORREF customColors[16] = { 0 }; // 静态保留自定义颜色
        CHOOSECOLORW cc = { 0 };
        cc.lStructSize = sizeof(cc);
        cc.hwndOwner = hwndOwner;
        cc.rgbResult = color.toCOLORREF();
        cc.lpCustColors = customColors;
        cc.Flags = CC_RGBINIT | CC_FULLOPEN;
        if (ChooseColorW(&cc)) {
            color = zhuziColor(cc.rgbResult);
            return true;
        }
        return false;
    }

    // ---------- 字体选择 ----------
    bool FontDialog(HWND hwndOwner, zhuziFont& font) {
        LOGFONTW lf = { 0 };
        HFONT hFont = font.getHandle();
        if (hFont) {
            GetObjectW(hFont, sizeof(LOGFONTW), &lf);
        }
        else {
            wcscpy_s(lf.lfFaceName, L"Microsoft YaHei");
            HDC hdc = GetDC(nullptr);
            int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(nullptr, hdc);
            lf.lfHeight = -MulDiv(9, dpiY, 72);
            lf.lfWeight = FW_NORMAL;
            lf.lfCharSet = DEFAULT_CHARSET;
            lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
            lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
            lf.lfQuality = DEFAULT_QUALITY;
            lf.lfPitchAndFamily = DEFAULT_PITCH;
        }

        CHOOSEFONTW cf = { 0 };
        cf.lStructSize = sizeof(cf);
        cf.hwndOwner = hwndOwner;
        cf.lpLogFont = &lf;
        cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_EFFECTS;
        if (ChooseFontW(&cf)) {
            HDC hdc = GetDC(nullptr);
            int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
            ReleaseDC(nullptr, hdc);
            int pointSize = MulDiv(-lf.lfHeight, 72, dpiY);
            if (pointSize <= 0) pointSize = 9;
            bool bold = (lf.lfWeight >= FW_BOLD);
            bool italic = (lf.lfItalic != 0);
            bool underline = (lf.lfUnderline != 0);
            font = zhuziFont(lf.lfFaceName, pointSize, bold, italic, underline);
            return true;
        }
        return false;
    }

    // ---------- 文件夹浏览 ----------
    static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
        if (uMsg == BFFM_INITIALIZED && lpData) {
            SendMessageW(hwnd, BFFM_SETSELECTIONW, TRUE, lpData);
        }
        return 0;
    }

    bool BrowseFolderDialog(HWND hwndOwner, zhuziString& outPath,
        const zhuziString& title, const zhuziString& initialDir) {
        BROWSEINFOW bi = { 0 };
        bi.hwndOwner = hwndOwner;
        bi.lpszTitle = title.c_str();
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        if (!initialDir.empty()) {
            bi.lpfn = BrowseCallbackProc;
            bi.lParam = (LPARAM)initialDir.c_str();
        }

        LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
        if (pidl) {
            wchar_t path[MAX_PATH];
            if (SHGetPathFromIDListW(pidl, path)) {
                outPath = path;
                CoTaskMemFree(pidl);
                return true;
            }
            CoTaskMemFree(pidl);
        }
        return false;
    }

} // namespace zhuzi