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

void sethandler( void (*f)(int), int signum) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1 == sigaction(signum, &act, NULL)) ERR("sigaction");
}

void sig_handler(int sig) {
	last_signal = sig;
}

//handles SIGCHILD signal sent by terminated children, we just wait on them
//very similar to the one in prog13
void sigchld_handler(int sig) {
	pid_t pid;
	for(;;){
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

//send SIGUSR1, SIGUSR2 to parent continously
void child_work(int m, int p) {
	int count = 0;
	struct timespec t = {0, m * 10000};
	while(1) {
		for(int i = 0; i < p; i++){
			nanosleep(&t, NULL);
			if (kill(getppid(), SIGUSR1)) ERR("kill");
		}
		nanosleep(&t, NULL);
		if (kill(getppid(), SIGUSR2)) ERR("kill");
		count++;
		printf("[%d] sent %d SIGUSR2\n", getpid(), count);

	}
}


void parent_work(sigset_t oldmask) {
	int count = 0;
	while(1) {
		last_signal = 0;
		while(last_signal != SIGUSR2) {
			//suspends and changes the mask until a signal NOT in mask arrives
			sigsuspend(&oldmask);
		}
		count++;
		printf("[PARENT] received %d SIGUSR2\n", count);
		
	}
}

void usage(char *name) {
	fprintf(stderr, "USAGE: %s m  p\n", name);
	fprintf(stderr, "m - number of 1/1000 milliseconds between signals [1,999], i.e. one milisecond maximum\n");
	fprintf(stderr, "p - after p SIGUSR1 send one SIGUSER2  [1,999]\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
	int sec, n;
	//check param count, validity
	if(argc != 3) usage(argv[0]);
	sec = atoi(argv[1]); n = atoi(argv[2]);
	if (sec <= 0 || sec > 999 || n <= 0 || n > 999)  usage(argv[0]); 
	//signal handling for parents/children
	sethandler(sigchld_handler, SIGCHLD);
	sethandler(sig_handler, SIGUSR1);
	sethandler(sig_handler, SIGUSR2);

	sigset_t mask, oldmask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);
	//add signals to the process mask to be blocked, can't block SIGSTOP SIGKILL
	sigprocmask(SIG_BLOCK, &mask, &oldmask);
	pid_t pid;
	if((pid = fork()) < 0) ERR("fork");
	if(0 == pid) child_work(sec, n);
	else {
		parent_work(oldmask);
		while(wait(NULL) > 0);
	}
	sigprocmask(SIG_UNBLOCK, &mask, NULL);
	return EXIT_SUCCESS;
}