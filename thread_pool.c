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
    int id;
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
int shutdown = 0;

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
int pool_add_task(pool_t *pool, void (*function)(void *), void *argument, int jobno)
{
    int rc;

    /* Lock the task queue before adding to it*/
    if (LDEBUG) printf("Waiting in add_task\n");
    rc = pthread_mutex_lock(&(pool->lock));
    if (!rc) {
      if (LDEBUG) {
        printf("Has the lock in add_task\n");
      }
    } else {
      printf("There was an error an error locking\n");
      return -1;
    }

    /* The new task's next ptr is NULL because we are
     * inserting it at the end of the list */
    pool_task_t* new_task = malloc(sizeof(pool_task_t));
    new_task->function = function;
    new_task->argument = argument;
    new_task->next = NULL;
    new_task->id = jobno;


    /* Iterate the queue, get to the end, put the new task on there
     * This is an inefficient queue, but seeing as the queue rarely exceeded 1-2
     * element, the O(n) time it takes to insert on the end is not significant
     */
    if (JDEBUG) printf("Adding job to the queue\n");
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

    /* Wake up threads to do this work, and give up the lock */
    pool->num_tasks++;
    if (LDEBUG) printf("Signaled that he added to the queue\n");
    pthread_cond_broadcast(&(pool->notify));
    rc = pthread_mutex_unlock(&(pool->lock));
    if (!rc) {
      if (LDEBUG) printf("Gave up the lock in add_task\n");
    } else {
      if (LDEBUG) printf("Error giving up the lock\n");
      return -1;
    }
    return 0;
}

/*
 * Destroy the threadpool, free all memory, destroy treads, etc
 *
 */
int pool_destroy(pool_t *pool)
{
    printf("Freeing resources\n");
    int i;
    shutdown = 1;
    /* Wake up all threads, have them exit, and join them */
    for(i=0; i < MAX_THREADS; i++) {
      pthread_cond_broadcast(&pool->notify);
      pthread_join(pool->threads[i], 0);
    }
    free(pool->threads);

    /* Free each job in the list */
    pool_task_t *job, *next;
    job = pool->queue;
    next = job->next;
    while (job != NULL) {
      free(job);
      job = next;
      next = job->next;
    }

    pthread_mutex_destroy(&pool->lock);
    pthread_cond_destroy(&pool->notify);
    free(pool);
    printf("Resources destroyed\n");
    return 0;
}

/*
 * Work loop for threads. Should be passed into the pthread_create() method.
 *
 */
static void *thread_do_work(void *pool)
{
    int rc;
    int r;
    r = rand() % 10000;
    pool_t *tpool = (pool_t*)pool;
    while(1) {
      pthread_mutex_lock(&(tpool->lock));
      while ((tpool->num_tasks == 0) && !shutdown) {
        if (LDEBUG) printf("%d [%d] is waiting for a signal\n", r, tpool->num_tasks);
        pthread_cond_wait(&tpool->notify, &tpool->lock);
        if (LDEBUG) printf("%d [%d] recieved a signal, acquired lock in thread_do_work\n", r, tpool->num_tasks);
      }
      if (shutdown) {
        if (LDEBUG) printf("Unlocking and exiting\n");
        pthread_mutex_unlock(&tpool->lock);
        pthread_exit(NULL);
      }
      if (tpool->num_tasks > 0) {
        pool_task_t* next = tpool->queue->next;
        if (JDEBUG) printf("Running job %d\n", tpool->queue->id);
        tpool->queue->function(tpool->queue->argument);
        tpool->queue = next;
        tpool->num_tasks--;
        rc = pthread_mutex_unlock(&tpool->lock);
        if (!rc) {
         if (LDEBUG) printf("%d [%d] gave up the lock in thread_do_work\n", r, tpool->num_tasks); fflush(stdout);
        }
        else printf("There was an error unlocking\n");
      } else {
        printf("A thread exited but didn't do anything\n");
      }
    }

    pthread_exit(NULL);
    return(NULL);
}
