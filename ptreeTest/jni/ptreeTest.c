///////////////////////////////////////////////////
// File Name: ptreeTest.c
// Author: Ding Zhou
// Date: 2016-4-14
// Usage: to fork a child process which would run the ptreeARM, in the 
//        output of the terminal, we shall see the parent-child relation
//        between ptreeTest and ptreeARM
// Version: 1.0
////////////////////////////////////////////////////
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

int main()
{
	int i = 1;
	pid_t pid;
/* fork another process */
	pid = fork();
	if (pid < 0) { /* error occurred */
		fprintf(stderr, "Fork Failed");
		return 1;
	}

	if (pid == 0) { /* child process */
		execl("/data/misc/ptreeARM","ptreeARM",((char*) NULL));
	}
	else { /* parent process */
		/* parent will wait for the child */
		wait(NULL);
		printf ("Child Complete.");
	}
}
