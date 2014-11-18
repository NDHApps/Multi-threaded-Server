//
//  semaphore.h
//  
//
//  Created by Nicholas Hall on 11/18/14.
//
//

#ifndef _semaphore_h
#define _semaphore_h

#include <pthread.h>

typedef struct m_sem_t {
    pthread_mutex_t m;
    pthread_cond_t cv;
    unsigned int count;
} m_sem_t;

int sem_wait(m_sem_t *s);
int sem_post(m_sem_t *s);

int sem_init(m_sem_t *s, unsigned int value);

#endif
