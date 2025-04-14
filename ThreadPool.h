#pragma once
// File: include/ThreadPool.h
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>                 // 用于存储线程对象
#include <queue>                  // 用于存储待执行的任务
#include <thread>                 // 提供了线程相关的功能
#include <mutex>                  // 用于实现线程同步，防止多个线程同时访问共享资源
#include <condition_variable>     // 用于线程间的等待和通知机制
#include <functional>             // 用于处理函数对象，方便存储和执行任务
#include <stdexcept>              // 用于抛出异常

using std::vector;
using std::queue;
using std::thread;
using std::mutex;
using std::condition_variable;
using std::function;
using std::unique_lock;
using std::runtime_error;

class ThreadPool
{
public:
    // 构造函数，explicit 关键字防止隐式类型转换。num_threads 是线程池中的线程数量，默认值为 4
    explicit ThreadPool(size_t num_threads = 4)
    {
        // for 循环：创建 num_threads 个线程并添加到 workers_ 向量中
        for (size_t i = 0; i < num_threads; ++i)
        {
            // 使用 lambda 表达式创建线程的执行体
            workers_.emplace_back([this]
                {
                    // 线程进入无限循环，持续等待任务
                    while (true)
                    {
                        // 定义一个无参数、无返回值的函数对象，用于存储待执行的任务
                        function<void()> task;
                        {
                            // 使用 unique_lock 对 queue_mutex_ 加锁，确保线程安全
                            unique_lock<mutex> lock(queue_mutex_);
                            // 线程进入等待状态，直到 stop_ 为 true 或者任务队列不为空
                            condition_.wait(lock, [this] {
                                return stop_ || !tasks_.empty();
                                });

                            //如果线程池停止且任务队列为空，线程退出循环
                            if (stop_ && tasks_.empty()) return;

                            // 从任务队列中取出一个任务并移除
                            task = move(tasks_.front());
                            tasks_.pop();
                        }
                        // 执行任务
                        task();
                    }
                });
        }
    }

    // 析构函数，用于释放线程池资源
    ~ThreadPool()
    {
        // 加锁并将 stop_ 标志设置为 true，表示线程池停止
        {
            unique_lock<mutex> lock(queue_mutex_);
            stop_ = true;
        }
        // 通知所有等待的线程，线程池即将停止
        condition_.notify_all();
        // for 循环：遍历所有线程，若线程可连接（joinable()），则调用 join() 等待线程结束
        for (auto& worker : workers_) {
            if (worker.joinable()) worker.join();
        }
    }

    // 定义一个模板函数，F 是任务的类型
    template<class F>
    // 将任务加入任务队列
    void enqueue(F&& task)
    {
        // 加锁，确保线程安全
        {
            unique_lock<mutex> lock(queue_mutex_);
            // 如果线程池已停止，抛出异常
            if (stop_)
                throw runtime_error("enqueue on stopped ThreadPool");
            // 将任务添加到任务队列中
            tasks_.emplace(forward<F>(task));
        }
        // 通知一个等待的线程有新任务可用
        condition_.notify_one();
    }

private:
    vector<thread> workers_;             // 存储线程池中的所有线程
    queue<function<void()>> tasks_;      // 存储待执行的任务
    mutex queue_mutex_;                  // 用于保护任务队列的互斥锁
    condition_variable condition_;       // 用于线程间的等待和通知机制
    bool stop_ = false;                  // 表示线程池是否停止的标志，初始值为 false
};

#endif

// 这段代码实现了一个简单的线程池类 ThreadPool。线程池在构造时创建指定数量的线程，这些线程会不断从任务队列中取出任务并执行。用户可以通过 enqueue 函数将任务添加到任务队列中。在析构时，线程池会停止所有线程并等待它们结束。线程池使用互斥锁和条件变量来确保线程安全和任务的正确调度。