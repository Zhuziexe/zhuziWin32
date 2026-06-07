#ifndef ZHUZIWIN32_H
#define ZHUZIWIN32_H

#ifndef NO_COMCTL_60
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif // !NO_COMCTL_60

#ifndef NO_ZHUZI_INCLUDES
// 基础头文件包含
#include "zhuziFont.h"
#include "zhuziInstance.h"
#include "zhuziPaint.h"
#include "zhuziString.h"
#include "zhuziImageList.h"
#include "zhuziDragDrop.h"

#ifndef NO_ZHUZICONTROLS_INCLUDES

#include "zhuziControl.h"
#include "zhuziCommctrl.h"
#include "zhuziDTP.h"
#include "zhuziDialog.h"
#include "zhuziEdit.h"
#include "zhuziGroupBox.h"
#include "zhuziImageLabel.h"
#include "zhuziListView.h"
#include "zhuziMenu.h"
#include "zhuziPanedWindow.h"
#include "zhuziProgressBar.h"
#include "zhuziRebar.h"
#include "zhuziScrollArea.h"
#include "zhuziStatusBar.h"
#include "zhuziTab.h"
#include "zhuziToolBar.h"
#include "zhuziToolTip.h"
#include "zhuziTracker.h"
#include "zhuziTreeView.h"
#include "zhuziUpDownEdit.h"
#include "zhuziComboBoxEx.h"
#include "zhuziCommDlg.h"
#include "CmEdit.h"
#include "zhuziImageButton.h"

#endif // !NO_ZHUZICONTROLS_INCLUDES

#endif // !NO_ZHUZI_INCLUDES
#endif // !ZHUZIWIN32_H
