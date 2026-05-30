#pragma once
#include <windows.h>
#include <vector>
#include "zhuziString.h"
#include "zhuziFont.h"

namespace zhuzi {

    struct FileFilter {
        zhuziString name;    // 显示名称，例如 "文本文件 (*.txt)"
        zhuziString pattern; // 模式，例如 "*.txt" 或 "*.jpg;*.png;*.bmp"
    };

    // 文件打开（单文件）
    bool FileOpenDialog(HWND hwndOwner,
        const std::vector<FileFilter>& filters,
        zhuziString& outPath,
        const zhuziString& defaultExtension = L"");

    // 文件打开（多文件）
    bool FileOpenMultiDialog(HWND hwndOwner,
        const std::vector<FileFilter>& filters,
        std::vector<zhuziString>& outPaths,
        const zhuziString& defaultExtension = L"");

    // 文件保存
    bool FileSaveDialog(HWND hwndOwner,
        const std::vector<FileFilter>& filters,
        zhuziString& outPath,
        const zhuziString& defaultExtension = L"",
        const zhuziString& defaultFileName = L"");

    // 颜色选择
    bool ColorDialog(HWND hwndOwner, COLORREF& color, COLORREF customColors[16] = nullptr);

    // 字体选择
    bool FontDialog(HWND hwndOwner, zhuziFont& font);

    // 文件夹浏览
    bool BrowseFolderDialog(HWND hwndOwner,
        zhuziString& outPath,
        const zhuziString& title = L"请选择文件夹",
        const zhuziString& initialDir = L"");

} // namespace zhuzi