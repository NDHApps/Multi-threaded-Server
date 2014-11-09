#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define NUM_THREADS 4

typedef struct _worker_t {
  int id;
  int busy;
  pthread_t *pthread;
  struct _worker_t *next;
} worker_t;

typedef struct _num_t {
  long n;
  int tid;
} num_t;

pthread_t threads[NUM_THREADS];
worker_t *pool = NULL;

void init_pool() {
  int i;
  for (i=0; i < NUM_THREADS; i++) {
    worker_t *t = malloc(sizeof(worker_t));
    t->busy=0;
    t->pthread = &threads[i];
    t->next = pool;
    t->id = i;
    pool = t;
  }
}

worker_t *find_worker_by_id(int id) {
  worker_t *curr = pool;
  while (curr != NULL) {
    if (curr->id == id) return curr;
    else curr = curr->next;
  }
  return curr;
}

worker_t* get_free_thread() {
  worker_t *curr = pool;
  while (curr != NULL) {
    if (curr->busy) curr = curr->next;
    else {
      return curr;
    }
  }
  return 0;
}

void* worker(void *arg) {
  num_t *nt = (num_t*)arg;
  long num = (long)nt->n;
  worker_t *t = find_worker_by_id(nt->tid);
  printf("\nThread %d is sleeping for %lu\n", t->id, num);
  fflush(stdout);
  sleep(num);
  printf("\nThread %d is done sleeping for %lu\n", t->id, num);
  fflush(stdout);
  putchar('>');
  fflush(stdout);
  t->busy = 0;
  pthread_exit(0);
}

int main(int argc, const char** argv) {
  long num;
  int rc = 0;
  char buf[10];
  num_t *nt = malloc(sizeof(nt));
  worker_t *thread;
  init_pool();

  while (1) {
    usleep(500);
    putchar('>');
    fflush(stdout);
    fgets(buf, 10, stdin);
    sscanf(buf, "%lu", &num);

    while ((thread = get_free_thread()) == 0) {
      printf("\nCould not find free thread\n");
      fflush(stdout);
      sleep(2);
    }

    thread->busy = 1;
    nt->n = num;
    nt->tid = thread->id;
    pthread_create(thread->pthread, NULL, worker, (void*)nt);
    if (rc) {
      perror("Error creating thread!\n");
      exit(1);
    }
  }

  return 0;
}
