#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "semaphore.h"

#define MAX_SEMAPHORES 16

struct semaphore semaphores[MAX_SEMAPHORES];

uint64 sys_sem_init(void) {
    int index, init;
    argint(0, &index);
    argint(1, &init);

    if (index < 0 || index >= MAX_SEMAPHORES) return -1;

    sem_init(&semaphores[index], init);
    return 0;
}

uint64 sys_sem_wait(void) {
    int index;
    argint(0, &index);

    if (index < 0 || index >= MAX_SEMAPHORES) return -1;

    sem_wait(&semaphores[index]);
    return 0;
}

uint64 sys_sem_signal(void) {
    int index;
    argint(0, &index);

    if (index < 0 || index >= MAX_SEMAPHORES) return -1;

    sem_signal(&semaphores[index]);
    return 0;
}

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_procnum(void)
{
  uint64 proc_num;
  proc_num=procnum();
  return proc_num;
}