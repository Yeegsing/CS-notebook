#define _PAGE_RESERVED (1 << 31)
#define _PAGE_ALLOCED (1 << 30)

struct page {
	unsigned int flag;
	unsigned int refrence;
	struct list_head list;
	void *virtual;
	unsigned int private;
};

#define PAGE_SHIFT 12
#define MAX_BUDDY_ORDER 4
//一共支持5种order，分别对应124816个物理页面


struct freelist{
	unsigned int nr_free;
	struct list_head free_head;
};

struct buddy_sys{
	unsigned int buddy_start_pfn;
	unsigned int buddy_end_pfn;
	struct page *start_page;
	struct lock_t lock;//在哪里定义了？
	struct freelist freelist[MAX_BUDDY_ORDER + 1];
};
