#pragma once
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <string>

class Socket
{
public:
    Socket() : _sockfd(-1) {}
    explicit Socket(int fd) : _sockfd(fd) {}
    ~Socket() { ::close(_sockfd); }
    Socket(const Socket&) = delete;
    Socket(Socket&&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket& operator=(Socket&&) = delete;

    int getFd() const { return _sockfd; }
    bool creat() {
        _sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (_sockfd == -1) {
            return false;
        }
        return true;
    }
    bool bind(const std::string& ip, uint16_t port) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        socklen_t len = sizeof(addr);
        if (::bind(_sockfd, reinterpret_cast<sockaddr*>(&addr), len) == -1) {
            return false;
        }
        return true;
    }
    bool listen(int backlog) {
        if (::listen(_sockfd, backlog) == -1) {
            return false;
        }
        return true;
    }
    bool connect(const std::string& ip, uint16_t port) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());
        socklen_t len = sizeof(addr);
        if (::connect(_sockfd, reinterpret_cast<sockaddr*>(&addr), len) == -1) {
            return false;
        }
        return true;
    }
    int accept() {
        sockaddr_in addr{};
        socklen_t len = sizeof(addr);
        int fd = ::accept(_sockfd, reinterpret_cast<sockaddr*>(&addr), &len);
        if (fd == -1) {
            return -1;
        }
        return fd;
    }
    ssize_t recv(void* buf, std::size_t len, int flag = 0)
    {
        if(len == 0)
        {
            return 0;
        }
        ssize_t ret = ::recv(_sockfd, buf, len, flag);
        if (ret == -1)
        {
            if(errno == EAGAIN || errno == EINTR)
            {
                return 0;
            }
        }
        return ret;
    }
    ssize_t send(const void* buf, std::size_t len, int flag = 0)
    {
        if(len == 0)
        {
            return 0;
        }
        ssize_t ret = ::send(_sockfd, buf, len, flag);
        if (ret == -1)
        {
            if(errno == EAGAIN || errno == EINTR)
            {
                return 0;
            }
        }
        return ret;
    }
    void close()
    {
        ::close(_sockfd);
    }
    bool createServer(uint16_t port, bool block = true, const std::string& ip = "0.0.0.0", int backlog = 1024)
    {
        if(!creat())
        {
            return false;
        }
        if(!bind(ip, port))
        {
            return false;
        }
        if(!listen(backlog))
        {
            return false;
        }
        if(!block)
        {
            if(!nonBlock())
            {
                return false;
            }
        }
        if(!reuseAddr())
        {
            return false;
        }
        return true;
    }
    bool createClient(uint16_t port, const std::string& ip)
    {
        if(!creat())
        {
            return false;
        }
        if(!connect(ip, port))
        {
            return false;
        }
        nonBlock();
        return true;
    }
    bool reuseAddr()
    {
        int opt = 1;
        if(setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        {
            return false;
        }
        opt = 1;
        if(setsockopt(_sockfd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
        {
            return false;
        }
        return true;
    }
    bool nonBlock()
    {
        int flag = fcntl(_sockfd, F_GETFL, 0);
        if(flag == -1)
        {
            return false;
        }
        flag |= O_NONBLOCK;
        if(fcntl(_sockfd, F_SETFL, flag) == -1)
        {
            return false;
        }
        return true;
    }
private:
    int _sockfd;
};
