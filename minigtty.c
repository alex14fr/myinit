#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char **argv) {
	setsid();
	char constty[64];
	memcpy(constty, "/dev/", 5);
	strncpy(constty+5, argv[2], 50);
	int ff=open(constty, O_RDWR);
	if(ff<0) {
		perror("open tty");
		goto launch_sh;
	} 
	dup2(ff, 0);
	dup2(ff, 1);
	dup2(ff, 2);
	for(int i=3;i<10;i++)
		close(i);
launch_sh:
	if(execl("/bin/sh", "sh", NULL) == -1) {
		perror("execl /bin/sh");
		return(-1);
	}
	return(0);
}
