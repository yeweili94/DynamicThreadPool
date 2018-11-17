#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "threadpool.h"

void threadpool_init(threadpool_t* pool, int max_threads) {
    condition_init(&pool->ready);
    pool->first = NULL;
    pool->last = NULL;
    pool->max_threads = max_threads;
    pool->idle = 0;
    pool->counter = 0;
    pool->quit = 0;
}

void* thread_routine(void *arg) {
    printf("thread: 0x%x is starting\n", (int)pthread_self());
    threadpool_t *pool = (threadpool_t*)arg;
    struct timespec abstime;
    while (true) {
        //lock
        condition_lock(&pool->ready);
        pool->idle++;
        bool is_timeout = false;
        while (!pool->quit && pool->first == NULL) {
            printf("thread 0x%x is waitting\n", (int)pthread_self());
            clock_gettime(CLOCK_REALTIME, &abstime);
            abstime.tv_sec += 2;
            int status = condition_timedwait(&pool->ready, &abstime);
            if (status == ETIMEDOUT) {
                is_timeout = true;
                break;
            }
        }
        pool->idle--;

        if (pool->first != NULL) {
            task_t* task = pool->first;
            pool->first = task->next;
            //unlock
            condition_unlock(&pool->ready);
            task->run(task->arg);
            free(task);
            //lock
            condition_lock(&pool->ready);
        }

        //收到线程池销毁通知，且线程池中没有任务
        if (pool->quit && pool->first == NULL) {
            //unlock
            pool->counter--;
            if (pool->counter == 0) {
                condition_broadcast(&pool->ready);
            }
            condition_unlock(&pool->ready);
            break;
        }

        //超时
        if (is_timeout && pool->first == NULL) {
            printf("thread 0x%x is timeout\n", (int)pthread_self());
            pool->counter--;
            condition_unlock(&pool->ready);
            break;
        }
        //unlock
        condition_unlock(&pool->ready);
    }
    printf("thread 0x%x is exit\n", (int)pthread_self());
    return NULL;
}

void threadpool_add_task(threadpool_t* pool, void* (*run)(void *arg), void* arg) {
    task_t *newtask = (task_t*)malloc(sizeof(task_t));
    newtask->next = NULL;
    newtask->run = run;
    newtask->arg = arg;
    //lock
    condition_lock(&pool->ready);
    if (pool->first == NULL) {
        pool->first = newtask;
    } else {
        pool->last->next = newtask;
    }
    pool->last = newtask;
    
    if (pool->idle > 0) {
        condition_signal(&pool->ready);
    }
    else if (pool->counter < pool->max_threads) {
        pthread_t pid;
        pool->counter++;
        pthread_create(&pid, NULL, thread_routine, pool);
    }
    //unlock
    condition_unlock(&pool->ready);
}

void threadpool_destroy(threadpool_t* pool) {
    //lock
    condition_lock(&pool->ready);
    if (pool->quit == 1) {
        condition_unlock(&pool->ready);
        return;
    }
    pool->quit = 1;

    if (pool->counter > 0) {
        if (pool->idle > 0) {
            condition_broadcast(&pool->ready);
        }
        while (pool->counter > 0) {
            condition_wait(&pool->ready);
        }
    }
    //unlock
    condition_unlock(&pool->ready);
    condition_destroy(&pool->ready);
    printf("main thread is exit\n");
}

void* callback(void *arg) {
    printf("thread : 0x%x is woring on the task %d\n", (int)pthread_self(), *(int*)arg);
    sleep(1);
    free(arg);
    return NULL;
}

int main() {
    threadpool_t pool;
    threadpool_init(&pool, 5);
    int i;
    for (i = 0; i < 30; i++) {
        int* arg = (int*)malloc(sizeof(int));
        *arg = i;
        threadpool_add_task(&pool, callback, arg);
    }
    threadpool_destroy(&pool);
}
