#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

extern void bringup(void);
char ttys[32][64];

int myrdline(int fd, char *s, int maxlen) {
	for(int i=0; i<maxlen; i++) {
		if(read(fd, s+i, 1)<0 || s[i]=='\n') {
			s[i]=0;
			return(i);
		} 
	}
	s[maxlen]=0;
	return(maxlen);
}

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
	sleep(1);
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
	sleep(1);
}


void launch_emerg(void) {
	int wstatus;
	int f=open("/etc/ttys", O_RDONLY);
	if(!f) {
		perror("open /etc/ttys");
		goto launch_sh;
	} 
	char constty[64];
	memcpy(constty, "/dev/", 5);
	int n=myrdline(f, constty+5, 50);
	close(f);
	printf("Mounting /dev and /proc...\n");
	if(fork()==0) { execl("/bin/mount", "mount", "-t", "devtmpfs", "none", "/dev", NULL); }
	wait(&wstatus);
	if(fork()==0) { execl("/bin/mount", "mount", "-t", "proc", "none", "/proc", NULL); }
	wait(&wstatus);
	if(n>0) {
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
	}
launch_sh:
	if(fork() == 0) {
		if(execl("/bin/sh", "sh", NULL) == -1) {
			perror("execl /bin/sh");
		}
	}
	wait(&wstatus);
	bringup();
}

pid_t launch_getty(char *ttyname) {
	pid_t x=fork();
	if(x==0) {
		for(int i=3; i<10; i++)	close(i);
		if(execl("/sbin/getty", "getty", "0", ttyname, NULL) == -1) {
			perror("execl /sbin/getty");
			exit(1);
		}
	}
	return(x);
}

void bringup(void) {
	int wstatus;
	pid_t gettypids[32];
	int nttys=0;

	printf("INIT: running /etc/rc\n");
	int rc_pid;
	if((rc_pid=fork())==0) {
		if(execl("/etc/rc", "rc", NULL) == -1) {
			perror("execl /etc/rc");
			exit(1);
		}
	} else {
		pid_t ch;
		do {
			ch=wait(&wstatus);
		} while(ch != rc_pid);
		printf("INIT: /etc/rc finished, status=%d\n", WEXITSTATUS(wstatus));
		int f=open("/etc/ttys", O_RDONLY);
		if(!f || !WIFEXITED(wstatus) || WEXITSTATUS(wstatus) != 0) {
			if(f) close(f);
			launch_emerg();
		}
		for(int i=0; i<32; i++) {
			int n=myrdline(f, ttys[i], 64);
			if(n>0) {
				gettypids[i] = launch_getty(ttys[i]);
				nttys++;
			} else {
				break;
			}
		}
		close(f);
		while(1) {
			pid_t ch=wait(&wstatus);
			for(int i=0; i<nttys; i++)  {
				if(strlen(ttys[i])>0 && ch == gettypids[i]) 
					gettypids[i] = launch_getty(ttys[i]);
			}
		}
	}
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
	prepare_shut();
	launch_emerg();
	prepare_shut();
	syscall(SYS_reboot, 0xfee1dead, 0x28121969, 0x1234567);
}


int main(int argc, char **argv) {
	setsid();
	signal(SIGCHLD, sigchld);
	signal(SIGUSR1, sighalt);
	signal(SIGUSR2, sigpoweroff);
	signal(SIGTERM, sigreboot);
	signal(SIGINT, sigreboot);
	syscall(SYS_reboot, 0xfee1dead, 0x28121969, 0);
	signal(SIGQUIT, sigquit);
	bringup();
}

