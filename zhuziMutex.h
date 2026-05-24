#pragma once
#include <Windows.h>
#include <stdexcept>

namespace zhuzi {

    // 互斥锁（基于临界区，非递归）
    class zhuziMutex {
    public:
        zhuziMutex() {
            InitializeCriticalSection(&m_cs);
        }

        ~zhuziMutex() {
            DeleteCriticalSection(&m_cs);
        }

        // 禁止拷贝和移动（临界区不可复制/移动）
        zhuziMutex(const zhuziMutex&) = delete;
        zhuziMutex& operator=(const zhuziMutex&) = delete;
        zhuziMutex(zhuziMutex&&) = delete;
        zhuziMutex& operator=(zhuziMutex&&) = delete;

        // 加锁（阻塞直到获得锁）
        void lock() {
            EnterCriticalSection(&m_cs);
        }

        // 解锁
        void unlock() {
            LeaveCriticalSection(&m_cs);
        }

        // 尝试加锁（非阻塞）
        bool tryLock() {
            return TryEnterCriticalSection(&m_cs) != FALSE;
        }

        // 获取原始临界区指针（谨慎使用）
        LPCRITICAL_SECTION nativeHandle() const {
            return &m_cs;
        }

    private:
        mutable CRITICAL_SECTION m_cs;
    };

    // RAII 锁守卫（自动加锁/解锁）
    class zhuziMutexGuard {
    public:
        explicit zhuziMutexGuard(zhuziMutex& mutex) : m_mutex(mutex) {
            m_mutex.lock();
        }

        ~zhuziMutexGuard() {
            m_mutex.unlock();
        }

        // 禁止拷贝和移动
        zhuziMutexGuard(const zhuziMutexGuard&) = delete;
        zhuziMutexGuard& operator=(const zhuziMutexGuard&) = delete;
        zhuziMutexGuard(zhuziMutexGuard&&) = delete;
        zhuziMutexGuard& operator=(zhuziMutexGuard&&) = delete;

    private:
        zhuziMutex& m_mutex;
    };

} // namespace zhuzi