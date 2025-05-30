#pragma once
#include <functional>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>

class EventLoop;
class Channel
{
public:
    using callback_t = std::function<void()>;
    Channel(int fd, EventLoop* loop) : _fd(fd), _events(0), _revents(0), _loop(loop){}
    ~Channel() = default;
    void setReadCallback(callback_t cb) { _readCallback = std::move(cb); }
    void setWriteCallback(callback_t cb) { _writeCallback = std::move(cb); }
    void setErrorCallback(callback_t cb) { _errorCallback = std::move(cb); }
    void setCloseCallback(callback_t cb) { _closeCallback = std::move(cb); }
    void setEventCallback(callback_t cb) { _eventCallback = std::move(cb); }
    void setRevents(int revents) { _revents = revents; }
    int getFd() const {return _fd;}
    int getEvents() const {return _events;}
    bool readable() const {return _events & EPOLLIN;}
    bool writable() const {return _events & EPOLLOUT;}
    void enableRead() {_events |= EPOLLIN; update();}
    void disableRead() {_events &= ~EPOLLIN; update();}
    void enableWrite() {_events |= EPOLLOUT; update();}
    void disableWrite() {_events &= ~EPOLLOUT; update();}
    void disableAll() {_events = 0; update();}
    void update();
    void remove();
    void handleEvent()
    {
        // EPOLLIN:  有数据可读
        // EPOLLPRI: 有紧急数据可读
        // EPOLLOUT: 有数据可写
        // EPOLLERR: 错误
        // EPOLLHUP: 对端关闭
        // EPOLLRDHUP: 对端关闭连接
        if((_revents & EPOLLIN) || (_revents & EPOLLPRI) || (_revents & EPOLLRDHUP))
        {
            if(_readCallback)
            {
                _readCallback();
            }
        }
        if((_revents & EPOLLERR))
        {
            if(_errorCallback)
            {
                _errorCallback();
            }
        }
        else if(_revents & EPOLLOUT)
        {
            if(_writeCallback)
            {
                _writeCallback();
            }
        }
        else if(_revents & EPOLLHUP)
        {
            if(_closeCallback)
            {
                _closeCallback();
            }
        }
        if(_eventCallback)
        {
            _eventCallback();
        }
    }

private:
    int _fd;
    int _events;
    int _revents;
    EventLoop* _loop;

    callback_t _readCallback;
    callback_t _writeCallback;
    callback_t _errorCallback;
    callback_t _closeCallback;
    callback_t _eventCallback;
};