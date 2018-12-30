#include "DynamicThreadPool.h"
#include "Thread.h"
#include <functional>
#include <boost/bind.hpp>
#include <iostream>

DynamicThreadPool::DynamicThread::DynamicThread(DynamicThreadPool* pool)
    : pool_(pool),
    thread_(std::bind(&DynamicThreadPool::DynamicThread::ThreadFunc, this))
{
    thread_.start();
}

DynamicThreadPool::DynamicThread::~DynamicThread()
{
    thread_.join();
}

void DynamicThreadPool::DynamicThread::ThreadFunc()
{
    pool_->ThreadFunc();
    //current threads exit in pool threadfunc, so it's dead
    std::unique_lock<std::mutex> lock(pool_->mutex_);
    pool_->loop_threads_--;
    pool_->dead_threads_.push_back(this);

    //it should let shutdown_cond_ know that can be exit now
    if (pool_->shutdown_ && pool_->loop_threads_ == 0) {
        pool_->shutdown_cond_.notify_one();
    }
}

DynamicThreadPool::DynamicThreadPool(int reserve_threads)
    : shutdown_(false),
      reserve_threads_(reserve_threads),
      loop_threads_(0),
      waitting_threads_(0)
{
    for (int i = 0; i < reserve_threads_; i++) {
        std::lock_guard<std::mutex> lock(mutex_);
        loop_threads_++;
        new DynamicThread(this);
    }
}

void DynamicThreadPool::ThreadFunc()
{
    for (;;) {
        //it should be noted that, break is present this thread prepare to dead
        //there are two condition to break : 1.waitting threads number is too much  
        //2.the threadpool is going to destroyed
        std::unique_lock<std::mutex> lock(mutex_);
        if (!shutdown_ && tasks_.empty()) {
            if (waitting_threads_ >= reserve_threads_) {
                break;
            }
            waitting_threads_++;
            cond_.wait(lock);
            waitting_threads_--;
        }

        if (!tasks_.empty()) {
            auto task = tasks_.front();
            tasks_.pop();
            lock.unlock();
            task();
        } else if (shutdown_) {
            break;
        }
    }
}

void DynamicThreadPool::Add(const std::function<void()>& callbacks)
{
    std::unique_lock<std::mutex> lock(mutex_);
    tasks_.push(callbacks);

    if (waitting_threads_ == 0) {
        loop_threads_++;
        new DynamicThread(this);
    } else {
        cond_.notify_one();
    }

    if (!dead_threads_.empty()) {
        std::list<DynamicThread*> tmplist;
        dead_threads_.swap(tmplist);
        lock.unlock();
        ReapThreads(&tmplist);
    }
}

DynamicThreadPool::~DynamicThreadPool()
{
    std::unique_lock<std::mutex> lock(mutex_);
    shutdown_ = true;
    cond_.notify_all();
    while (loop_threads_ != 0) {
        shutdown_cond_.wait(lock);
    }
    ReapThreads(&dead_threads_);
}

void DynamicThreadPool::ReapThreads(std::list<DynamicThread*>* tlist)
{
    for (auto it = tlist->begin(); it != tlist->end(); it = tlist->erase(it)) {
        delete *it;
    }
}

void work()
{
    std::cout << pthread_self() << "work" << std::endl;
    // sleep(1);
}

