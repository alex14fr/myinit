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
	while(waitpid(-1, NULL, WNOHANG)>0);
}

int main(int argc, char **argv) {
	pid_t cl;
	if((cl=fork()) > 0) { while(waitpid(cl, NULL, 0)<0); return(0); }
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
	char *envp[]={"PATH=/bin:/usr/bin:/usr/local/bin:/sbin:/usr/sbin","HOME=/home/" USER,"USER=" USER,"TERM=linux","XDG_RUNTIME_DIR=/run/xdgruntime",NULL};
	setgroups(NSGID, sgid);
	setregid(GID, GID);
	if(setreuid(UID, UID)<0) { perror("setreuid"); return(1); }
	int vtnum=5;
	//ioctl(fd, VT_ACTIVATE, vtnum);
	if((cl=fork())==0) {
		if(execve(argv[1], argv+1, envp)<0) {
			perror("execve");
			return(1);
		}
	} else {
		while(1) {
			pid_t clw;
			clw=wait(NULL);
			if(clw==cl) return(0);
		}
	}
}

