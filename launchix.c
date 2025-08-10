#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <linux/vt.h>

#include "launchix_config.h"

int main(int argc, char **argv) {
	pid_t cl;
	if(chown(TTY, UID, GID)<0) { perror("chown"); return(1); }
	if(chmod(TTY, 0600)<0) { perror("chmod"); return(1); }
	chdir("/home/" USER);
	gid_t sgid[]={GRPS};
	char *envp[]={"PATH=/bin:/usr/bin:/usr/local/bin:/sbin:/usr/sbin","HOME=/home/" USER,"USER=" USER,"TERM=linux","XDG_RUNTIME_DIR=/run/xdgruntime",NULL};
	setgroups(NSGID, sgid);
	setregid(GID, GID);
	if(setreuid(UID, UID)<0) { perror("setreuid"); return(1); }
#if 0
	if((cl=fork())==0) {
#endif
		if(execve(argv[1], argv+1, envp)<0) {
			perror("execve");
			return(1);
		}
#if 0
	} else {
		while(1) {
			pid_t clw;
			clw=wait(NULL);
			if(clw==cl) return(0);
		}
	}
#endif
}

