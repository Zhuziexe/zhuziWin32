#pragma once
#include <windows.h>

namespace zhuzi {

    class zhuziRgn {
    public:
        zhuziRgn() : m_hRgn(nullptr) {}
        explicit zhuziRgn(HRGN hRgn) : m_hRgn(hRgn) {}
        ~zhuziRgn() { destroy(); }

        // 禁止拷贝
        zhuziRgn(const zhuziRgn&) = delete;
        zhuziRgn& operator=(const zhuziRgn&) = delete;

        // 移动语义
        zhuziRgn(zhuziRgn&& other) noexcept : m_hRgn(other.m_hRgn) {
            other.m_hRgn = nullptr;
        }
        zhuziRgn& operator=(zhuziRgn&& other) noexcept {
            if (this != &other) {
                destroy();
                m_hRgn = other.m_hRgn;
                other.m_hRgn = nullptr;
            }
            return *this;
        }

        // 释放所有权（转移给调用者，不再自动释放）
        HRGN release() {
            HRGN h = m_hRgn;
            m_hRgn = nullptr;
            return h;
        }

        // 创建矩形区域
        bool createRectRgn(int left, int top, int right, int bottom) {
            destroy();
            m_hRgn = CreateRectRgn(left, top, right, bottom);
            return m_hRgn != nullptr;
        }

        // 创建圆角矩形区域
        bool createRoundRectRgn(int left, int top, int right, int bottom, int widthEllipse, int heightEllipse) {
            destroy();
            m_hRgn = CreateRoundRectRgn(left, top, right, bottom, widthEllipse, heightEllipse);
            return m_hRgn != nullptr;
        }

        // 创建椭圆/圆形区域
        bool createEllipticRgn(int left, int top, int right, int bottom) {
            destroy();
            m_hRgn = CreateEllipticRgn(left, top, right, bottom);
            return m_hRgn != nullptr;
        }

        // 创建多边形区域
        bool createPolygonRgn(const POINT* points, int count, int mode = ALTERNATE) {
            destroy();
            m_hRgn = CreatePolygonRgn(points, count, mode);
            return m_hRgn != nullptr;
        }

        // 组合区域（合并、相交等）
        bool combineRgn(const zhuziRgn& rgn1, const zhuziRgn& rgn2, int mode) {
            destroy();
            m_hRgn = CreateRectRgn(0, 0, 0, 0); // 临时区域
            int res = CombineRgn(m_hRgn, rgn1.get(), rgn2.get(), mode);
            if (res == ERROR) {
                destroy();
                return false;
            }
            return true;
        }

        HRGN get() const { return m_hRgn; }
        bool isValid() const { return m_hRgn != nullptr; }

    private:
        HRGN m_hRgn;
        void destroy() {
            if (m_hRgn) DeleteObject(m_hRgn);
            m_hRgn = nullptr;
        }
    };

} // namespace zhuzi