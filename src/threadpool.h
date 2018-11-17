#ifndef PTHREAD_POOL
#define PTHREAD_POOL

#include <pthread.h>
#include <time.h>
#include <unistd.h>

/****************任务结构体*********************/
typedef struct task_t
{
    void* (*run)(void *arg);
    void* arg;
    struct task_t* next;
} task_t;

/****************条件变量封装*******************/
typedef struct condition_t
{
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
} condition_t;

/****************线程池结构体*******************/
typedef struct threadpool_t
{
    condition_t ready;
    int max_threads;    //工作最大线程数
    int idle;   //等待的线程数
    int counter;    //目前线程数
    task_t* first;
    task_t* last;
    int quit;
} threadpool_t;

/**************条件变量封装接口****************/
int condition_lock(condition_t* cond) {
    return pthread_mutex_lock(&cond->_mutex);
}
int condition_unlock(condition_t* cond) {
    return pthread_mutex_unlock(&cond->_mutex);
}
int condition_wait(condition_t* cond) {
    return pthread_cond_wait(&cond->_cond, &cond->_mutex);
}
int condition_timedwait(condition_t* cond, struct timespec* abstime) {
    return pthread_cond_timedwait(&cond->_cond, &cond->_mutex, abstime);
}
int condition_signal(condition_t* cond) {
    return pthread_cond_signal(&cond->_cond);
}
int condition_broadcast(condition_t* cond) {
    return pthread_cond_broadcast(&cond->_cond);
}
int condition_destroy(condition_t* cond) {
    int status = 0;
    if ((status = pthread_mutex_destroy(&cond->_mutex)) != 0) {
        return status;
    }
    if ((status = pthread_cond_destroy(&cond->_cond)) != 0) {
        return status;
    }
    return status;
}
int condition_init(condition_t* cond) {
    int status = 0;
    if ((status = pthread_mutex_init(&cond->_mutex, NULL)) != 0) {
        return status;
    }
    if ((status = pthread_cond_init(&cond->_cond, NULL)) != 0) {
        return status;
    }
    return status;
}

/***************线程池接口*****************/
//初始化线程池
void threadpool_init(threadpool_t* pool, int max_threads);
//往线程池中添加任务
void threadpool_add_task(threadpool_t *pool, void* (*run)(void *arg), void *arg);
//销毁线程池
void threadpool_destroy(threadpool_t *pool);




#endif
