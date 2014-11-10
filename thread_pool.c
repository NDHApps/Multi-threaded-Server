#include <stdlib.h>
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
    pool_t *thread_pool = malloc(sizeof(pool_t));
    
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    thread_pool->lock = mutex;
    
    pthread_cond_t cond;
    pthread_cond_init(&cond, NULL);
    thread_pool->notify = cond;
    
    thread_pool->threads = malloc(sizeof(pthread_t) * num_threads);
    
    thread_pool->queue = NULL;
    
    thread_pool->thread_count = num_threads;
    
    thread_pool->task_queue_size_limit = queue_size;

    return NULL;
}


/*
 * Add a task to the threadpool
 *
 */
int pool_add_task(pool_t *pool, void (*function)(void *), void *argument)
{
    int err = 0;
    
    pool_task_t* new_task;
    new_task->function = function;
    new_task->argument = argument;
    new_task->next = NULL;
    
    pthread_mutex_lock(&(pool->lock));
    
    if (pool->queue == NULL) {
        pool->queue = new_task;
    } else {
        pool_task_t* curr;
        curr = pool->queue;
        int queue_index = 1;
        while (curr->next != NULL) {
            curr = curr->next;
            queue_index++;
        }
        if (queue_index < pool->task_queue_size_limit)
            curr->next = new_task;
        else
            err = -1;
    }
    
    pthread_mutex_unlock(&(pool->lock));
    
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

    while(1) {
        
    }

    pthread_exit(NULL);
    return(NULL);
}
