#pragma once
#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>
#include <sys/timerfd.h>
#include "channel.hpp"

class EventLoop;
class TimerTask
{
public:
    using TaskFunc = std::function<void()>;
    using ReleaseFunc = std::function<void()>;

    TimerTask(uint64_t timeout, TaskFunc task):_timeout(timeout), _task(task){}
    ~TimerTask(){if(!_canceled) _task(); _release();}
    void setRelease(const ReleaseFunc& release) {_release = release;}
    void cancel() {_canceled = true;}
    uint64_t timeout() const {return _timeout;}
private:
    uint64_t _timeout;
    TaskFunc _task;
    ReleaseFunc _release;
    bool _canceled = false;
};

class TimerWheel
{
public:
    explicit TimerWheel(EventLoop* loop)
    : _tick(0)
    , _wheelSize(60)
    , _timerfd(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC))
    , _loop(loop)
    ,_channel(new Channel(_timerfd, _loop))
    {
        _wheel.resize(_wheelSize);
        if(_timerfd == -1)
        {
            perror("timerfd_create");
            exit(EXIT_FAILURE);
        }
        itimerspec ts {};
        ts.it_value.tv_sec = 1;
        ts.it_value.tv_nsec = 0;
        ts.it_interval.tv_sec = 1;
        ts.it_interval.tv_nsec = 0;
        if(timerfd_settime(_timerfd, 0, &ts, nullptr) == -1)
        {
            perror("timerfd_settime");
            exit(EXIT_FAILURE);
        }
        _channel->setReadCallback([this](){onTime();});
        _channel->enableRead();
    }
    ~TimerWheel()
    {
        _channel->disableAll();
        _channel->remove();
        close(_timerfd);
    }
    void addTask(uint64_t id, uint64_t timeout, const TimerTask::TaskFunc& task);
    void refreshTask(uint64_t id);
    void removeTask(uint64_t id);
    bool hasTask(uint64_t id)
    {
        return _taskMap.find(id) != _taskMap.end();
    }
private:
    using TaskPtr = std::shared_ptr<TimerTask>;
    using TaskWeakPtr = std::weak_ptr<TimerTask>;

    void tick () 
    {
        _tick = (_tick + 1) % _wheelSize;
        _wheel[_tick].clear();
    }
    bool onTime()
    {
        uint64_t res;
        ssize_t n = read(_timerfd, &res, sizeof(res));
        if(n != sizeof(res))
        {
            if(errno == EAGAIN || errno == EINTR)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        while (res --)
        {
            tick();
        }
        return true;
    }
    void _addTask(uint64_t id, uint64_t timeout, const TimerTask::TaskFunc& task)
    {
        TaskPtr pt(new TimerTask(timeout, task));
        pt->setRelease([this, id](){_taskMap.erase(id);});
        _taskMap[id] = TaskWeakPtr(pt);
        uint64_t index = (_tick + timeout) % _wheelSize;
        _wheel[index].push_back(pt);
    }
    void _refreshTask(uint64_t id)
    {
        auto it = _taskMap.find(id);
        if(it != _taskMap.end())
        {
            TaskPtr pt = it->second.lock();
            if(pt)
            {
                uint64_t index = (_tick + pt->timeout()) % _wheelSize;
                _wheel[index].push_back(pt);
            }
        }
    }
    void _removeTask(uint64_t id)
    {
        auto it = _taskMap.find(id);
        if(it!= _taskMap.end())
        {
            TaskPtr pt = it->second.lock();
            if(pt)
            {
                pt->cancel();
            }
            _taskMap.erase(id);
        }
    }
private:
    std::vector<std::vector<TaskPtr>> _wheel;
    std::unordered_map<uint64_t, TaskWeakPtr> _taskMap;
    size_t _tick;
    size_t _wheelSize;
    int _timerfd;
    EventLoop* _loop;
    std::unique_ptr<Channel> _channel;
};
