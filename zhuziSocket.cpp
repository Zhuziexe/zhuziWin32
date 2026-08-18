#include "zhuziSocket.h"
#include <vector>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

namespace zhuzi {

    // ---------- SocketException ----------
    SocketException::SocketException(const std::string& message)
        : std::runtime_error(message) {}

    SocketException::SocketException(const std::string& message, int errorCode)
        : std::runtime_error(message + " (error code: " + std::to_string(errorCode) + ")") {}

    // ---------- WsaInitializer ----------
    int WsaInitializer::refCount = 0;

    WsaInitializer::WsaInitializer() {
        if (refCount == 0) {
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                throw SocketException("WSAStartup failed");
            }
        }
        ++refCount;
    }

    WsaInitializer::~WsaInitializer() {
        if (--refCount == 0) {
            WSACleanup();
        }
    }

    // ---------- 辅助函数 ----------
    static void throwLastError(const std::string& prefix) {
        int err = WSAGetLastError();
        throw SocketException(prefix + ": " + std::to_string(err), err);
    }

    static std::string sockaddrToString(const sockaddr_in& addr) {
        char buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), buf, INET_ADDRSTRLEN);
        return std::string(buf);
    }

    static uint16_t sockaddrToPort(const sockaddr_in& addr) {
        return ntohs(addr.sin_port);
    }

    // ---------- Socket 实现 ----------
    Socket::Socket() {
        m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_sock == INVALID_SOCKET) {
            throwLastError("socket creation failed");
        }
    }

    Socket::Socket(SOCKET sock) : m_sock(sock) {
        if (m_sock == INVALID_SOCKET) {
            throw SocketException("Invalid socket passed to constructor");
        }
    }

    Socket::~Socket() {
        if (m_sock != INVALID_SOCKET) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
        }
    }

    Socket::Socket(Socket&& other) noexcept : m_sock(other.m_sock) {
        other.m_sock = INVALID_SOCKET;
    }

    Socket& Socket::operator=(Socket&& other) noexcept {
        if (this != &other) {
            if (m_sock != INVALID_SOCKET) {
                closesocket(m_sock);
            }
            m_sock = other.m_sock;
            other.m_sock = INVALID_SOCKET;
        }
        return *this;
    }

    void Socket::close() {
        if (m_sock != INVALID_SOCKET) {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
        }
    }

    void Socket::checkValid() const {
        if (m_sock == INVALID_SOCKET) {
            throw SocketException("Socket is invalid or already closed");
        }
    }

    sockaddr_in Socket::makeSockAddr(const std::string& ip, uint16_t port) const {
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
            throw SocketException("Invalid IP address: " + ip);
        }
        return addr;
    }

    void Socket::bind(uint16_t port, const std::string& ip) {
        checkValid();
        sockaddr_in addr = makeSockAddr(ip, port);
        if (::bind(m_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
            throwLastError("bind failed");
        }
    }

    void Socket::listen(int backlog) {
        checkValid();
        if (::listen(m_sock, backlog) == SOCKET_ERROR) {
            throwLastError("listen failed");
        }
    }

    Socket Socket::accept() {
        checkValid();
        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET clientSock = ::accept(m_sock, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
        if (clientSock == INVALID_SOCKET) {
            throwLastError("accept failed");
        }
        return Socket(clientSock);
    }

    Socket Socket::accept(std::string& clientIp, uint16_t& clientPort) {
        checkValid();
        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET clientSock = ::accept(m_sock, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
        if (clientSock == INVALID_SOCKET) {
            throwLastError("accept failed");
        }
        clientIp = sockaddrToString(clientAddr);
        clientPort = sockaddrToPort(clientAddr);
        return Socket(clientSock);
    }

    void Socket::connect(const std::string& ip, uint16_t port) {
        checkValid();
        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
            // 域名解析
            struct addrinfo hints = {}, * result = nullptr;
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            if (getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
                throw SocketException("Failed to resolve host: " + ip);
            }
            bool connected = false;
            for (auto* rp = result; rp != nullptr; rp = rp->ai_next) {
                if (::connect(m_sock, rp->ai_addr, static_cast<int>(rp->ai_addrlen)) != SOCKET_ERROR) {
                    connected = true;
                    break;
                }
            }
            freeaddrinfo(result);
            if (!connected) {
                throwLastError("connect failed (all addresses)");
            }
        }
        else {
            if (::connect(m_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR) {
                throwLastError("connect failed");
            }
        }
    }

    // ===== 修改点1：connect(zhuziString) 使用 to_utf8 =====
    void Socket::connect(const zhuziString& ip, uint16_t port) {
        char ipBuf[64];
        ip.to_utf8(ipBuf, sizeof(ipBuf));
        connect(std::string(ipBuf), port);
    }

    int Socket::send(const void* data, int len, int flags) {
        checkValid();
        if (len <= 0) return 0;
        int ret = ::send(m_sock, static_cast<const char*>(data), len, flags);
        if (ret == SOCKET_ERROR) {
            throwLastError("send failed");
        }
        return ret;
    }

    int Socket::recv(void* buf, int len, int flags) {
        checkValid();
        if (len <= 0) return 0;
        int ret = ::recv(m_sock, static_cast<char*>(buf), len, flags);
        if (ret == SOCKET_ERROR) {
            throwLastError("recv failed");
        }
        return ret;
    }

    int Socket::sendAll(const void* data, size_t len, int flags) {
        checkValid();
        const char* ptr = static_cast<const char*>(data);
        size_t remaining = len;
        while (remaining > 0) {
            int sent = ::send(m_sock, ptr, static_cast<int>(remaining), flags);
            if (sent == SOCKET_ERROR) {
                throwLastError("sendAll failed");
            }
            ptr += sent;
            remaining -= sent;
        }
        return static_cast<int>(len);
    }

    int Socket::recvAll(void* buf, size_t len, int flags) {
        checkValid();
        char* ptr = static_cast<char*>(buf);
        size_t remaining = len;
        while (remaining > 0) {
            int received = ::recv(m_sock, ptr, static_cast<int>(remaining), flags);
            if (received == SOCKET_ERROR) {
                throwLastError("recvAll failed");
            }
            if (received == 0) {
                throw SocketException("recvAll: connection closed by peer");
            }
            ptr += received;
            remaining -= received;
        }
        return static_cast<int>(len);
    }

    // ===== 修改点2：send(zhuziString) 使用 to_utf8 =====
    int Socket::send(const zhuziString& data, int flags) {
        // 先计算需要的 UTF-8 长度（含 '\0'）
        int len = WideCharToMultiByte(CP_UTF8, 0, data.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) return 0;
        std::vector<char> utf8(len);
        data.to_utf8(utf8.data(), len);
        // 发送时排除结尾的 '\0'
        return sendAll(utf8.data(), len - 1, flags);
    }

    // ===== 修改点3：sendAll(zhuziString) 使用 to_utf8 =====
    int Socket::sendAll(const zhuziString& data, int flags) {
        int len = WideCharToMultiByte(CP_UTF8, 0, data.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (len <= 0) return 0;
        std::vector<char> utf8(len);
        data.to_utf8(utf8.data(), len);
        return sendAll(utf8.data(), len - 1, flags);
    }

    // ---------- recvLine 保持不变（未使用 c_charptr） ----------
    zhuziString Socket::recvLine(int flags) {
        std::string line;
        char ch;
        while (true) {
            int ret = recv(&ch, 1, flags);
            if (ret <= 0) {
                if (ret == 0) break;
                throwLastError("recvLine failed");
            }
            if (ch == '\n') break;
            if (ch != '\r') line.push_back(ch);
        }
        return zhuziString::FromUTF8(line.c_str());
    }

    // ---------- 其他成员 ----------
    void Socket::shutdown(int how) {
        checkValid();
        if (::shutdown(m_sock, how) == SOCKET_ERROR) {
            throwLastError("shutdown failed");
        }
    }

    void Socket::setReuseAddr(bool enable) {
        checkValid();
        int value = enable ? 1 : 0;
        if (setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR,
            reinterpret_cast<char*>(&value), sizeof(value)) == SOCKET_ERROR) {
            throwLastError("setReuseAddr failed");
        }
    }

    void Socket::setBlocking(bool blocking) {
        checkValid();
        u_long mode = blocking ? 0 : 1;
        if (ioctlsocket(m_sock, FIONBIO, &mode) == SOCKET_ERROR) {
            throwLastError("setBlocking failed");
        }
    }

    void Socket::getSockName(sockaddr_in& addr) const {
        checkValid();
        int addrLen = sizeof(addr);
        if (getsockname(m_sock, reinterpret_cast<sockaddr*>(&addr), &addrLen) == SOCKET_ERROR) {
            throwLastError("getsockname failed");
        }
    }

    void Socket::getPeerName(sockaddr_in& addr) const {
        checkValid();
        int addrLen = sizeof(addr);
        if (getpeername(m_sock, reinterpret_cast<sockaddr*>(&addr), &addrLen) == SOCKET_ERROR) {
            throwLastError("getpeername failed");
        }
    }

    std::string Socket::getLocalIp() const {
        sockaddr_in addr;
        getSockName(addr);
        return sockaddrToString(addr);
    }

    uint16_t Socket::getLocalPort() const {
        sockaddr_in addr;
        getSockName(addr);
        return sockaddrToPort(addr);
    }

    std::string Socket::getPeerIp() const {
        sockaddr_in addr;
        getPeerName(addr);
        return sockaddrToString(addr);
    }

    uint16_t Socket::getPeerPort() const {
        sockaddr_in addr;
        getPeerName(addr);
        return sockaddrToPort(addr);
    }

} // namespace zhuzi