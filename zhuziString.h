#pragma once
#include <cstddef>
#include <string>

namespace zhuzi {

    class zhuziString {
    public:
        // 构造函数：只支持宽字符或空
        zhuziString();
        zhuziString(const wchar_t* str);
        zhuziString(const zhuziString& other);
        zhuziString(zhuziString&& other) noexcept;
        zhuziString(size_t count, wchar_t ch);
        ~zhuziString();

        // 显式从 UTF-8 / ACP 构造
        static zhuziString FromUTF8(const char* utf8);
        static zhuziString FromUTF8(const std::string& utf8);
        static zhuziString FromACP(const char* ansi);
        static zhuziString FromACP(const std::string& ansi);

        // 赋值：只支持宽字符或自身
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

        // ===== 新增：安全 UTF-8 转换（写入调用者提供的缓冲区） =====
        void to_utf8(char* buffer, size_t bufferSize) const;

        // ===== 标记为废弃（建议改用 to_utf8） =====
        [[deprecated("Use to_utf8() instead; this function allocates and must be delete[]")]]
        const char* c_charptr() const;

        // 修改
        void clear();
        void push_back(wchar_t ch);
        zhuziString& operator+=(const zhuziString& other);
        zhuziString& operator+=(const wchar_t* str);
        zhuziString& operator+=(wchar_t ch);

        // 子串和查找
        static const size_t npos = static_cast<size_t>(-1);
        zhuziString substr(size_t pos = 0, size_t count = npos) const;
        size_t find(const zhuziString& str, size_t pos = 0) const;
        size_t find(wchar_t ch, size_t pos = 0) const;
        size_t find(const wchar_t* str, size_t pos = 0) const;

        // 比较
        bool operator==(const zhuziString& other) const;
        bool operator!=(const zhuziString& other) const;
        bool operator<(const zhuziString& other) const;
        bool operator>(const zhuziString& other) const;
        bool operator<=(const zhuziString& other) const;
        bool operator>=(const zhuziString& other) const;

        // 友元
        friend zhuziString operator+(const zhuziString& lhs, const zhuziString& rhs);
        friend zhuziString operator+(const zhuziString& lhs, const wchar_t* rhs);
        friend zhuziString operator+(const wchar_t* lhs, const zhuziString& rhs);
        friend zhuziString operator+(const zhuziString& lhs, wchar_t rhs);
        friend zhuziString operator+(wchar_t lhs, const zhuziString& rhs);

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

    // 全局 operator+
    zhuziString operator+(const zhuziString& lhs, const zhuziString& rhs);
    zhuziString operator+(const zhuziString& lhs, const wchar_t* rhs);
    zhuziString operator+(const wchar_t* lhs, const zhuziString& rhs);
    zhuziString operator+(const zhuziString& lhs, wchar_t rhs);
    zhuziString operator+(wchar_t lhs, const zhuziString& rhs);

    std::wostream& operator<<(std::wostream& wos, const zhuziString& str);
    std::wistream& operator>>(std::wistream& wis, zhuziString& str);

} // namespace zhuzi