enum mm_usage{
	_MM_KERNEL,
	_MM_MMMAP,
	_MM_VGABUFF,
	_MM_PDTABLE,
	_MM_PTABLE,
	_MM_DYNAMIC,
	_MM_COUNT
};

struct bootmm_info{
	unsigned int start_pfn;
	unsigned int end_pfn;
	unsigned int type;
};

#define PAGE_FREE 0x0 //代表页是空闲的
#define PAGE_USED 0xff //代表页已经被占用了

#define MAX_INFO 10
struct bootmm {
	unsigned int phymm;//机器总共的物理内存大小
	unsigned int max_pfn;//系统中最大的物理页框号max_physical frame number
	unsigned char *s_map;//位图的起始地址
	unsigned char *e_map;//位图的末尾
	unsigned int last_alloc;//表示最近一次分配的地址????
	unsigned int cnt_infos;//表示其中有效项的项数
	struct bootmm_info info[MAX_INFO];//info数组用来记录当前内存中按照内存区域类型划分的使用情况
	//这是一下创建了很多个bootmm_info结构体放在info数组里
};
