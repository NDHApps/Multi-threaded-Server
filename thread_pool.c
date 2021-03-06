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
    int priority;
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
int pool_add_task(pool_t *pool, void (*function)(void *), void *argument, int jobno, int priority)
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
    new_task->priority = priority;

    /* Iterate the threadpool and insert yourself in front of the first element
     * you see with lower priority than you, this puts you at the end of the
     * list of elements with the same priority as you */
    if (JDEBUG) printf("Adding job to the queue\n");
    if (pool->queue == NULL) {
      /* Inserting in an empty queue */
      pool->queue = new_task;
    } else {
      pool_task_t *curr, *prev;
      curr = pool->queue;
      prev = NULL;
      while ((curr->next != NULL) && (priority <= curr->next->priority)) {
        prev = curr;
        curr = curr->next;
      }
      if (prev == NULL) {
        /* Inserting at the front of the queue */
        new_task->next = curr;
        pool->queue = new_task;
      } else if (curr->next == NULL) {
        /* Inserting at the end */
        curr->next = new_task;
      }else {
        /* Inserting in the middle */
        new_task->next = curr->next;
        curr->next = new_task;
      }
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
    pool_t *tpool = (pool_t*)pool;

    while(1) {

      pthread_mutex_lock(&(tpool->lock));
      /* Wait in a while loop, on the condition we are not shutting down and
       * that there is work to be done.  */
      while ((tpool->num_tasks == 0) && !shutdown) {
        if (LDEBUG) printf("Waiting for a signal\n");
        pthread_cond_wait(&tpool->notify, &tpool->lock);
        if (LDEBUG) printf("Recieved a signal, acquired lock in thread_do_work\n");
      }

      /* If we're shutting down, have the thread that got through give up the
       * lock and exit, so the join will terminate */
      if (shutdown) {
        if (LDEBUG) printf("Unlocking and exiting\n");
        pthread_mutex_unlock(&tpool->lock);
        pthread_exit(NULL);
      }

      /* Pop an element off the front of queue, run its function, and give up the lock */
      pool_task_t *job_to_run = tpool->queue;
      pool_task_t *next = job_to_run->next;
      if (JDEBUG) printf("Running job %d\n", tpool->queue->id);
      tpool->queue = next;
      tpool->num_tasks--;
      rc = pthread_mutex_unlock(&tpool->lock);

      /* The seats code is threadsafe, so it's okay to give up the lock before
       * we run the function itself */

      job_to_run->function(job_to_run->argument);

      if (!rc) {
       if (LDEBUG) printf("Gave up the lock in thread_do_work\n");
      }
      else printf("There was an error unlocking\n");

    }
    pthread_exit(NULL);
    return(NULL);
}
