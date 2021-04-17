void init_kmem_cpu(struct Kmem_cache_cpu *kcpu)
{
	kcpu->page = 0;
	kcpu->freeobj = 0;
}

void init_kmem_node(struct Kmem_cache_node *knode)
{
	INIT_LIST_HEAD(&(knode->full));
	INIT_LIST_HEAD(&(knode->partial));
}

void init_each_slub(struct kmem_cache *cache, unsigned int size)
{
	cache->objsize = size;
	cache->objsize += (BYTES_PER_LONG -1);
	cache->objsize &= ~(BYTES_PER_LONG -1);
	cache->offset = cache->objsize;
	cache->size = cache->objsize + sizeof(void *);
	init_kmem_cpu(&(cache->cpu));
	init_kmem_node(&(cache->node));
}

void init_slub()
{
	unsigned int order;

	init_each_slub(&(kmalloc_caches[1]), 96);//可以看出slub对于96B和192B的对象有特殊处理
	init_each_slub(&(kmalloc_caches[2]), 192);

	for(order = 3; order <= 11; ++order) {//对剩下8种进行初始化设置
		init_each_slub(&(kmalloc_caches[order]), 1 << order);
	}

	printk("Setup Slub ok : \n");
	printk("\t");
	for(order = 1; order < PAGE_SHIFT; ++order)
		printk("%x", kmalloc_caches[order].objsize);
	printk("\n");
}

//对新分配的缓存页面进行初始化设置的操作
void format_slubpage(struct kmem_cache *cache, struct page *page)
{
	unsigned char *m = (unsigned char *)((page - pages) << PAGE_SHIFT);
	struct slub_head *s_head = (struct slub_head *)m;
	unsigned int remaining = 1 << PAGE_SHIFT;
	unsigned int *ptr;

	set_flag(page, _PAGE_SLUB);
	s_head->nr_objs = 0;
	do {
		ptr = (unsigned int *)(m + cache->offset);
		m += cache->size;
		*ptr = (unsigned int)m;
		remaining -= cache->size;
	}while(remaining >= cache->size);

	*ptr = (unsigned int)m & ~((1 << PAGE_SHIFT) -1);

	s_head->end_ptr = ptr;
	s_head->nr_objs = 0;
	cache->cpu.page = page;
	cache->cpu.freeobj = (void **)(*ptr + cache->offset);
	page->private = (unsigned int)(*(cache->cpu.freeobj));
	page->virtual = (void*)cache;
}


//Slub系统分配内存
void *slub_alloc(struct kmem_cache *cache)
{
	struct slub_head *s_head;
	void *object = 0;
	struct page *new;

	if(cache->cpu.freeobj)
		object = *(cache->cpu.freeobj);

check:
	if(is_bound((unsigned int)object, 1 << PAGE_SHIFT)) {
		if(cache->cpu.page)
			list_add_tail(&(cache->cpu.page->list), &(cache->node.full));
		if(list_empty(&(cache->node.partial)))
			goto new_slub;
		cache->cpu.page = container_of(cache->node.partial.next, struct page, list);
		list_del(cache->node.partial.next);
		object = (void *)(cache->cpu.page->private);
		cache->cpu.freeobj = (void **)((unsigned char *)object + cache->offset);
		goto check;
	}
	cache->cpu.freeobj = (void **)((unsigned char *)object + cache->offset);
	cache->cpu.page->private = (unsigned int)(*(cache->cpu.freeobj));
	s_head = (struct slub_head *)((cache->cpu.page-pages) << PAGE_SHIFT);
	++(s_head->nr_objs);
	if(is_bound(cache->cpu.page->private, 1 << PAGE_SHIFT))
	{
		list_add_tail(&(cache->cpu.page->list), &(cache->node, full));
		init_kmem_cpu(&(cache->cpu));
	}
	return object;

new_slub:

	new = _alloc_pages(0);
	if(!new) {
		printk("ERROR: slub_alloc error!\n");
		die();
	}

	print("\n *** %x\n", new -pages);

	format_slubpage(cache, new);
	object = *(cache->cpu.freeobj);
	goto check;
}

void slub_free(struct kmem_cache *cache, void *obj)
{
	struct page *page = pages += ((unsigned int)obj >> PAGE_SHIFT);
	struct slub_head *s_head = (struct slub_head *)((page - pages) << PAGE_SHIFT);
	unsigned int *ptr;

	if(!(s_head->nr_objs)) {
		printk("ERROR: slub_free error!\n");
		die();
	}

	ptr = (unsigned int *)((unsigned char *)obj + cache->offset);
	*ptr = *((unsigned int *)(s_head->end_ptr));
	*((unsigned int *)(s_head->end_ptr)) = (unsigned int)obj;
	--(s_head->nr_objs);

	if(list_empty(&(page->list)))
		return;

	if(!(s_head->nr_objs)){
		_free_pages(page, 0);
		return;
	}

	list_del_init(&(page->list));
	list_add_tail(&(page->list), &(cache->node.partial));
}

//实现一种无论程序分配多大的内存空间都可以通过此接口完成，就不用自行
//去判断是要用alloc_slub还是alloc_pages函数

void *kmalloc(unsigned int size)
{
	struct kmem_cache *cache;

	if(!size)
		return 0;
	
	if(size > kmalloc_caches[PAGE_SHIFT - 1].objsize){
		size += ((1 << PAGE_SHIFT) -1);
		size &= ~((1 << PAGE_SHIFT) -1);
		return alloc_pages(size >> PAGE_SHIFT);
	}

	cache = get_slub(size);
	if(!size){
		printk("ERROR: kmalloc error!\n");
		die();
	}
	return slub_alloc(cache);
}

void kfree(void *obj)
{
	struct page *page;

	page = pages + ((unsigned int)obj >> PAGE_SHIFT);
	if(!has_flag(page, _PAGE_SLUB))
		return free_pages((void *)((unsigned int)obj & ~((1 << PAGE_SHIFT) -1)), page->private);

	return slub_free(page->virtual, obj);
}
