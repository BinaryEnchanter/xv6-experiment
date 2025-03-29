#include "kernel/types.h"
#include "user/user.h"
#define NUM_PROC 4
int main(int argc, char *argv[])
{
  printf("Entering Malloc test...\n");
  char buf[10];
  buf[0]='r';
  
  while(buf[0]!=0){
    
    int n=read(0, buf, sizeof(buf) - 1);
    if(n!=0&&buf[0]!='d')
    malloctest(buf[0]);
    else if (n!=0&&buf[0]=='d')
    {
      int father = getpid();
      for (int i = 0; i < NUM_PROC; i++) {
        if (father == getpid()) { 
          int pid = fork();
          if (pid < 0) {
            printf("Fork failed!\n");
            exit(1);
          }
        }
      }

      if (getpid() != father) {
        printf("Child PID: %d running malloctest('c')\n", getpid());
        malloctest('c');
        exit(0);  
      }
      else {
        int state;
        for (int i = 0; i < NUM_PROC; i++) {
            wait(&state);
        }
        printf("All children finished, running malloctest('r')\n");
        malloctest('r');
      }
    }else if (buf[0]==0)
    {
      exit(0);
    }
  }
  exit(0);
}