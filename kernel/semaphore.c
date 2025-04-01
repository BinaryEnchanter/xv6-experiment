#include "types.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "sleeplock.h"
#include "semaphore.h"


// to solve implicit declaration
// void initsleeplock(struct sleeplock *lk, char *name);
// void acquiresleeplock(struct sleeplock *lk);
// void releasesleeplock(struct sleeplock *lk);
// int holdingsleeplock(struct sleeplock *lk);


void sem_init(struct semaphore *s, int init) {
    s->C = init;
    initsleeplock(&s->S1, "S1");
    initsleeplock(&s->S2, "S2");
    acquiresleep(&s->S2);  // S2 is locked initially
}

// wait aka P
void sem_wait(struct semaphore *s) {
    acquiresleep(&s->S1);// gaurantee of atomic op
    s->C--;
    if (s->C < 0) {// no more free resourse
        releasesleep(&s->S1);// allow other to P
        acquiresleep(&s->S2);// block cur proc
        //releasesleep(&s->S2);
    } 
        releasesleep(&s->S1);// keep going
    
}

// signal aka V
void sem_signal(struct semaphore *s) {
    acquiresleep(&s->S1);
    s->C++;
    if (s->C <= 0) {// still have proc waiting for res
        releasesleep(&s->S2);// wakeup the waiting res
    }
    releasesleep(&s->S1);
}
