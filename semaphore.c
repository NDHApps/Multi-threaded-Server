#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>


typedef struct m_sem_t {
    pthread_mutex_t m;
    pthread_cond_t cv;
    unsigned int count;
} m_sem_t;

int sem_wait(m_sem_t *s);
int sem_post(m_sem_t *s);

int sem_init(m_sem_t *s, unsigned int value) {
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    s->m = mutex;
    
    pthread_cond_t condvar;
    pthread_cond_init(&condvar, NULL);
    s->cv = condvar;
    
    s->count = value;
    
    return 0;
}

int sem_wait(m_sem_t *s)
{
    pthread_mutex_lock(&(s->m));
    while (s->count == 0) {
        pthread_cond_wait(&(s->cv), &(s->m));
    }
    (s->count)--;
    pthread_mutex_unlock(&(s->m));
    return 0;
}

int sem_post(m_sem_t *s)
{
    pthread_mutex_lock(&(s->m));
    (s->count)++;
    pthread_cond_broadcast(&(s->cv));
    pthread_mutex_unlock(&(s->m));
    return 0;
}
