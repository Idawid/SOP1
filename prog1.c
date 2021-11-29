#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
                                     exit(EXIT_FAILURE))

//child labour, china would be proud
void child_work(int i) {
    srand(time(NULL)*getpid());     
    int t = 5 + rand() % 6;
    sleep(t); //wait 5-10s
    printf("PROCESS with pid %d terminates\n",getpid());
}

//error printing with command name (program name)
void errarg(char *name){
    fprintf(stderr,"USAGE: arg %s 0<n\n",name);
    exit(EXIT_FAILURE);
}

void create_children(int n) {
    //process id var type
    pid_t s;
    for (n--; n >= 0; n--) {
        //duplicates current process in separate memory space(tho they have the same content)
        //child process is an exact duplicate but for:
        //PID, parent's id is obv, process resource utilization, timers are reset to 0, asynchronous operations
        //return child's PID else, on failure, returns -1, sets errno
        if((s = fork()) < 0) ERR("Fork:");
        if(!s) {
            child_work(n);
            exit(EXIT_SUCCESS);
        }
    }
}

int main(int argc, char** argv) {
        int n;
        if(argc < 2) errarg(argv[0]);
        //how many processes?
        n = atoi(argv[1]);
        //error if 0 or less processes to make
        if(n <= 0)  errarg(argv[0]);
        //create children
        create_children(n);
        //this loop is so the process of waiting for children is repeated every 3s, until all are waited for (or error occurs)
        while(n > 0) {
            //return value (remaining time) not checked, because we don't handle any signals, sleep won't be broken
            sleep(3);
            pid_t pid;
            //this is so you can wait for many children
            for(;;) {
                //doesn't really wait if no waitable children exist (WNOHANG)
                pid = waitpid(0, NULL, WNOHANG);
                //waited for a child successfully -> collect other children
                if (pid > 0) n--;
                //no waitable children atm -> break, wait another 3s before this for(;;) loop
                if (pid == 0) break;
                //no children at all, common error (loop exits after n children are waited thought)
                if (pid <= 0) {
                    if(ECHILD == errno) break;
                    ERR("waitpid:");
                }
		    }
		    printf("PARENT: %d processes remain\n", n);
	    }
        return EXIT_SUCCESS;
}