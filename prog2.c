#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source), kill(0,SIGKILL),\
		     		     exit(EXIT_FAILURE))

volatile sig_atomic_t last_signal = 0;

//change action performed on signal signum
void sethandler( void (*f)(int), int signum) {
    //initialize sigaction var to be passed to the true function
    //sigaction struct: void sa_handler, sigset_t mask to be blocked, other...
	struct sigaction act;
    //0 fill act struct
	memset(&act, 0, sizeof(struct sigaction));
    //sa_handler is the function passed in args
	act.sa_handler = f;


    //THIS IS WHAT CHANGES THE ACTION!
	if (-1 == sigaction(signum, &act, NULL)) ERR("sigaction");
}

//prints info ab the signal, changes value of global var last_signal
void sig_handler(int sig) {
	printf("[%d] received signal %d\n", getpid(), sig);
	last_signal = sig;
}

//handles SIGCHILD signal sent by terminated children, we just wait on them
//very similar to the one in prog13
void sigchld_handler(int sig) {
	pid_t pid;
    //always use this loop
	while(1) {
		pid = waitpid(0, NULL, WNOHANG);
        //no waitable children
		if (pid == 0) return;
        //no children at all
		if (pid <= 0) {
			if (errno == ECHILD) return;
			ERR("waitpid");
		}
	}
}

void child_work(int l) {
	int t, tt;
	srand(getpid());
    //range from 5 to 10
	t = rand() % 6 + 5;
    //work for l * 5-10s, on each iteration check last_signal
	while(l-- > 0) {
        //this strange loop guarantees full sleep duration if its interrupted for some reason
		for(tt = t; tt > 0; tt = sleep(tt));

		if (last_signal == SIGUSR1) printf("Success [%d]\n", getpid());
		else printf("Failed [%d]\n", getpid());
	}
	printf("[%d] Terminates \n",getpid());
}

//sends SIGALARM, after that: SIGUSR1, SIGUSR1
void parent_work(int k, int p, int l) {
    //struct: seconds, nanoseconds
	struct timespec tk = {k, 0};
	struct timespec tp = {p, 0};
    //sets sig_handler as the action for SIGALARM
	sethandler(sig_handler, SIGALRM);
    //send the SIGALARM signal after l*10s, this will trigger sig_handler
	alarm(l * 10);
    //as long as sig_handler didn't set last_signal to SIGALARM
	while(last_signal != SIGALRM) {
        //sleep for tk to delay the loop
		nanosleep(&tk, NULL);
        //send SIGUSR1 to all processes in the group
		if (kill(0, SIGUSR1) < 0) ERR("kill");
        //sleep for tk to delay the loop
		nanosleep(&tp, NULL);
        //send SIGUSR1 to all processes in the group
		if (kill(0, SIGUSR2) < 0) ERR("kill");
	}
	printf("[PARENT] Terminates \n");
}

//creates n children
void create_children(int n, int l) {
	while (n-- > 0) {
		switch (fork()) {
            //if children created successfully override SIGUSR handling from the parent
            //sig_handler is now the action for SIGUSR1, SIGUSR2
			case 0: sethandler(sig_handler,SIGUSR1);
				sethandler(sig_handler,SIGUSR2);
				child_work(l);
				exit(EXIT_SUCCESS);
            //fork encountered an error, exit
			case -1: perror("Fork:");
				exit(EXIT_FAILURE);
		}
	}
}

void usage(void){
	fprintf(stderr,"USAGE: signals n k p l\n");
	fprintf(stderr,"n - number of children\n");
	fprintf(stderr,"k - Interval before SIGUSR1\n");
	fprintf(stderr,"p - Interval before SIGUSR2\n");
	fprintf(stderr,"l - lifetime of child in cycles\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
    //n children, k sec wait before SIGUSR1, p sec wait before SIGUSR2, l - children work for l * 5-10s 
	int n, k, p, l;
    //error, one of the parameters is absent!
	if (argc != 5) usage();
    //reads 4 parameters
	n = atoi(argv[1]); k = atoi(argv[2]); p = atoi(argv[3]); l = atoi(argv[4]);
    //invalid parameters!
	if (n <= 0 || k <= 0 || p <= 0 || l <= 0)  usage(); 

    //SIGCHILD (signal sent when child terminates) handling for parent (for children, too)
	sethandler(sigchld_handler, SIGCHLD);
    //SIG_IGN - function to ignore signal, parent must ignore them obv, it's also set for children- to be changed in them later
    //actually it's possible to replace SIG_IGN with sig_handler rn, before forking
	sethandler(SIG_IGN, SIGUSR1);
	sethandler(SIG_IGN, SIGUSR2);
    //create n children, each works for l*5-10s
	create_children(n, l);
	parent_work(k, p, l);
	while (wait(NULL) > 0);
	return EXIT_SUCCESS;
}