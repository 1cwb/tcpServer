#pragma once
#include <functional>
#include <thread>
#include <queue>
#include <sys/eventfd.h>
#include <mutex>
#include <condition_variable>
#include "channel.hpp"
#include "poller.hpp"
#include "timer.hpp"

class EventLoop
{
public:
    using callback_t = std::function<void()>;
    EventLoop() : _eventFd(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)),
    _tid(std::this_thread::get_id()),
    _eventch(new Channel(_eventFd, this)),
    _timeWheel(this)
    {
        if(_eventFd < 0)
        {
            perror("eventfd");
            exit(1);
        }
        _eventch->setReadCallback([this](){readEventfd();});
        _eventch->enableRead();
    }

    ~EventLoop()
    {
        delete _eventch;
        close(_eventFd);
    }
    void runInLoop(const callback_t& cb)
    {
        if(isInLoopThread())
        {
            cb();
        }
        else
        {
            queueInLoop(cb);
        }
    }
    void queueInLoop(const callback_t& cb)
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _pending.push(cb);
        }
        wakeup();
    }
    bool isInLoopThread() const
    {
        return _tid == std::this_thread::get_id();
    }
    void updateEvent(Channel* ch)
    {
        _poller.update(ch);
    }
    void removeEvent(Channel* ch)
    {
        _poller.remove(ch);
    }
    void runAfter(uint64_t id, uint64_t timeout, const TimerTask::TaskFunc& task)
    {
        _timeWheel.addTask(id, timeout, task);
    }
    void refreshAfter(uint64_t id)
    {
        _timeWheel.refreshTask(id);
    }
    void removeAfter(uint64_t id)
    {
        _timeWheel.removeTask(id);
    }
    bool hasAfter(uint64_t id)
    {
        return _timeWheel.hasTask(id);
    }
    void start()
    {
        while(1)
        {
            std::vector<Channel*> activeChannels;
            _poller.poll(activeChannels);
            for(auto& ch : activeChannels)
            {
                ch->handleEvent();
            }
            runPendingTasks();
        }
    }
private:
    void readEventfd()
    {
        uint64_t res;
        ssize_t n = ::read(_eventFd, &res, sizeof(res));
        if(n != sizeof(res))
        {
            if(errno == EAGAIN || errno == EINTR)
            {
                return;
            }
            else
            {
                perror("read eventfd");
                exit(1);
            }
        }
    }
    void wakeup()
    {
        uint64_t one = 1;
        ssize_t n = ::write(_eventFd, &one, sizeof(one));
        if(n!= sizeof(one))
        {
            perror("write eventfd");
            exit(1);
        }
    }
    void runPendingTasks()
    {
        std::queue<callback_t> tasks;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            tasks.swap(_pending);
        }
        while(!tasks.empty())
        {
            auto task = tasks.front();
            tasks.pop();
            task();
        }
    }
private:
    int _eventFd;
    std::thread::id _tid;
    Channel* _eventch;
    Poller _poller;
    TimerWheel _timeWheel;
    std::queue<callback_t> _pending;
    std::mutex _mutex;
};

class LoopThread
{
public:
    LoopThread() : _thread(std::thread([this] { threadEntry(); })), _loop(nullptr){}
    ~LoopThread()
    {
        if(_thread.joinable())
        {
            _thread.join();
        }
    }
    EventLoop* getLoop()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _cond.wait(lock, [this] { return _loop != nullptr; });
        return _loop;
    }
private:
    void threadEntry()
    {
        EventLoop loop;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _loop = &loop;
            _cond.notify_all();
        }
        loop.start();
    }
private:
    std::thread _thread;
    EventLoop* _loop;
    std::mutex _mutex;
    std::condition_variable _cond;
};