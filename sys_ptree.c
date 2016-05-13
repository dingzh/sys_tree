#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/unistd.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/slab.h>
MODULE_LICENSE("Dual BSD/GPL");
#define __NR_myptree 356
#define STACK_SIZE 100


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


static int (*oldcall)(void);
static int ptree(struct prinfo *buf, int *nr)
{
	int count = 0;
	int bufcount = 0;
	int bufsize = *nr;
	int avail_kbuf = *nr;
	int stp = 0;
	int strcpyTem;
	struct list_head* head;
	struct task_struct* pos;
	struct task_struct **S;
	struct prinfo *kbuf;
	struct prinfo *this;
	kbuf = (struct prinfo*) kmalloc(bufsize*sizeof(struct prinfo), GFP_KERNEL);
	if (kbuf == NULL){
		printk("stack allocation error!");
		return -3;
	}
	S = (struct task_struct**) kmalloc(STACK_SIZE*sizeof(struct task_struct*), GFP_KERNEL);
	if (S == NULL){
		printk("stack allocation error!");
		kfree(kbuf);
		return -1;
	}

	read_lock(&tasklist_lock);
	S[stp] = &init_task;
	while(stp != -1){ 
		//copy info from the top element of the stack and then pop
		if(avail_kbuf != 0){
			this = &(kbuf[bufcount]);
			--avail_kbuf;
			++bufcount;
			this->parent_pid = S[stp]->parent->pid;
			this->pid = S[stp]->pid;
			this->first_child_pid = list_entry((S[stp]->children).next, struct task_struct, sibling)->pid;
			this->next_sibling_pid = list_entry((S[stp]->sibling).next, struct task_struct, sibling)->pid;
			this->state = S[stp]->state;
			this->uid = S[stp]->cred->uid;
			for(strcpyTem = 0; strcpyTem<64; strcpyTem++) (this->comm)[strcpyTem] = ((S[stp])->comm)[strcpyTem];
		}		
		head = &(S[stp]->children);
		--stp;
		//push its children
		list_for_each_entry_reverse(pos,head,sibling){
			++count;
			if(stp != STACK_SIZE -5)	S[++stp] = pos;
			else {	
				printk("Error: stack full, exit!\n");
				read_unlock(&tasklist_lock);
				kfree(kbuf);
				kfree(S);
				return -2;
			}
		}
	}
	read_unlock(&tasklist_lock);
	printk("read_unlock\n");
	while(bufsize--){
		buf[bufsize] = kbuf[bufsize];
	}
	kfree(S);
	kfree(kbuf);
	return count+1;	
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

