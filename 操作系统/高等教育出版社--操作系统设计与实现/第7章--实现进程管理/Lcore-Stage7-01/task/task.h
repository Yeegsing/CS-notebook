//进程间的关系
//最大的是task_union，然后里面含有task_struct，这个是与vma联系在一起的（通过什么？
struct vma {
	struct list_head node;
	unsigned int start;
	unsigned int cnt;
	unsigned int vend;
};


struct regs_context{
	unsigned int v0, v1;
	unsigned int a0, a1, a2, a3;
	unsigned int t0, t1, t2, t3, t4, t5, t6, t7;
	unsigned int s0, s1, s2, s3, s4, s5, s6, s7;
	unsigned int t8, t9;
	unsigned int gp, sp, fp, ra, k0, k1;
	unsigned int epc, ear;
};

struct task_struct{
	unsigned int user_stack;
	unsigned int user_code;
	unsigned int pid;
	unsigned int parent;
	unsigned int state;
	unsigned int counter;
	unsigned int *pgd;
	struct list_head sched;
	struct list_head node;
	struct signal sigaction[MAX_SIGNS];

	struct list_head stack_vma_head;
	struct list_head user_vma_head;
	struct list_head heap_vma_head;
};

union task_union{
	struct task_struct task;
	unsigned char kernel_stack[KERN_STACK_SIZE];
};
