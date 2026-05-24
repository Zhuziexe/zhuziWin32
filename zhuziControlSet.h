#include "zhuziControl.h"
namespace zhuzi {
	class zhuziControlGen {
	public:
		zhuziControlGen() {}
		~zhuziControlGen() {}
		void genFunc(int count,std::function<void(int x,int y,
			int width,int height,int index)> _gen) 
		{
			int __x = startx;
			int __y = starty;
			for (int i = 0; i < count; i++) {
				_gen(__x, __y, width, height, i);
				__x += xspace; __y += yspace;
			}
		}
		int xspace = 0;
		int yspace = 40;
		uint16_t width = 100;
		uint16_t height = 30;
		uint16_t startx = 0;
		uint16_t starty = 0;
	private:
	};
}