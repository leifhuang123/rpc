#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include "threadpool.h"

static void *worker(void *arg)
{
    threadpool_t *pool = (threadpool_t *)arg;
    assert(pool != NULL);
    while (!pool->shutdown)
    {
        pthread_mutex_lock(&pool->mutex);
        while (pool->work_num == 0)
        {
            pthread_cond_wait(&pool->cond, &pool->mutex);
        }
        pool->work_num--;
        work_t *pwork = pool->head;
        assert(pwork != NULL);
        pool->head = pwork->next;

        pthread_mutex_unlock(&pool->mutex);
        pwork->process(pwork->arg);
        free(pwork);
        pwork = NULL;
    }
    return NULL;
}

threadpool_t *threadpool_create(int thread_num)
{
    threadpool_t *pool = (threadpool_t *)malloc(sizeof(threadpool_t));
    assert(pool != NULL);

    pool->thread_num = thread_num;
    pool->work_num = 0;
    pool->shutdown = false;
    pool->head = NULL;
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond, NULL);
    pool->ptid = (pthread_t *)malloc(sizeof(pthread_t) * thread_num);
    if (pool->ptid == NULL)
    {
        perror("malloc failed");
        goto err;
    }
    int i;
    for (i = 0; i < thread_num; ++i)
    {
        if (pthread_create(&pool->ptid[i], NULL, worker, pool) != 0)
        {
            perror("pthread_create failed");
            goto err2;
        }
        if (pthread_detach(pool->ptid[i]) != 0)
        {
            perror("pthread_detach failed");
            goto err2;
        }
    }
    return pool;
err2:
    free(pool->ptid);
err:
    free(pool);
    return NULL;
}

void threadpool_destroy(threadpool_t *pool)
{
    if (pool->shutdown)
        return;

    pool->shutdown = true;
    free(pool->ptid);

    work_t *p = NULL;
    while (pool->head != NULL)
    {
        p = pool->head;
        pool->head = p->next;
        free(p);
    }
    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);
}

work_t *threadpool_append(threadpool_t *pool, threadpool_process process, void *arg)
{
    work_t *new_work = (work_t *)malloc(sizeof(work_t));
    assert(new_work != NULL);
    new_work->next = NULL;
    new_work->process = process;
    new_work->arg = arg;

    pthread_mutex_lock(&pool->mutex);
    work_t *p = pool->head;
    if (p == NULL)
    {
        pool->head = new_work;
    }
    else
    {
        while (p->next != NULL)
        {
            p = p->next;
        }
        p->next = new_work;
    }
    pool->work_num++;
    pthread_mutex_unlock(&pool->mutex);
    pthread_cond_signal(&pool->cond);

    return new_work;
}
