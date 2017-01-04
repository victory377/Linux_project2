#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

main(int argc, char **argv) {
    int pid;
    printf("Input the global PID of the process you want to wake up.\n");
    printf("Pid: ");
    scanf(" %d",&pid);
    printf("Format: program PID\n");
    if(syscall(353,pid)<0) { //new system call two
        printf("Cannot wake up process %d\n", pid);
    }
    else
        printf("Process %d has been waked up.\n",pid);
    
}