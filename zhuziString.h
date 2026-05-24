#pragma once
#include <cstddef>

namespace zhuzi {

    class zhuziString {
    public:
        // 构造函数
        zhuziString();
        zhuziString(const char* str);
        zhuziString(const wchar_t* str);
        zhuziString(const zhuziString& other);
        zhuziString(zhuziString&& other) noexcept;
        zhuziString(size_t count, wchar_t ch);      // 重复字符构造
        ~zhuziString();

        // 赋值
        zhuziString& operator=(const char* str);
        zhuziString& operator=(const wchar_t* str);
        zhuziString& operator=(const zhuziString& other);
        zhuziString& operator=(zhuziString&& other) noexcept;

        // 容量
        size_t size() const;
        size_t length() const;
        bool empty() const;
        void resize(size_t newSize, wchar_t ch = L'\0');
        void reserve(size_t newCapacity);

        // 元素访问
        wchar_t operator[](size_t index) const;
        wchar_t& operator[](size_t index);
        const wchar_t* c_str() const;
        const wchar_t* data() const;

        // 修改
        void clear();
        void push_back(wchar_t ch);

        // 连接
        zhuziString& operator+=(const zhuziString& other);
        zhuziString& operator+=(const wchar_t* str);
        zhuziString& operator+=(wchar_t ch);

        // 比较
        bool operator==(const zhuziString& other) const;
        bool operator!=(const zhuziString& other) const;

        // 转换（UTF-8）
        const char* c_charptr() const;   // 调用者必须 delete[]

        // 友元全局 operator+
        friend zhuziString operator+(const zhuziString& lhs, const zhuziString& rhs);
        friend zhuziString operator+(const zhuziString& lhs, const wchar_t* rhs);
        friend zhuziString operator+(const wchar_t* lhs, const zhuziString& rhs);
        friend zhuziString operator+(const zhuziString& lhs, wchar_t rhs);
        friend zhuziString operator+(wchar_t lhs, const zhuziString& rhs);

        // 在 class zhuziString 内部添加以下公有成员函数：
        bool operator<(const zhuziString& other) const;
        bool operator>(const zhuziString& other) const;
        bool operator<=(const zhuziString& other) const;
        bool operator>=(const zhuziString& other) const;

    private:
        wchar_t* m_data;
        size_t   m_len;
        size_t   m_capacity;

        void allocate(size_t capacity);
        void release();
        void copyFrom(const wchar_t* src, size_t len);
        void moveFrom(zhuziString&& other);
        void ensureCapacity(size_t newLen);
    };

    // 全局 operator+ 声明
    zhuziString operator+(const zhuziString& lhs, const zhuziString& rhs);
    zhuziString operator+(const zhuziString& lhs, const wchar_t* rhs);
    zhuziString operator+(const wchar_t* lhs, const zhuziString& rhs);
    zhuziString operator+(const zhuziString& lhs, wchar_t rhs);
    zhuziString operator+(wchar_t lhs, const zhuziString& rhs);

} // namespace zhuzi