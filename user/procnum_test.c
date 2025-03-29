#include "kernel/types.h"
#include "user/user.h"

int main(int argc,char *argv[])
{
    int v=procnum();

    printf("API & system call returned, the return value is %d.\n", v);
    int pid=fork();
    if (pid==0){
        printf("API & system call returned, the return value is %d.\n",procnum());
        exit(0);
    }else if(pid>0){
        int states;
        wait(&states);
        printf("API & system call returned, the return value is %d.\n",procnum());
    }
    

    
    exit(0);
}