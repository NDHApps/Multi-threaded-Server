#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_THREADS 10
long count = 0;
pthread_mutex_t lock;
pthread_cond_t cond;

void* worker(void *arg) {
  long n = (long)arg;
  pthread_mutex_lock(&lock);
  while( count != n )
    pthread_cond_wait(&cond, &lock);
  printf("Worker called with %lu\n", n);
  sleep(1);
  count+=1;
  pthread_cond_signal(&cond);
  pthread_mutex_unlock(&lock);
  pthread_exit(0);
}


int main(int argc, char **argv) {
  pthread_t threads[NUM_THREADS];

  pthread_mutex_init(&lock, NULL);
  pthread_cond_init(&cond, NULL);

  for(long i=0;i<NUM_THREADS;i++) {
    pthread_create(&threads[i], NULL, worker, (void*)i);
  }

  for(long i=0;i<NUM_THREADS;i++) {
    pthread_join(threads[i], NULL);
  }
  printf("Final count was %lu\n", count);
  pthread_mutex_destroy(&lock);
  return 0;
}
