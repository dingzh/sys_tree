//////////////////////////////////
//	File Name: sys_ptree.c
//	Author: Ding Zhou
//	Date: 	4-10
//      Usage:  add a system call to copy all running task info into userspace in DFS order;
//	version:1.0
//	Brief explaination: Create a stack and a kbuf in the kernel with kmalloc(), use the stack to DFS
//			    all the task starting with the macro "init_task", every time a element is popped
//			    out and its selected info will be copied into the kbuf, along all the time, task_list
//			    is locked. when the search job is done, infomation are copied to the usel space, with
//			    location specified by the struct prinfo *buf and int *nr.
//////////////////////////////////
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
MODULE_LICENSE("Dual BSD/GPL");
#define __NR_myptree 356
#define STACK_SIZE 60

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


static int (*oldcall)(void);
static int ptree(struct prinfo *buf, int *nr)
{
	int count = 0;				//count the task DFS has gone through
	int kbufCount = 0;			//as an iterator going through the kbuf
	int bufsize = *nr;			//record the bufsize in the user space
	int avail_kbuf = *nr;		//if kbuf is full, the DFS will countinue, but won't write into kbuf
	int stp = 0;				//stack pointer for stack S
	struct list_head* head;		//hold the element just popped up, which would determine when to stop push its children into stack
	struct task_struct* pos;	//used for iterate over a certain task's children
	struct task_struct **S;		//stack used to hold the pointer to task_struct
	struct prinfo *kbuf;		//point to the kbuf mem space

	//allocate memory in the kernel for kbuf and stack S, and check if successful
	kbuf = (struct prinfo*) kmalloc(bufsize*sizeof(struct prinfo), GFP_KERNEL);
	if (kbuf == NULL){
		printk("stack allocation error!");
		return -1;
	}
	S = (struct task_struct**) kmalloc(STACK_SIZE*sizeof(struct task_struct*), GFP_KERNEL);
	if (S == NULL){
		printk("stack allocation error!");
		kfree(kbuf);
		return -2;
	}
	read_lock(&tasklist_lock);
	S[stp] = &init_task;
	while(stp != -1){ 
		//copy info from the top element of the stack and then pop
		if(avail_kbuf != 0){
			struct prinfo *this = &(kbuf[kbufCount]);
			--avail_kbuf;
			++kbufCount;
			this->parent_pid = S[stp]->parent->pid;
			this->pid = S[stp]->pid;
			this->first_child_pid = list_entry(&(S[stp]->children), struct task_struct, sibling)->pid;
			this->next_sibling_pid = list_entry(&(S[stp]->sibling), struct task_struct, sibling)->pid;
			this->state = S[stp]->state;
			this->uid = S[stp]->cred->uid;
			get_task_comm(this->comm, S[stp]);
			//memcpy(this->comm, S[stp]->comm, 64);
			//printk("%s\t%d\t%ld\t\n",S[stp]->comm,S[stp]->pid,S[stp]->cred->uid);
			//printk("%s\t%d\t%ld\n\n",this->comm,this->pid,this->uid);
		}		
		else printk("kbuf full.\n");
		head = &(S[stp]->children);
		--stp;
		//push its children
		list_for_each_entry_reverse(pos,head,sibling){
			++count;
			//check the stack pointer, in case the stack was full
			if(stp != STACK_SIZE -5)	S[++stp] = pos;
			else {	
				printk("Error: stack full, exit!\n");
				read_unlock(&tasklist_lock);		//if full, exit, but still free the mem and unlock tasklist
				kfree(kbuf);
				kfree(S);
				return -3;
			}
		}
	}
	read_unlock(&tasklist_lock);
	memcpy(buf,kbuf,(bufsize)*sizeof(struct prinfo));
	kfree(S);
	kfree(kbuf);
	return count+1;	 //only if we count the init, or we can just return count.
}

static int addsyscall_init(void)
{
	long *syscall_list = (long *)0xc000d8c4;
	oldcall = (int(*)(void))(syscall_list[__NR_myptree]);
	syscall_list[__NR_myptree] = (unsigned long)ptree;
	printk(KERN_INFO "Module inserted!\n");
	return 0;
}

static void addsyscall_exit(void)
{
	long *syscall_list = (long*)0xc000d8c4;
	syscall_list[__NR_myptree] = (unsigned long)oldcall;
	printk(KERN_INFO "Module removed!\n");
}



module_init(addsyscall_init);
module_exit(addsyscall_exit);
