#include "Thread.h"
#include <list>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>

class DynamicThreadPool : boost::noncopyable
{
public:
    //reference is ok, because we push it into queue, that must be copy, so it's safe
    explicit DynamicThreadPool(int reserve_threads);
    ~DynamicThreadPool();

    void Add(const std::function<void()>& callbacks);
private:
    class DynamicThread
    {
     public:
         DynamicThread(DynamicThreadPool* pool);
         ~DynamicThread();
     private:
         DynamicThreadPool* pool_;
         Thread thread_;
         void ThreadFunc();
    };
    std::mutex mutex_;
    std::condition_variable cond_;
    std::condition_variable shutdown_cond_;
    bool shutdown_;
    std::queue<std::function<void()>> tasks_;
    int reserve_threads_;
    int loop_threads_;
    int waitting_threads_;
    std::list<DynamicThread*> dead_threads_;

    void ThreadFunc();
    static void ReapThreads(std::list<DynamicThread*>* tlist);
};
