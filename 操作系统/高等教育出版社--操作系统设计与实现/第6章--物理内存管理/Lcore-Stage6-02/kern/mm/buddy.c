//首先要初始化buddy系统，因为目前的内存分配器仍然是bootmem

void init_buddy()
{
unsigned int index = sizeof(struct page);

pages = bootmm_alloc_pages(multiply(sizeof(struct page), boot_mm.max_pfn), _MM_KERNEL, 1 << PAGE_SHIFT);
//每个物理页面都会对应一个struct page对象
//首先调用bootmm_alloc_pages函数，申请一块足以容纳所有page对象的内存空间
if(!pages)
{	
	printk("\nERROR: bootmm_alloc_pages failed!\n");
	die();//如果失败则直接停止系统
}	

init_pages(0, boot_mm.max_pfn);//调用init_pages初始化上一步所有的struct page对象

kernel_start_pfn = 0;
kernel_end_pfn = 0;
for(index = 0; index < boot_mm.cnt_infos; ++index) {
	if(boot_mm.info[index].end > kernel_end_pfn)
		kernel_end_pfn = boot_mm.info[index].end;
}
kernel_end_pfn >>= PAGE_SHIFT;

buddy.buddy_start_pfn = (kernel_end_pfn + (1 << MAX_BUDDY_ORDER) - 1) & ~((1 << MAX_BUDDY_ORDER)-1);
buddy.buddy_end_pfn = boot_mm.max_pfn & ~((1 << MAX_BUDDY_ORDER) -1);
for(index = 0; index <= MAX_BUDDY_ORDER; ++index) {
	buddy.freelist[index].nr_free = 0;
	INIT_LIST_HEAD(&(buddy.freelist[index].free_head));
}
buddy.start_page = pages + buddy.buddy_start_pfn;
init_lock(&(buddy.lock));

for(index = buddy.buddy_start_pfn; index < buddy.buddy_end_pfn; ++index) {
	free_pages(pages +index, 0);//调用free_pages将系统中的可用内存释放，通过此接口将系统可用内存填充到Buddy系统中，实现如下
}

buddy_info();
}


//page表示要释放的内存块的第一个页面所对应的page对象，order表示要释放的内存块的阶大小
void free_pages(struct page *page, unsigned int order)
{
	unsigned int page_idx, buddy_idx;//表示当前要释放的内存块第一个页面的索引号
	unsigned int combined_idx;
	struct page *buddy_page;

	clean_flag(page, -1);

	lockup(&buddy.lock);

	page_idx = page - buddy.start_page;
	while(order < MAX_BUDDY_ORDER){//经典的Buddy系统释放算法
		buddy_idx = page_idx ^(1 << order);//通过page_idx和当前要是释放的order异或得出buddy_idx
		buddy_page = page + (buddy_idx - page_idx);//通过buddy_index获得对应的page对象
		if(!is_buddy(buddy_page, order))//通过is_buddy函数来判断两者是否为伙伴关系，即观察两个内存块的order是否相等
			break;
		list_del_init(&buddy_page->list);//如果是就开始进行合并操作
		--buddy.freelist[order].nr_free;
		clear_order(buddy_page);
		combined_idx = buddy_idx & page_idx;
		page += (combined_idx - page_idx);
		page_idx = combined_idx;
		++order;
	}
	set_order(page, order);
	list_add(&(page->list), &(buddy.freelist[order].free_head));
	++buddy.freelist[order].nr_free;

	unlock(&buddy.lock);
}


//内存分配就是释放的反过程，主要是通过alloc_pages来完成的
struct page *alloc_pages(unsigned int order)
{
unsigned int current_order, size;
struct page *page, *buddy_page;
struct freelist *free;

lockup(&buddy.lock);

for(current_order = order; current_order <= MAX_BUDDY_ORDER; ++current_order){
	free = buddy.freelist + current_order;
	if(!list_empty(&(free->free_head)))
		goto found;
}

unlock(&buddy.lock);
return 0;

found:
page = container_of(free->free_head.next, struct page, list);
list_del_init(&(page->list));
clear_order(page);
set_flag(page, _PAGE_ALLOCED);
--(free->nr_free);

size = 1 << current_order;
while(current_order > order){
	--free;
	--current_order;
	size >>= 1;
	buddy_page = page + size;
	list_add(&(buddy_page->list), &(free->free_head));
	++(free->nr_free);
	set_order(buddy_page, current_order);
}
unlock(&budd.lock);
return page;
}
