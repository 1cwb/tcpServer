#pragma once
#include "channel.hpp"
#include "socket.hpp"

class Accepter
{
public:
    using acceptCallback = std::function<void(int)>;
    Accepter(EventLoop* loop, uint16_t port, acceptCallback cb = nullptr)
    : _socket(this->createServer(port))
    , _channel(_socket.getFd(), loop)
    , _acceptCallback(std::move(cb))
    {
        _channel.setReadCallback([this]{handleRead();});
    }
    void setAcceptCallback(const acceptCallback& cb)
    {
        _acceptCallback = cb;
    }
    void listen()
    {
        _channel.enableRead();
    }
private:
    int createServer(uint16_t port)
    {
        if(!_socket.createServer(port, false))
        {
            perror("createServer");
            return -1;
        }
        return _socket.getFd();
    }
    void handleRead()
    {
        int fd = _socket.accept();
        if(fd != -1)
        {
            if(_acceptCallback)
            {
                _acceptCallback(fd);
            }
        }
    }
private:
    Socket _socket;
    Channel _channel;
    acceptCallback _acceptCallback;
};
