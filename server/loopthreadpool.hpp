#pragma once
#include <vector>
#include <memory>
#include <thread>
#include "eventloop.hpp"

class LoopThreadPool
{
public:
    explicit LoopThreadPool(EventLoop* baseLoop, int threadNum = std::thread::hardware_concurrency())
    : _threadNum(threadNum),
    _next(0),
    _baseLoop(baseLoop)
    {

    }
    void creat()
    {
        for(int i = 0; i < _threadNum; i++)
        {
            LoopThread* lt = new LoopThread();
            _threads.push_back(lt);
            _loops.push_back(lt->getLoop());
        }
    }
    EventLoop* getNextLoop()
    {
        EventLoop* loop = _baseLoop;
        if(!_loops.empty())
        {
            loop = _loops[_next];
            _next = (_next + 1) % _threadNum;
        }
        return loop;
    }
private:
    int _threadNum;
    int _next;
    EventLoop* _baseLoop;
    std::vector<LoopThread*> _threads;
    std::vector<EventLoop*> _loops;
};