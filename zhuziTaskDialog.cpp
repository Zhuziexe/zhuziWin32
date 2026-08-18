#include "zhuziTaskDialog.h"
namespace zhuzi {
	TDResult TaskDialog(zhuziControl* parent, zhuziInstance* instoficon, const zhuziString windowtitle,
		const zhuziString mainInstruction, const zhuziString dlgcontent,
		TASKDIALOG_COMMON_BUTTON_FLAGS tdbtnflags, PCWSTR iconrc, TDFlags fl) {
		int* a = new int(0);
		TDResult tdr{
			::TaskDialog(
				parent->getHandle(), instoficon->getHandle(), 
				(fl.NoWindowTitle?NULL:windowtitle.c_str()),
				(fl.NoMainInstruction?NULL:mainInstruction.c_str()), 
				(fl.NoDlgContent?NULL:dlgcontent.c_str()),
				tdbtnflags, iconrc, a),
			*a
		};
		delete a;
		return tdr;
	}
}