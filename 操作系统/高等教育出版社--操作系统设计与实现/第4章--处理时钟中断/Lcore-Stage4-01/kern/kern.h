#define container_of(ptr, type, member) ((type *)((char *)ptr - (char *)&(((type*)0)->)member))

//中断处理是通过双向链表来进行实现的，所有先实现它
//list_head循环双向链表
//prev是指向前一个元素
//next是指向后一个元素

struct list_head {
	struct list_head *prev;
	struct list_head *next;
};

/*
*中断处理机构
*/

typedef void (*intr_fn)();

//对应的是具体某类中断中的一项任务
struct intr_work{
	intr_fn work;
	struct list_head node;
};

//对应的是每一类中断
struct intr_block{
	intr_fn entry_fn;
	struct list_head head;
};

