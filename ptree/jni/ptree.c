///////////////////////////////////////////////////
// File Name: ptree.c
// Author: Ding Zhou
// Date: 2016-4-14
// Usage: use the system call sys_ptree to get all the task running in DFS
//        order, and print it with appropriate indent
// Version: 1.0
////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#ifndef __PR_INFO__
#define __PR_INFO__
struct prinfo
{
	pid_t parent_pid; 		/* process id of parent */
	pid_t pid; 				/* process id */
	pid_t first_child_pid; 	/* pid of youngest child */
	pid_t next_sibling_pid; /* pid of older sibling */
	long  state; 			/* current state of process */
	long  uid; 				/* user id of process owner */
	char  comm[64]; 		/* name of program executed */
};
#endif

int parent_in_stack(int *S, int ppid, int stp){
	int i = 0;
	int flag = 0;
	for(i = 0; i<=stp; ++i){
		if (S[i] == ppid) {
			flag = 1;
			break;
		}
	}
	if(flag)	stp = i;
	else	S[++stp] = ppid;
	return stp;
}

int main(int argc, char const *argv[])
{
	//int nr = atoi(argv[1]);
	int nr = 100;
	int *S;
	int stp = 0;
	struct prinfo *buf = (struct prinfo*) malloc(nr*sizeof(struct prinfo));
	S = (int*) malloc(20*sizeof(int));
	int i=syscall(356,buf,&nr);
	printf("%d entries in total, %d copied\n", i,((i<nr)?i:nr));
	struct prinfo* p = buf;
	int j,k;
	S[stp] = 0;
	for(j = 0; j<i && j<nr; ++j){
		k = stp = parent_in_stack(S, p->parent_pid,stp);
		while(k--) printf("\t");
		printf("%s,%d,%ld,%d,%d,%d,%ld\n", p->comm, p->pid, p->state,p->parent_pid, p->first_child_pid, p->next_sibling_pid, p->uid);
		p++;
	}
	free(S);
	free(buf);
	return 0;
}
