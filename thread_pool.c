#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#include "thread_pool.h"

/**
 *  @struct threadpool_task
 *  @brief the work struct
 *
 *  Feel free to make any modifications you want to the function prototypes and structs
 *
 *  @var function Pointer to the function that will perform the task.
 *  @var argument Argument to be passed to the function.
 */

typedef struct __task_t {
    void (*function)(void *);
    void *argument;
    struct __task_t *next;
} pool_task_t;


struct pool_t {
  pthread_mutex_t lock;
  pthread_cond_t notify;
  pthread_t *threads;
  pool_task_t *queue;
  int num_tasks;
  int thread_count;
  int task_queue_size_limit;
};

static void *thread_do_work(void *pool);


/*
 * Create a threadpool, initialize variables, etc
 *
 */
pool_t *pool_create(int queue_size, int num_threads)
{
    printf("Initializing threadpool\n");
    pool_t *thread_pool = malloc(sizeof(pool_t));

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    thread_pool->lock = mutex;

    pthread_cond_t cond;
    pthread_cond_init(&cond, NULL);
    thread_pool->notify = cond;

    thread_pool->threads = (pthread_t*)malloc(sizeof(pthread_t) * num_threads);
    int i;
    for (i=0;i<num_threads;i++) {
      pthread_create(&(thread_pool->threads[i]),
                     NULL,
                     thread_do_work,
                     (void*)thread_pool);
    }
    thread_pool->queue = NULL;
    thread_pool->thread_count = num_threads;
    thread_pool->task_queue_size_limit = queue_size;
    thread_pool->num_tasks = 0;

    return thread_pool;
}


/*
 * Add a task to the threadpool
 *
 */
int pool_add_task(pool_t *pool, void (*function)(void *), void *argument)
{
    int err = 0;
    int r = rand() % 100;

    pool_task_t* new_task = malloc(sizeof(pool_task_t*));
    new_task->function = function;
    new_task->argument = argument;
    new_task->next = NULL;

    printf("%d is waiting in add task\n", r);
    pthread_mutex_lock(&(pool->lock));
    printf("%d has the lock\n", r);

    if (pool->queue == NULL) {
        pool->queue = new_task;
    } else {
        pool_task_t* curr;

        curr = pool->queue;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = new_task;
    }

    printf("%d is giving up the lock\n", r);
    pthread_mutex_unlock(&(pool->lock));
    if (!err) {
      pool->num_tasks++;
      pthread_cond_signal(&(pool->notify));
    }

    printf("Successfully added to the queue!\n");
    return err;
}



/*
 * Destroy the threadpool, free all memory, destroy treads, etc
 *
 */
int pool_destroy(pool_t *pool)
{
    int err = 0;

    return err;
}



/*
 * Work loop for threads. Should be passed into the pthread_create() method.
 *
 */
static void *thread_do_work(void *pool)
{
    pool_t *tpool = (pool_t*)pool;
    while(1) {
      while (tpool->num_tasks == 0) {
        pthread_cond_wait(&(tpool->notify), &(tpool->lock));
        printf("Lock aquired in thread_do_work\n");
      }
        // we have the lock when we enter here, mostly needless check
      if (tpool->num_tasks != 0) {
        pool_task_t* next = tpool->queue->next;
        tpool->queue->function(tpool->queue->argument);
        tpool->num_tasks--;
        tpool->queue = next;
      }
      printf("Thread do work is reliquishing the lock\n");
      pthread_mutex_unlock(&tpool->lock);
    }

    pthread_exit(NULL);
    return(NULL);
}
