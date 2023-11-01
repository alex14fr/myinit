#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <fcntl.h>

void prepare_shut(void) {
	struct sigaction sa;
	sa.sa_handler=SIG_IGN;
	sigaction(SIGCHLD, &sa, NULL);
	int wstatus;
	printf("Syncing...\n");
	sync();
	sleep(1);
	printf("Sending TERM signal to all processes...\n");
	kill(-1, SIGTERM);
	sleep(3);
	printf("Sending KILL signal to all processes...\n");
	kill(-1, SIGKILL);
	sleep(1);
	printf("Remounting root filesystem read-only...\n");
	if(fork()==0) { execl("/bin/mount", "mount", "-o", "remount,ro", "/", NULL); }
	wait(&wstatus);
	printf("Unmounting all filesystems...\n");
	if(fork()==0) { execl("/bin/umount", "umount", "-a", "-r", NULL); }
	wait(&wstatus);
	printf("Syncing...\n");
	sync();
	sleep(2);
}

void launch_emerg(FILE *f) {
	int wstatus;
	char *constty=NULL, *constty2=NULL;
	ssize_t n=64;
	if(!f) {
		perror("open /etc/ttys");
		goto launch_sh;
	} 
	constty=malloc(n);
	constty2=malloc(n);
	n=getline(&constty2, &n, f);
	if(n>0) {
		constty2[n-1]=0;
		snprintf(constty, 64, "/dev/%s", constty2);
	} else {
		goto launch_sh;
	}
	printf("Using %s for emergency console. \n", constty);
	int ff=open(constty, O_RDWR);
	if(ff<0) {
		perror("open emergency console");
		goto launch_sh;
	} 
	dup2(ff, 0);
	dup2(ff, 1);
	dup2(ff, 2);
	close(ff);
launch_sh:
	free(constty);
	free(constty2);
	if(fork() == 0) {
		if(execl("/bin/sh", "sh", NULL) == -1) {
			perror("execl /bin/sh");
		}
	}
	wait(&wstatus);
}

pid_t launch_getty(char *ttyname) {
	pid_t x=fork();
	if(x==0) {
		if(execl("/sbin/getty", "getty", "0", ttyname, NULL) == -1) {
			perror("execl /sbin/getty");
			exit(1);
		}
	}
	return(x);
}

void sigchld(int s) {

}

void sighalt(int s) {
	prepare_shut();
	syscall(SYS_reboot, 0xfee1dead, 0x28121969, 0xcdef0123);
}

void sigpoweroff(int s) {
	prepare_shut();
	syscall(SYS_reboot, 0xfee1dead, 0x28121969, 0x4321fedc);
}

void sigreboot(int s) {
	prepare_shut();
	syscall(SYS_reboot, 0xfee1dead, 0x28121969, 0x1234567);
}

void sigquit(int s) {
	while(1) {
		prepare_shut();
		FILE *f=fopen("/etc/ttys", "r");
		launch_emerg(f);
		fclose(f);
	}
}


int main(int argc, char **argv) {
	int wstatus;
	pid_t gettypids[4];
	char *ttys[32];
	int nttys=0;
	setsid();
	signal(SIGCHLD, sigchld);
	signal(SIGUSR1, sighalt);
	signal(SIGUSR2, sigpoweroff);
	signal(SIGTERM, sigreboot);
	signal(SIGINT, sigreboot);
	syscall(SYS_reboot, 0xfee1dead, 0x28121969, 0);
	signal(SIGQUIT, sigquit);

	printf("INIT: running /etc/rc\n");
	if(fork()==0) {
		if(execl("/etc/rc", "rc", NULL) == -1) {
			perror("execl /etc/rc");
			exit(1);
		}
	} else {
		pid_t ch=wait(&wstatus);
		ssize_t n=64;
		printf("INIT: /etc/rc finished, status=%d\n", WEXITSTATUS(wstatus));
		FILE *f=fopen("/etc/ttys", "r");
		if(!f || !WIFEXITED(wstatus) || WEXITSTATUS(wstatus) != 0) {
			launch_emerg(f);
		}
		if(f) fclose(f);
		f=fopen("/etc/ttys", "r");
		if(!f) {
			perror("open /etc/ttys");
			launch_emerg(NULL);
		}
		for(int i=0; i<32; i++) {
			ttys[i] = malloc(n);
			if((n=getline(&(ttys[i]), &n, f)) < 0) break;
			ttys[i][n-1] = 0;
			gettypids[i] = launch_getty(ttys[i]);
			nttys++;
		}
		fclose(f);
		while(1) {
			pid_t ch=wait(&wstatus);
			for(int i=0; i<nttys; i++)  {
				if(ch == gettypids[i]) 
					gettypids[i] = launch_getty(ttys[i]);
			}
		}
	}
}

