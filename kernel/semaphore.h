#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "sleeplock.h"

struct semaphore {
    int C;              
    struct sleeplock S1; // as mutes,protector of C
    struct sleeplock S2; // as block()
};

void sem_init(struct semaphore *s, int init);
void sem_wait(struct semaphore *s);
void sem_signal(struct semaphore *s);

#endif