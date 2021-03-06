#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#define MAX_THREADS 20
#define STANDBY_SIZE 8

#define JDEBUG 0
#define LDEBUG 0

typedef struct pool_t pool_t;

pool_t *pool_create(int thread_count, int queue_size);

int pool_add_task(pool_t *pool, void (*routine)(void *), void *arg, int id, int priority);

int pool_destroy(pool_t *pool);

#endif
