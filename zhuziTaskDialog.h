#include "zhuziControl.h"
#include "zhuziInstance.h"
#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib,"comctl32.lib")

namespace zhuzi {
	struct TDResult {
		int success;
		int clickedbtn;
	};
	struct TDFlags {
		unsigned char NoWindowTitle : 1;
		unsigned char NoMainInstruction : 1;
		unsigned char NoDlgContent : 1;
	};
}

#ifndef NO_COMCTL_60
namespace zhuzi {
	TDResult TaskDialog(zhuziControl* parent, zhuziInstance* instoficon, const zhuziString windowtitle,
		const zhuziString mainInstruction, const zhuziString dlgcontent,
		TASKDIALOG_COMMON_BUTTON_FLAGS tdbtnflags, PCWSTR iconrc, TDFlags fl = {0,0,0});
}
#endif