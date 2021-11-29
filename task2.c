#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define MAXMLEN 102
#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
		     		     exit(EXIT_FAILURE))

volatile sig_atomic_t last_signal = 0;

void create_children(int n) {
	while (n-- > 0) {
		switch (fork()) {
            //if children created successfully override SIGUSR handling from the parent
            //sig_handler is now the action for SIGUSR1, SIGUSR2
			case 0:
				exit(EXIT_SUCCESS);
            //fork encountered an error, exit
			case -1: perror("Fork:");
				exit(EXIT_FAILURE);
		}
	}
}

//returns strlen()
ssize_t read_file_word(int fd, char* buffer) {
    ssize_t wordlen;
    if ((wordlen = read(fd, buffer, MAXMLEN)) == -1) ERR("Couldn't read_file_whole");
    wordlen--;
    buffer[wordlen] = '\0';
    return wordlen;
}

int main(int argc, char** argv) {
    //filepaths
    const char* fn_m = "Fm.txt";
    const char* fn_d = "Fd.txt";
    //buffer for Fm
    char mbuffer[MAXMLEN];
    //fd for Fm, Fd
    int fd_m;
    int fd_d;
    //open Fm.txt, Fd.txt, check
    if((fd_m = open(fn_m, O_RDONLY)) == -1) ERR("Fm couldn't be opened");
    if((fd_d = open(fn_d, O_RDONLY)) == -1) ERR("Fd couldn't be opened");
    //read Fm.txt, write to mbuffer
    int mlen = read_file_word(fd_m, mbuffer);

    //creates children
    create_children(mlen);
    //close Fm.txt, Fd.txt
    if(close(fd_m) == -1 || close(fd_d) == -1) ERR("Couldn't close files");
	return EXIT_SUCCESS;
}