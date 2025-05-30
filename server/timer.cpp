#include "timer.hpp"
#include "eventloop.hpp"

void TimerWheel::addTask(uint64_t id, uint64_t timeout, const TimerTask::TaskFunc& task)
{
    _loop->runInLoop([this, id, timeout, task](){
        _addTask(id, timeout, task);
    });
}
void TimerWheel::refreshTask(uint64_t id)
{
    _loop->runInLoop([this, id](){
        _refreshTask(id);
    });
}
void TimerWheel::removeTask(uint64_t id)
{
    _loop->runInLoop([this, id](){
        _removeTask(id);
    });
}