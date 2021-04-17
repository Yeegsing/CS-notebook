//buddy系统可以应付以页为单位的内存分配请求，但是在内核中会有很多较小的数据结构对象
//有些是静态分布的，更多的是系统运行过程中动态分布的，若进程结构体
//linux中使用slub分配器来实现

//这个slub_head结构体用来维护一个Slub内存页表的头部信息
struct slub_head {
	void **end_ptr;//指向Slub内存页面中最后一个对象的地址的指针（取地址，在作指针，如此为**）
	unsigned int nr_objs;//表示这个Slub内存页面已经分配了多少个对象
};

//其中维护两个链表
//partial链表用来链接那些部分分配的slub内存页面
//full链表用来链接那些已经被全部使用的Slub内存页面
struct kmem_cache_node {
	struct list_head partial;
	struct list_head full;
};

//该结构体用来描述当前正被用于分配的Slub内存页面
struct kmem_cache_cpu {
	void **freeobj;//指向下一个待分配出去的空闲内存对象的地址
	struct page *page;//指向此Slub内存页面对应的struct page结构体对象
};

//kmem_cache结构体是Slub内存管理系统的核心，它维护了每个Slub缓存的基本信息
struct kmem_cache {
	unsigned int size;//表示Slub缓存中每个对象实际要占多大的内存空间
	unsigned int objsize;//表示对象本身需要多少内存空间，这个大小也是需要对齐的
	unsigned int offset;//表示Slub内存页用于存放对象的内存地址相对于此页面起始地址的偏移
	struct kmem_cache_node node;//分别对于的······
	struct kmem_cache_cpu cpu;
	unsigned char name[16];//这个Slub缓存的名字
};
