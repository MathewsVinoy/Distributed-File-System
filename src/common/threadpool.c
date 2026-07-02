#include "common/threadpool.h"
#include <stdlib.h>

static void *worker_loop(void *arg) {
    threadpool_t *pool = (threadpool_t *)arg;
    while (1) {
        pthread_mutex_lock(&pool->mutex);
        while (!pool->head && !pool->stop) {
            pthread_cond_wait(&pool->cond, &pool->mutex);
        }
        if (pool->stop) {
            pthread_mutex_unlock(&pool->mutex);
            break;
        }
        threadpool_task_t *task = pool->head;
        if (task) {
            pool->head = task->next;
            if (!pool->head) {
                pool->tail = NULL;
            }
        }
        pthread_mutex_unlock(&pool->mutex);

        if (task) {
            task->fn(task->arg);
            free(task);
        }
    }
    return NULL;
}

int threadpool_init(threadpool_t *pool, size_t worker_count) {
    if (!pool || worker_count == 0) {
        return -1;
    }
    pool->head = pool->tail = NULL;
    pool->worker_count = worker_count;
    pool->stop = 0;
    pthread_mutex_init(&pool->mutex, NULL);
    pthread_cond_init(&pool->cond, NULL);
    pool->workers = calloc(worker_count, sizeof(pthread_t));
    if (!pool->workers) {
        return -1;
    }
    for (size_t i = 0; i < worker_count; ++i) {
        if (pthread_create(&pool->workers[i], NULL, worker_loop, pool) != 0) {
            return -1;
        }
    }
    return 0;
}

int threadpool_submit(threadpool_t *pool, threadpool_task_fn fn, void *arg) {
    if (!pool || !fn) {
        return -1;
    }
    threadpool_task_t *task = malloc(sizeof(threadpool_task_t));
    if (!task) {
        return -1;
    }
    task->fn = fn;
    task->arg = arg;
    task->next = NULL;

    pthread_mutex_lock(&pool->mutex);
    if (!pool->head) {
        pool->head = pool->tail = task;
    } else {
        pool->tail->next = task;
        pool->tail = task;
    }
    pthread_cond_signal(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);
    return 0;
}

void threadpool_shutdown(threadpool_t *pool) {
    if (!pool) {
        return;
    }
    pthread_mutex_lock(&pool->mutex);
    pool->stop = 1;
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->mutex);

    for (size_t i = 0; i < pool->worker_count; ++i) {
        pthread_join(pool->workers[i], NULL);
    }

    free(pool->workers);

    threadpool_task_t *task = pool->head;
    while (task) {
        threadpool_task_t *next = task->next;
        free(task);
        task = next;
    }

    pthread_mutex_destroy(&pool->mutex);
    pthread_cond_destroy(&pool->cond);
}
