#include "kernel/types.h"
#include "user/user.h"

int main(int argc,char *argv[])
{
    int v=procnum();

    printf("API & system call returned, the return value is %d.\n", v);
    exit(0);
}