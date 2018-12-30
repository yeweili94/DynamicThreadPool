#ifndef THREAD_POOL_THREAD_H
#define THREAD_POOL_THREAD_H

#include <pthread.h>
#include <functional>

#include <boost/noncopyable.hpp>

class Thread 
{
public:
    using ThreadFunc = std::function<void()>;

    Thread(const ThreadFunc& callback);
    ~Thread();
    void start();
    void join();
private:
    pthread_t threadId_;
    static void* ThreadRoutine(void* arg);
    ThreadFunc func_;
};

void* Thread::ThreadRoutine(void* arg) {
    Thread* th = static_cast<Thread*>(arg);
    th->func_();
    return NULL;
}

Thread::Thread(const ThreadFunc& callback)
    : func_(callback)
{
}

Thread::~Thread()
{
}

void Thread::start()
{
    pthread_create(&threadId_, NULL, ThreadRoutine, this);
}

void Thread::join()
{
    pthread_join(threadId_, NULL);
}

#endif
