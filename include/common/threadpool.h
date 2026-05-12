#ifndef DFS_THREADPOOL_H
#define DFS_THREADPOOL_H

#include <stddef.h>
#include <pthread.h>

typedef void (*threadpool_task_fn)(void *arg);

typedef struct threadpool_task {
    threadpool_task_fn fn;
    void *arg;
    struct threadpool_task *next;
} threadpool_task_t;

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    threadpool_task_t *head;
    threadpool_task_t *tail;
    pthread_t *workers;
    size_t worker_count;
    int stop;
} threadpool_t;

int threadpool_init(threadpool_t *pool, size_t worker_count);
int threadpool_submit(threadpool_t *pool, threadpool_task_fn fn, void *arg);
void threadpool_shutdown(threadpool_t *pool);

#endif
