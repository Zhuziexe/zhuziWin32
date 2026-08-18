#ifndef ZHUZI_SOCKET_H
#define ZHUZI_SOCKET_H

// ===== 防止 winsock.h 与 winsock2.h 冲突 =====
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WINSOCKAPI_
#define _WINSOCKAPI_
#endif
// ==========================================

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdexcept>
#include <string>
#include "zhuziString.h"

namespace zhuzi {

    class SocketException : public std::runtime_error {
    public:
        explicit SocketException(const std::string& message);
        SocketException(const std::string& message, int errorCode);
    };

    class WsaInitializer {
    public:
        WsaInitializer();
        ~WsaInitializer();
        WsaInitializer(const WsaInitializer&) = delete;
        WsaInitializer& operator=(const WsaInitializer&) = delete;
    private:
        static int refCount;
    };

    class Socket {
    public:
        Socket();
        explicit Socket(SOCKET sock);
        ~Socket();

        Socket(Socket&& other) noexcept;
        Socket& operator=(Socket&& other) noexcept;

        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        void close();

        // 服务器端
        void bind(uint16_t port, const std::string& ip = "0.0.0.0");
        void listen(int backlog = SOMAXCONN);
        Socket accept();
        Socket accept(std::string& clientIp, uint16_t& clientPort);

        // 客户端连接（std::string 和 zhuziString）
        void connect(const std::string& ip, uint16_t port);
        void connect(const zhuziString& ip, uint16_t port);

        // 收发（原始字节）
        int send(const void* data, int len, int flags = 0);
        int recv(void* buf, int len, int flags = 0);
        int sendAll(const void* data, size_t len, int flags = 0);
        int recvAll(void* buf, size_t len, int flags = 0);

        // zhuziString 重载
        int send(const zhuziString& data, int flags = 0);
        int sendAll(const zhuziString& data, int flags = 0);
        zhuziString recvLine(int flags = 0);   // 接收一行（以 '\n' 结尾）

        // 选项
        void shutdown(int how = SD_BOTH);
        void setReuseAddr(bool enable);
        void setBlocking(bool blocking);

        // 地址信息
        std::string getLocalIp() const;
        uint16_t getLocalPort() const;
        std::string getPeerIp() const;
        uint16_t getPeerPort() const;

        SOCKET nativeHandle() const { return m_sock; }

    private:
        SOCKET m_sock;
        void checkValid() const;
        void getSockName(sockaddr_in& addr) const;
        void getPeerName(sockaddr_in& addr) const;
        sockaddr_in makeSockAddr(const std::string& ip, uint16_t port) const;
    };

} // namespace zhuzi

#endif