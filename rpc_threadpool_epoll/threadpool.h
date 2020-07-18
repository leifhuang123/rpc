#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <stdbool.h>
#include <pthread.h>

//#define THREADPOOL_WAIT

typedef void (*threadpool_process)(void *);

// 任务接口（节点）
typedef struct work
{
    threadpool_process process;
    void *arg;
    struct work *next;
} work_t;

// 线程池
typedef struct thread_pool
{
    // 任务队列
    struct work *head;
    // 线程数组指针
    pthread_t *ptid;
    // 条件变量和锁
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    // 线程池是否被销毁
    bool shutdown;
    // 线程池中线程总量
    int thread_num;
    // 当前任务队列中任务数量
    int work_num;
} threadpool_t;

threadpool_t *threadpool_create(int thread_num);

void threadpool_destroy(threadpool_t *pool);

work_t *threadpool_append(threadpool_t *pool, threadpool_process process, void *arg);

#endif