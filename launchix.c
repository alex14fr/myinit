#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/vt.h>

#include "launchix_config.h"

void sigchld(int signum) {
	int wstatus;
	while(waitpid(-1, &wstatus, WNOHANG) > 0);
}

int main(int argc, char **argv) {
	for(int i=0; i<10; i++) close(i);
	setsid();
	int fd=open(TTY, O_RDWR);
	if(fd<0) { perror("open"); return(1); }
	if(fchown(fd, UID, GID)<0) { perror("fchown"); return(1); }
	if(fchmod(fd, 0600)<0) { perror("fchmod"); return(1);}
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	chdir("/home/" USER);
	signal(SIGCHLD, sigchld);
	gid_t sgid[]={GRPS};
	char *envp[]={"PATH=/bin:/usr/bin:/usr/local/bin:/sbin:/usr/sbin","HOME=/home/" USER,"USER=" USER,"TERM=linux",NULL};
	setgroups(NSGID, sgid);
	setregid(GID, GID);
	if(setreuid(UID, UID)<0) { perror("setreuid"); return(1); }
	int vtnum=5;
	//ioctl(fd, VT_ACTIVATE, vtnum);
	pid_t cl;
	if((cl=fork())==0) {
		if(execle(CMD, CMD, NULL, envp)<0) {
			perror("execle");
			return(1);
		}
	} else {
		int wstatus;
		while(1) if(cl==wait(&wstatus)) return(0);
	}
}

