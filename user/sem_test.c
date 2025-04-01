#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define CUSTOMER 0
#define BARBER 1
#define MUTEX 2
int p[2];// pipe for shared variable "int waiting"
int chairs=5;
  

void customer(int id) {
    sem_wait(MUTEX); // entering critic zone 

    int w;
    read(p[0], &w, sizeof(int));

    if(w<chairs){
      //sleep(10);
      w+=1;
      write(p[1], &w, sizeof(int));
      printf("Customer [%d] is seated. Now %d is waiting\n",id,w);
      
      sleep(1);// time for printf, otherwise msg mix
      sem_signal(CUSTOMER);// tell barber new customer
      sem_signal(MUTEX);// exit critic zone
      sem_wait(BARBER);//
      //printf("Customer [%d] have done haircut,leaving. Now %d is waiting\n",id,w);
    }
    else{
      //sleep(10);
      printf("Customer [%d] leave for no free chair\n",id);
      write(p[1], &w, sizeof(int));
      sem_signal(MUTEX);//leave
      return;
    }
}

void barber() {
  int severd_num=0;
  int w;
    while (1) {
      // barber sleep until a customer
      printf("Barber is sleeping.\n");
      sem_wait(CUSTOMER); // wait customer
      printf("Barber is waken.\n");
      sem_wait(MUTEX); // entering critic zone

      read(p[0], &w, sizeof(int)); 
      w--;
      write(p[1], &w, sizeof(int));

      sem_signal(BARBER);// ready for cut
      sem_signal(MUTEX); //exit critic zone

      printf("Barber is cutting hair.\n");
      sleep(10); // haircutting
      severd_num+=1;
      printf("Barber finished cutting hair. Having cut %d person\n",severd_num);
      
    }
}

int main() {
  sem_init(0, 0);// num of customers
  sem_init(1, 0);// initial barber state is 0 aka sleeping
  sem_init(2, 1);// mutex
   
  pipe(p); 
  int w = 0;
  write(p[1], &w, sizeof(int));

    int pid;
    int father=getpid();
    if ((pid = fork()) == 0) {
        barber();
        exit(0);
    }
    // customers
    for (int i = 0; i < 10; i++) {
      if (father==getpid()) {
        fork();
        //sleep(5);
      }
    }
    if (father!=getpid()&&pid!=0){
      customer(getpid());
      exit(0);
    }else{
      for (int i = 0; i < 10; i++) {
        wait(0);
    }
    }

    wait(0);

    exit(0);
}
