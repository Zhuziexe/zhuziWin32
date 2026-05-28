# zhuziWin32 - Lightweight Windows C++ GUI Library

`zhuziWin32` is a lightweight C++ GUI framework based on native Win32
API and GDI+, designed to simplify the development of Windows desktop
applications. It provides a rich set of controls, event callbacks,
layout management, and graphics drawing capabilities while remaining
small and efficient.

## Features

-   **Pure native implementation**: Directly uses Win32 API, no external
    runtime dependencies (except GDI+ and common controls)
-   **Modern C++ style**: RAII, move semantics, `std::function`
    callbacks, iterators, etc.
-   **Rich control set**: Button, label, edit box, list box, combo box,
    tree view, list view, toolbar, status bar, tab control, progress
    bar, scroll area, paned window, etc.
-   **Flexible layout**: Supports absolute positioning, percentage
    positioning, anchor layout, automatic response to parent window
    resize
-   **Image support**: Manage icons/bitmaps via `zhuziImageList` for
    tree views, list views, toolbars, etc.
-   **Graphics drawing**: Based on GDI+, provides `zhuziPaint` drawing
    context with anti-aliasing, text rendering, basic shapes, and custom
    drawing
-   **Event system**: Control-level events (click, selection change,
    double-click, etc.), window-level message binding and chaining
-   **Resource management**: Automatic ID allocation, `zhuziString`
    class (wide-character storage, convenient conversion),
    `zhuziBinaryTree` container
-   **Thread safety**: Provides `zhuziMutex` and RAII guard

## Environment Variable Setup (Optional but Recommended)

To help compilers (e.g., Visual Studio, MinGW, CMake) locate
`zhuziWin32` headers and libraries, it is recommended to add the root
directory of `zhuziWin32` to the system\'s `ZHUZI_ROOT` environment
variable. You can also directly add the directory to `PATH` (for
command-line access).

## Quick Start

### Requirements

-   Windows SDK (supports Windows Vista and later)
-   Compiler: Visual Studio 2017+ (recommended) or MinGW-w64 (supports
    C++17)
-   Link libraries: `gdiplus.lib`, `comctl32.lib`, `zhuziWin32.lib`

### Compiling with Visual Studio (Recommended)

1.  Create a new Windows Desktop Application project (or an empty
    project).
2.  Add the `zhuziWin32` header directory(usually at ...\zhuziWin32\include\) to **Additional Include
    Directories**.
3.  Add the directory containing `zhuziWin32.lib(usually at ...\zhuziWin32\lib\)` to **Additional
    Library Directories**.
4.  Include the following `#pragma` directive in your
    source code to let the compiler auto-link the library(after setting **Additional Include Directories** and **Additional Library Directories**):

``` cpp
#pragma comment(lib, "zhuziWin32.lib") // Link zhuziWin32.lib
```

5.  Build and run.

### Compiling with `cl.exe` Command Line (without Visual Studio IDE)

If you have Visual Studio Build Tools or Visual Studio installed and
opened a **Developer Command Prompt**, you can compile with:

``` cmd
cl /EHsc /I"your_zhuziWin32_path" main.cpp /link gdiplus.lib comctl32.lib zhuziWin32.lib
```

Or, if your source already contains `#pragma comment(lib, ...)`, you
only need to link system and third-party libraries:

``` cmd
cl /EHsc /I"your_zhuziWin32_path" main.cpp /link gdiplus.lib comctl32.lib
```

### Example Program

Create a `main.cpp` with the following code:

``` cpp
#include "zhuziWin32.h"
#pragma comment(lib, "zhuziWin32.lib")

using namespace zhuzi;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    zhuziInstance inst(hInstance);
    zhuziWindow wnd;
    wnd.create(L"zhuziWin32 Sample", 100, 100, 400, 400);

    zhuziButton btn(&wnd);
    btn.create(10U, 10U, 10U, 10U);   // Anchor layout: 10px from left, top, right, bottom edges
    btn.setText(L"This is a zhuziButton");
    btn.setOnClick([&]() {
        MessageBoxW(wnd.getHandle(), L"Button Clicked!", L"Title", MB_ICONINFORMATION);
    });

    return inst.run(wnd);
}
```

Compile and run (using Visual Studio Developer Command Prompt):

``` cmd
cl /EHsc /I"path\to\zhuziWin32" main.cpp /link gdiplus.lib comctl32.lib
```

**Note:** The `create(10U, 10U, 10U, 10U)` in the example uses anchor
layout (the four parameters represent distances to the left, top, right,
bottom edges of the parent window). The button will automatically resize
and reposition when the parent window is resized. You can also use
absolute coordinates: `create(10, 10, 100, 30)`.

## Core Class Descriptions

### `zhuziControl`

Base class for all controls, providing:

-   Three layout methods: `create(x, y, w, h)` absolute;
    `create(x%, y%, w%, h%)` percentage;
    `create(toLeft, toTop, toRight, toBottom)` anchor (distance from
    parent edges)
-   Lifecycle: `onCreate` (subclasses create the actual window),
    `destroy`
-   Common properties: `setText`, `setFont`, `setRect`, `show`, `enable`
-   Virtual event methods: `onPaint`, `onLButtonUp`, `onMouseMove`, etc.
-   Automatic ID management: `setId`, `getId` (child control IDs
    automatically assigned)

### `zhuziWindow`

Top-level window class, inherits from `zhuziControl`, additionally
provides:

-   Background color `setBgColor`
-   Window icon `setIcon`
-   Min/Max size restrictions
-   Custom region clipping `setWindowRgn`
-   Message binding `Bind` (plain messages and wParam-filtered messages)
-   Message chaining `BindChain` (multiple callbacks can be processed in
    order; returning `true` stops the chain)

### Container Controls

-   `zhuziFrame`: Simple container, supports border styles and
    background color
-   `zhuziPanedWindow`: Multi-pane resizable container with draggable
    splitter, supports horizontal and vertical orientation
-   `zhuziRebar`: Windows Rebar control wrapper, can contain multiple
    bands
-   `zhuziScrollArea`: Custom area with scrollbars

### Data and Utilities

-   `zhuziString`: Wide string class with UTF-8 conversion (`c_charptr`)
    and common operators.
-   `zhuziBinaryTree<T>`: Binary search tree template with iterators,
    insert/erase/find, manual building support.
-   `zhuziMutex` / `zhuziMutexGuard`: Critical-section-based mutex lock.
-   `zhuziFont`, `zhuziColor`, `zhuziPen`, `zhuziBrush`, `zhuziPaint`:
    GDI+ drawing helpers.

## Building `zhuziWin32` Static Library

If you are building `zhuziWin32.lib` from source, you can use the
following methods:

### Using Visual Studio Project

1.  Create a **Static Library** project.
2.  Add all `.cpp` files (e.g., `zhuziControl.cpp`, `zhuziString.cpp`,
    etc.).
3.  Configure preprocessor definitions (optional).
4.  Build to generate `zhuziWin32.lib`.

### Using `cl` Command Line

``` cmd
cl /c /EHsc /I. *.cpp
lib /out:zhuziWin32.lib *.obj
```

Then you can link `zhuziWin32.lib` in other projects.

## Important Notes

-   All controls must be created before `zhuziInstance::run`, and the
    `zhuziInstance` object must remain alive during the message loop.
-   For custom drawing, override `onPaint(zhuziPaint& paint)` and use
    the provided paint object.
-   When using `zhuziImageList`, pay attention to the lifecycle; ensure
    the image list remains valid while the control uses it.
-   To use common controls version 6.0 (visual styles), make sure
    `NO_COMCTL_60` is not defined (enabled by default).
-   If you do not need certain control modules, you can define
    `NO_ZHUZICONTROLS_INCLUDES` to reduce header dependencies.

------------------------------------------------------------------------

**zhuziWin32 --- Making Win32 GUI programming simpler and more modern.**
