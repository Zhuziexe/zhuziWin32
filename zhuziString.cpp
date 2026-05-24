#include "zhuziString.h"
#include <windows.h>
#include <cstring>
#include <algorithm>

#define MULTIBYTE2WCHAR_CP CP_ACP
namespace zhuzi {

    static size_t wcslen_safe(const wchar_t* str) {
        if (!str) return 0;
        size_t len = 0;
        while (str[len]) ++len;
        return len;
    }

    static wchar_t* utf8_to_wchar(const char* utf8) {
        if (!utf8 || !*utf8) {
            wchar_t* empty = new wchar_t[1];
            empty[0] = L'\0';
            return empty;
        }
        int len = MultiByteToWideChar(MULTIBYTE2WCHAR_CP, 0, utf8, -1, nullptr, 0);
        if (len <= 0) {
            wchar_t* empty = new wchar_t[1];
            empty[0] = L'\0';
            return empty;
        }
        wchar_t* wstr = new wchar_t[len];
        MultiByteToWideChar(MULTIBYTE2WCHAR_CP, 0, utf8, -1, wstr, len);
        return wstr;
    }

    static char* wchar_to_utf8(const wchar_t* wstr) {
        if (!wstr || !*wstr) {
            char* empty = new char[1];
            empty[0] = '\0';
            return empty;
        }
        int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) {
            char* empty = new char[1];
            empty[0] = '\0';
            return empty;
        }
        char* utf8 = new char[len];
        WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8, len, nullptr, nullptr);
        return utf8;
    }

    void zhuziString::allocate(size_t capacity) {
        m_data = new wchar_t[capacity + 1];
        m_capacity = capacity;
        m_len = 0;
        if (capacity > 0) m_data[0] = L'\0';
    }

    void zhuziString::release() {
        if (m_data) {
            delete[] m_data;
            m_data = nullptr;
        }
        m_len = 0;
        m_capacity = 0;
    }

    void zhuziString::copyFrom(const wchar_t* src, size_t len) {
        if (len == 0) {
            ensureCapacity(0); // 确保至少有一个字符的空间
            if (!m_data) {
                allocate(0);   // 如果 ensureCapacity 没有分配（因为 capacity 可能已够），但 m_data 为空则手动分配
            }
            m_data[0] = L'\0';
            m_len = 0;
            return;
        }
        ensureCapacity(len);
        wmemcpy(m_data, src, len);
        m_data[len] = L'\0';
        m_len = len;
    }

    void zhuziString::moveFrom(zhuziString&& other) {
        m_data = other.m_data;
        m_len = other.m_len;
        m_capacity = other.m_capacity;
        other.m_data = nullptr;
        other.m_len = 0;
        other.m_capacity = 0;
    }

    void zhuziString::ensureCapacity(size_t newLen) {
        if (newLen <= m_capacity) return;
        size_t newCap = m_capacity * 2 + 16;
        if (newCap < newLen) newCap = newLen;
        wchar_t* newData = new wchar_t[newCap + 1];
        if (m_data) {
            wmemcpy(newData, m_data, m_len);
            delete[] m_data;
        }
        m_data = newData;
        m_capacity = newCap;
    }

    zhuziString::zhuziString() : m_data(nullptr), m_len(0), m_capacity(0) {
        allocate(0);
    }

    zhuziString::zhuziString(const char* str) : m_data(nullptr), m_len(0), m_capacity(0) {
        wchar_t* wstr = utf8_to_wchar(str);
        size_t len = wcslen_safe(wstr);
        copyFrom(wstr, len);
        delete[] wstr;
    }

    zhuziString::zhuziString(const wchar_t* str) : m_data(nullptr), m_len(0), m_capacity(0) {
        size_t len = wcslen_safe(str);
        copyFrom(str, len);
    }

    zhuziString::zhuziString(size_t count, wchar_t ch) : m_data(nullptr), m_len(0), m_capacity(0) {
        if (count == 0) {
            allocate(0);
            return;
        }
        allocate(count);
        for (size_t i = 0; i < count; ++i) m_data[i] = ch;
        m_data[count] = L'\0';
        m_len = count;
    }

    zhuziString::zhuziString(const zhuziString& other) : m_data(nullptr), m_len(0), m_capacity(0) {
        copyFrom(other.m_data, other.m_len);
    }

    zhuziString::zhuziString(zhuziString&& other) noexcept : m_data(nullptr), m_len(0), m_capacity(0) {
        moveFrom(std::move(other));
    }

    zhuziString::~zhuziString() {
        release();
    }

    zhuziString& zhuziString::operator=(const char* str) {
        zhuziString temp(str);
        std::swap(m_data, temp.m_data);
        std::swap(m_len, temp.m_len);
        std::swap(m_capacity, temp.m_capacity);
        return *this;
    }

    zhuziString& zhuziString::operator=(const wchar_t* str) {
        zhuziString temp(str);
        std::swap(m_data, temp.m_data);
        std::swap(m_len, temp.m_len);
        std::swap(m_capacity, temp.m_capacity);
        return *this;
    }

    zhuziString& zhuziString::operator=(const zhuziString& other) {
        if (this == &other) return *this;
        zhuziString temp(other);
        std::swap(m_data, temp.m_data);
        std::swap(m_len, temp.m_len);
        std::swap(m_capacity, temp.m_capacity);
        return *this;
    }

    zhuziString& zhuziString::operator=(zhuziString&& other) noexcept {
        if (this != &other) {
            release();
            moveFrom(std::move(other));
        }
        return *this;
    }

    size_t zhuziString::size() const { return m_len; }
    size_t zhuziString::length() const { return m_len; }
    bool zhuziString::empty() const { return m_len == 0; }

    void zhuziString::reserve(size_t newCapacity) {
        if (newCapacity > m_capacity) {
            ensureCapacity(newCapacity);
        }
    }

    void zhuziString::resize(size_t newSize, wchar_t ch) {
        if (newSize <= m_len) {
            m_data[newSize] = L'\0';
            m_len = newSize;
        }
        else {
            ensureCapacity(newSize);
            for (size_t i = m_len; i < newSize; ++i) m_data[i] = ch;
            m_data[newSize] = L'\0';
            m_len = newSize;
        }
    }

    wchar_t zhuziString::operator[](size_t index) const {
        if (index >= m_len) return L'\0';
        return m_data[index];
    }

    wchar_t& zhuziString::operator[](size_t index) {
        if (index > m_len) index = m_len;
        return m_data[index];
    }

    const wchar_t* zhuziString::c_str() const { return m_data ? m_data : L""; }
    const wchar_t* zhuziString::data() const { return c_str(); }

    void zhuziString::clear() {
        if (m_data) m_data[0] = L'\0';
        m_len = 0;
    }

    void zhuziString::push_back(wchar_t ch) {
        ensureCapacity(m_len + 1);
        m_data[m_len] = ch;
        m_data[++m_len] = L'\0';
    }

    zhuziString& zhuziString::operator+=(const zhuziString& other) {
        ensureCapacity(m_len + other.m_len);
        wmemcpy(m_data + m_len, other.m_data, other.m_len);
        m_len += other.m_len;
        m_data[m_len] = L'\0';
        return *this;
    }

    zhuziString& zhuziString::operator+=(const wchar_t* str) {
        size_t len = wcslen_safe(str);
        ensureCapacity(m_len + len);
        wmemcpy(m_data + m_len, str, len);
        m_len += len;
        m_data[m_len] = L'\0';
        return *this;
    }

    zhuziString& zhuziString::operator+=(wchar_t ch) {
        push_back(ch);
        return *this;
    }

    bool zhuziString::operator==(const zhuziString& other) const {
        if (m_len != other.m_len) return false;
        return wcscmp(m_data, other.m_data) == 0;
    }
    bool zhuziString::operator!=(const zhuziString& other) const { return !(*this == other); }

    const char* zhuziString::c_charptr() const {
        return wchar_to_utf8(m_data);
    }

    // 全局 operator+ 实现
    zhuziString operator+(const zhuziString& lhs, const zhuziString& rhs) {
        zhuziString result = lhs;
        result += rhs;
        return result;
    }
    zhuziString operator+(const zhuziString& lhs, const wchar_t* rhs) {
        zhuziString result = lhs;
        result += rhs;
        return result;
    }
    zhuziString operator+(const wchar_t* lhs, const zhuziString& rhs) {
        zhuziString result(lhs);
        result += rhs;
        return result;
    }
    zhuziString operator+(const zhuziString& lhs, wchar_t rhs) {
        zhuziString result = lhs;
        result.push_back(rhs);
        return result;
    }
    zhuziString operator+(wchar_t lhs, const zhuziString& rhs) {
        zhuziString result(1, lhs);
        result += rhs;
        return result;
    }

    bool zhuziString::operator<(const zhuziString& other) const {
        return wcscmp(m_data, other.m_data) < 0;
    }
    bool zhuziString::operator>(const zhuziString& other) const {
        return wcscmp(m_data, other.m_data) > 0;
    }
    bool zhuziString::operator<=(const zhuziString& other) const {
        return wcscmp(m_data, other.m_data) <= 0;
    }
    bool zhuziString::operator>=(const zhuziString& other) const {
        return wcscmp(m_data, other.m_data) >= 0;
    }

} // namespace zhuzi