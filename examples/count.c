#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 10
#define MAX 1000000000
#define VERBOSE 1

long int sumSq(long int max) {

  long int total = 0;

  for (long int i = 0; i < max; i++) {
    total += i;
  }
  return total;
}

void *worker(void *arg) {

  long int start = (long int)arg;
  long int end = start + (MAX / NUM_THREADS);
  long int total = 0;

  if (VERBOSE) printf("Summing %lu to %lu\n", start, end);
  for(int i=start; i < end; i++) {
    total += i;
  }

  return (void*)total;
}

int main(int argc, char **argv) {

  int threaded = atoi(argv[1]);
  if (!threaded) printf("Unthreaded version: %lu \n", sumSq(MAX));

  else {
    pthread_t threads[NUM_THREADS];
    for(long int i=0; i < NUM_THREADS; i++) {
      pthread_create(&threads[i], NULL, worker, (void *)(i * (MAX / NUM_THREADS)));
    }

    long int total = 0;
    void* val;
    for(int i=0; i < NUM_THREADS; i++) {
      pthread_join(threads[i], &val);
      if (VERBOSE) printf("Thread %d calculated %lu\n", i, (long int)val);
      total += (long int)val;
    }

    printf("Threaded version: %lu\n", total);

  }
  return 0;
}
