#pragma once
#include <thread>
#include <future>
#include <functional>
#include "safequeue.hpp"

class threadPool
{
public:
    threadPool(const int num_threads = 4) : m_threads(std::vector<std::thread>(num_threads)) ,m_shutdown(false)
    {
        
    }
    ~threadPool() 
    {

    }
    threadPool(const threadPool &) = delete;
    threadPool(threadPool &&) = delete;
    threadPool &operator=(const threadPool &) = delete;
    threadPool &operator=(threadPool &&) = delete;

    void init()
    {
        for(int i = 0; i < m_threads.size(); i++)
        {
            m_threads[i] = std::thread(threadWorker(this, i));
        }
    }
    void shutdown()
    {
        m_shutdown = true;
        m_cond.notify_all();
        for(int i = 0; i < m_threads.size(); i++)
        {
            if(m_threads[i].joinable())
            {
                m_threads[i].join();
            }
        }
    }
    template <typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
    {
        std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(std::move(func));
        std::function<void()> wrapper_func = [task_ptr]() {
            (*task_ptr)();
        };
        m_queue.push(wrapper_func);
        m_cond.notify_one();
        return task_ptr->get_future();
    }
private:
    class threadWorker
    {
    public:
        threadWorker(threadPool* pool, const int id) : m_pool(pool), m_id(id){}
        void operator()()
        {
            std::function<void()> func;
            bool dequeued = false;
            while(!m_pool->m_shutdown)
            {
                {
                    std::unique_lock<std::mutex> lock(m_pool->m_conditional_mutex);
                    if(m_pool->m_queue.empty())
                    {
                        m_pool->m_cond.wait(lock);
                    }
                    dequeued = m_pool->m_queue.pop(func);
                }
                if(dequeued)
                {
                    func();
                }
            }
        }
    private:
        int m_id;
        threadPool* m_pool;
    };
private:
    bool m_shutdown; // 线程池是否关闭
    safeQueue<std::function<void()>> m_queue; // 任务队列
    std::vector<std::thread> m_threads; // 线程池
    std::mutex m_conditional_mutex; // 保护任务队列的互斥锁
    std::condition_variable m_cond; // 条件变量
};