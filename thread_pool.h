#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#define MAX_THREADS 1
#define STANDBY_SIZE 8

#define JDEBUG 1
#define LDEBUG 0

typedef struct pool_t pool_t;

pool_t *pool_create(int thread_count, int queue_size);

int pool_add_task(pool_t *pool, void (*routine)(void *), void *arg, int id);

int pool_destroy(pool_t *pool);

#endif
