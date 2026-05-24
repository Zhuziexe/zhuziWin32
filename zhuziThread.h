#pragma once
#include <Windows.h>
#include <process.h>    // _beginthreadex
#include <stdexcept>

namespace zhuzi {

    // 线程入口函数类型（必须使用 WINAPI）
    using ThreadEntry = unsigned (WINAPI*)(LPVOID);

    class zhuziThread {
    public:
        // 构造函数：直接启动线程
        // entry   : 线程函数（不能为 nullptr）
        // param   : 传递给线程函数的参数
        // threadId: 可选输出，返回线程 ID
        zhuziThread(ThreadEntry entry, LPVOID param = nullptr, LPDWORD threadId = nullptr)
            : m_hThread(nullptr)
        {
            if (entry == nullptr) {
                throw std::runtime_error("Thread entry function is NULL");
            }

            unsigned int tid;
            m_hThread = reinterpret_cast<HANDLE>(
                _beginthreadex(nullptr, 0, entry, param, 0, &tid)
                );
            if (m_hThread == nullptr) {
                throw std::runtime_error("Failed to create thread");
            }
            if (threadId) {
                *threadId = static_cast<DWORD>(tid);
            }
        }

        // 禁止拷贝
        zhuziThread(const zhuziThread&) = delete;
        zhuziThread& operator=(const zhuziThread&) = delete;

        // 移动语义
        zhuziThread(zhuziThread&& other) noexcept
            : m_hThread(other.m_hThread)
        {
            other.m_hThread = nullptr;
        }

        zhuziThread& operator=(zhuziThread&& other) noexcept {
            if (this != &other) {
                if (m_hThread) {
                    // 先释放当前资源（分离）
                    CloseHandle(m_hThread);
                }
                m_hThread = other.m_hThread;
                other.m_hThread = nullptr;
            }
            return *this;
        }

        // 析构函数：自动分离线程（仅关闭句柄，不等待线程结束）
        ~zhuziThread() {
            if (m_hThread) {
                CloseHandle(m_hThread);
                m_hThread = nullptr;
            }
        }

        // 等待线程结束（带超时），不自动关闭句柄
        DWORD wait(DWORD milliseconds = INFINITE) const {
            if (!m_hThread) {
                return WAIT_FAILED;
            }
            return WaitForSingleObject(m_hThread, milliseconds);
        }

        // 等待线程结束并关闭句柄（类似 join）
        // 返回值：true 表示成功等待并关闭，false 表示超时或失败
        bool join(DWORD timeoutMs = INFINITE) {
            if (!m_hThread) {
                return false;
            }
            DWORD ret = WaitForSingleObject(m_hThread, timeoutMs);
            if (ret == WAIT_OBJECT_0) {
                CloseHandle(m_hThread);
                m_hThread = nullptr;
                return true;
            }
            return false;   // 超时或失败
        }

        // 分离线程：不再管理生命周期（仅关闭句柄）
        void detach() {
            if (m_hThread) {
                CloseHandle(m_hThread);
                m_hThread = nullptr;
            }
        }

        // 获取线程退出码（线程未结束时返回 STILL_ACTIVE）
        DWORD getExitCode() const {
            if (!m_hThread) {
                return STILL_ACTIVE;
            }
            DWORD code;
            GetExitCodeThread(m_hThread, &code);
            return code;
        }

        // 线程是否仍在运行
        bool isRunning() const {
            return m_hThread && (getExitCode() == STILL_ACTIVE);
        }

        // 获取原始句柄（谨慎使用）
        HANDLE getHandle() const {
            return m_hThread;
        }

        // 手动关闭句柄（等同于 detach）
        BOOL closeHandle() {
            if (!m_hThread) {
                return FALSE;
            }
            BOOL ret = CloseHandle(m_hThread);
            m_hThread = nullptr;
            return ret;
        }

        // 危险操作：终止线程（默认禁用）
        BOOL terminate(DWORD ExitCode) {
            // 强烈不推荐，为接口兼容而保留，但直接禁用
            // 如需启用，请手动注释下面两行并自行承担风险
            // 此处直接返回 FALSE 或抛出异常
            throw std::logic_error("TerminateThread is disabled for safety");
            // if (!m_hThread) return FALSE;
            // return TerminateThread(m_hThread, ExitCode);
        }

    private:
        HANDLE m_hThread;
    };

} // namespace zhuzi