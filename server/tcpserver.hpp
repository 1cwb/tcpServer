#pragma once
#include "network.hpp"
#include "eventloop.hpp"
#include "accepter.hpp"
#include "loopthreadpool.hpp"
#include "connect.hpp"

class TcpServer : public NetWork
{
public:
    using ptrConnection = Connection::ptrConnection;
    using connectedCallback = Connection::connectedCallback;
    using messageCallback = Connection::messageCallback;
    using closeCallback = Connection::closeCallback;
    using eventCallback = Connection::eventCallback;
    explicit TcpServer(int port, int threadNum = 0)
    : _nextId(0)
    ,  _timeout(0)
    ,  _inactiveRelease(false)
    ,  _accepter(&_baseLoop, port, [this](auto && PH1) { newConnection(std::forward<decltype(PH1)>(PH1)); })
    ,  _threadPool(&_baseLoop, threadNum)
    {
        _threadPool.creat();
        _accepter.listen();
    }
    void setConnectedCallback(const connectedCallback& cb) {_connectedCallback = cb;}
    void setMessageCallback(const messageCallback& cb) {_messageCallback = cb;}
    void setCloseCallback(const closeCallback& cb) {_closeCallback = cb;}
    void setEventCallback(const eventCallback& cb) {_eventCallback = cb;}
    void enableInactivityRelease(int timeout) 
    {
        _inactiveRelease = true;
        _timeout = timeout;
    }
    void disableInactivityRelease() {_inactiveRelease = false;}
    void start() {_baseLoop.start();}
    void runAfter(uint64_t timeout, const TimerTask::TaskFunc& task)
    {
        _nextId ++;
        _baseLoop.runInLoop([this, timeout, task]{_runAfter(_nextId, timeout, task);});
    }
    private:
    void newConnection(int fd)
    {
        _nextId ++;
        ptrConnection conn(new Connection(_threadPool.getNextLoop(), _nextId, fd));
        conn->setConnectedCallback(_connectedCallback);
        conn->setMessageCallback(_messageCallback);
        conn->setCloseCallback(_closeCallback);
        conn->setEventCallback(_eventCallback);
        conn->setServerCloseCallback([this](auto && PH1) {removeConnection(std::forward<decltype(PH1)>(PH1));});
        if(_inactiveRelease)
        {
            conn->enableInactivityRelease(_timeout);
        }
        conn->establish();
        _connections[_nextId] = conn;
    }
    void removeConnection(const ptrConnection& conn)
    {
        _baseLoop.runInLoop([this, conn]{_removeConnection(conn);});
    }
    void _removeConnection(const ptrConnection& conn)
    {
        _connections.erase(conn->getId());
    }
    void _runAfter(uint64_t id, uint64_t timeout, const TimerTask::TaskFunc& task)
    {
        _baseLoop.runAfter(id, timeout, task);
    }
private:
    uint64_t _nextId;
    int _timeout;
    bool _inactiveRelease;
    EventLoop _baseLoop;
    Accepter _accepter;
    LoopThreadPool _threadPool;
    std::unordered_map<uint64_t, ptrConnection> _connections;

    connectedCallback _connectedCallback;
    messageCallback _messageCallback;
    closeCallback _closeCallback;
    eventCallback _eventCallback;
};