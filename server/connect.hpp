#pragma once
#include "eventloop.hpp"
#include "buffer.hpp"
#include "channel.hpp"
#include "socket.hpp"
#include <memory>
#include <any>

enum class ConnectionState
{
    K_DISCONNECTED,
    K_CONNECTING,
    K_CONNECTED,
    K_DISCONNECTING
};

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    using ptrConnection = std::shared_ptr<Connection>;
    using connectedCallback = std::function<void(const ptrConnection&)>;
    using messageCallback = std::function<void(const ptrConnection&, Buffer*)>;
    using closeCallback = std::function<void(const ptrConnection&)>;
    using eventCallback = std::function<void(const ptrConnection&)>;

    Connection(EventLoop* loop, uint64_t connId, int sockfd)
    : _id(connId),
    _fd(sockfd),
    _inactiveRelease(false),
    _loop(loop),
    _state(ConnectionState::K_DISCONNECTED),
    _socket(sockfd),
    _channel(sockfd, loop)
    {
        _channel.setReadCallback([this]{handleRead();});
        _channel.setWriteCallback([this]{handleWrite();});
        _channel.setCloseCallback([this]{handleClose();});
        _channel.setEventCallback([this]{handleEvent();});
    }
    ~Connection() = default;
    uint64_t getId() const {return _id;}
    int getFd() const {return _fd;}
    EventLoop* getLoop() const {return _loop;}
    ConnectionState getState() const {return _state;}
    std::any* getContext() {return &_context;}
    bool isConnected() const {return _state == ConnectionState::K_CONNECTED;}

    void setConnectedCallback(const connectedCallback& cb) {_connectedCb = cb;}
    void setMessageCallback(const messageCallback& cb) {_messageCb = cb;}
    void setCloseCallback(const closeCallback& cb) {_closeCb = cb;}
    void setServerCloseCallback(const closeCallback& cb) {_serverCloseCb = cb;}
    void setEventCallback(const eventCallback& cb) {_eventCb = cb;}

    void establish()
    {
        _loop->runInLoop([this]{_establish();});
    }
    void send(const char* data, size_t len)
    {
        Buffer buf;
        buf.write(data, len);
        _loop->runInLoop([this, buf]{_send(std::move(buf));});
    }
    void shutdown()
    {
        _loop->runInLoop([this]{_shutdownInLoop();});
    }
    void enableInactivityRelease(int timeout)
    {
        _loop->runInLoop([this, timeout]{_enableInactivityRelease(timeout);});
    }
    void disableInactivityRelease()
    {
        _loop->runInLoop([this]{_disableInactivityRelease();});
    }
    void upgrade(const std::any& context, const connectedCallback& cb,
                 const messageCallback& msgCb, const closeCallback& closeCb,
                 const eventCallback& eventCb)
    {
        if(!_loop->isInLoopThread())
        {
            perror("not in loop thread");
            exit(1);
        }
        _loop->runInLoop([this, context, cb, msgCb, closeCb, eventCb]{_upgrade(context, cb, msgCb, closeCb, eventCb);});
    }
    void setContext(const std::any& context) {_context = context;}

private:
    void handleRead()
    {
        char buf[65536];
        ssize_t n = _socket.recv(buf, sizeof(buf), MSG_DONTWAIT);
        if(n > 0)
        {
            _input.write(buf, n);
            if(_input.readableSize() > 0)
            {
                if(_messageCb)
                {
                    _messageCb(shared_from_this(), &_input);
                }
            }
        }
        else if(n == 0)
        {
            //not recieve data
            return;
        }
        else
        {
            _shutdownInLoop();
        }
    }
    void handleWrite()
    {
        ssize_t n = _socket.send(_output.readPos(), _output.readableSize(), MSG_DONTWAIT);
        if(n > 0)
        {
            _output.moveReadIdx(n);
            if(_output.readableSize() == 0)
            {
                _channel.disableWrite();
                if(_state == ConnectionState::K_DISCONNECTING)
                {
                    _close();
                }
            }
        }
        else if(n == 0)
        {
            //not send data
            return;
        }
        else //disconnect
        {
            if(_input.readableSize() > 0)
            {
                if(_messageCb)
                {
                    _messageCb(shared_from_this(), &_input);
                }
            }
            _close();
        }
    }
    void handleClose()
    {
        if(_input.readableSize() > 0)
        {
            if(_messageCb)
            {
                _messageCb(shared_from_this(), &_input);
            }
        }
        _close();
    }
    void handleEvent()
    {
        if(_inactiveRelease)
        {
            _loop->refreshAfter(_id);
        }
        if(_eventCb)
        {
            _eventCb(shared_from_this());
        }
    }
    void _send(const Buffer& buf)
    {
        if(_state == ConnectionState::K_CONNECTED)
        {
            _output.Write(buf);
            if(!_channel.writable())
            {
                _channel.enableWrite();
            }
        }
    }
    void _enableInactivityRelease(int timeout)
    {
        _inactiveRelease = true;
        _loop->hasAfter(_id) ? 
        _loop->refreshAfter(_id) : _loop->runAfter(_id, timeout, [this] {_close();});
    }
    void _disableInactivityRelease()
    {
        _inactiveRelease = false;
        _loop->removeAfter(_id);
    }
    void _upgrade(const std::any& context, const connectedCallback& cb,
                 const messageCallback& msgCb, const closeCallback& closeCb,
                 const eventCallback& eventCb)
    {
        _context = context;
        _connectedCb = cb;
        _messageCb = msgCb;
        _closeCb = closeCb;
        _eventCb = eventCb;
    }
    void _establish()
    {
        if(_state != ConnectionState::K_CONNECTING)
        {
            perror("not in connecting state");
            exit(1);
        }
        _state = ConnectionState::K_CONNECTED;
        _channel.enableRead();
        if(_connectedCb)
        {
            _connectedCb(shared_from_this());
        }
    }
    void _shutdownInLoop()
    {
        _state = ConnectionState::K_DISCONNECTING;
        if(_input.readableSize() > 0)
        {
            if(_messageCb)
            {
                _messageCb(shared_from_this(), &_input);
            }
        }
        if(_output.readableSize() > 0)
        {
            if(!_channel.writable())
            {
                _channel.enableWrite();
            }
        }
        else
        {
            _close();
        }
    }
    void _close()
    {
        _loop->runInLoop([this]{_closeInLoop();});
    }
    void _closeInLoop()
    {
        if(_state == ConnectionState::K_DISCONNECTED) return;
        _state = ConnectionState::K_DISCONNECTED;
        _channel.remove();
        _socket.close();
        _loop->removeAfter(_id);
        if(_closeCb)
        {
            _closeCb(shared_from_this());
        }
        if(_serverCloseCb)
        {
            _serverCloseCb(shared_from_this());
        }
    }
private:
    uint64_t _id;
    int _fd;
    bool _inactiveRelease;
    EventLoop* _loop;
    ConnectionState _state;
    Socket _socket;
    Channel _channel;
    Buffer _input;
    Buffer _output;
    std::any _context;

    connectedCallback _connectedCb;
    messageCallback _messageCb;
    closeCallback _closeCb;
    closeCallback _serverCloseCb;
    eventCallback _eventCb;
};