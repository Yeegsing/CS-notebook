//set_mminfo函数的作用就是单纯的根据传入的参数进行对bootmm_info对象进行设置
void set_mminfo(struct bootmm_info *info, unsigned int start, unsigned int end, unsigned int type)
{
	info->start_pfn = start;
	info->end_pfn = end;
	info->type = type;
}

void remove_mminfo(struct bootmm *mm, unsigned int index)
{
	unsigned int tmp;

	if(index >= mm->cnt_infos)
		return;

	for(tmp = (index + 1); tmp != mm->cnt_infos; ++tmp) {
		mm->info[tmp-1] = mm->info[tmp];
	}

	--(mm->cnt_infos);
}

//函数的作用是在内存区域数组中插入一段内存区域
//需要好好理解bootmm与info数组之间的关系
//实际上，只需要记住bootmm_info是用来描述一片内存区域的
//这时候回看info数组的定义
//数组里存储的正是bootmm_info

void insert_mminfo(struct bootmm *mm, unsigned int start,
		unsigned int end, unsigned int type)
{
	unsigned int index;

	for(index = 0; index < mm->cnt_infos; ++index){
		if(mm->info[index].type != type)
			continue;
		if(mm->info[index].end_pfn == start - 1) {
			if((index + 1) < mm->cnt_infos) {
				if(mm->info[index+1].type != type)
					goto merge1;
				if(mm->info[index+1].start_pfn - 1 == end) {
					mm->info[index].end_pfn = 
						mm->info[index+1].end_pfn;
					remove_mminfo(mm, index);
					return;
				}
			}
merge1:
			mm->info[index].end_pfn = end;
			return;
		}
	}
	set_mminfo(mm->info + mm->cnt_infos, start, end, type);
	++(mm->cnt_infos);
}

//实现内存的释放
//的一部分，对info数组进行操作，将一个元素分裂成两个
void split_mminfo(struct bootmm *mm, unsigned int index, unsigned int split_start)
{
	unsigned int start, end;
	unsigned int tmp;

	start = mm->info[index].start;
	end = mm->info[index].end;
	split_start &= (~((1 << PAGE_SHIFT) - 1));//对分裂位置split_start进行对齐操作

	if((split_start <= start) || (split_start >= end))//检查要分裂位置是否属于第index个内存区域
		return;

	for(tmp = mm->cnt_infos - 1; tmp >= index; --tmp){
		mm->info[tmp+1] = mm->info[tmp];
	}
	mm->info[index].end = split_start - 1;
	mm->info[index+1].start = split_start;
	++(mm->cnt_infos);
}


void init_bootmm()
{
	unsigned int index;
	unsigned char *t_map;

	boot_mm.phymm = get_phymm_size();
	boot_mm.max_pfn = boot_mm.phymm >> PAGE_SHIFT;//用来代替昂贵的除法，这条语句相当于作除法

	boot_mm.s_map = _end + ((1 << PAGE_SHIFT) -1);
	boot_mm.s_map = (unsigned char *)((unsigned int)(boot_mm.s_map) & (~((1 << PAGE_SHIFT) -1)));

	boot_mm.e_map = boot_mm.s_map + boot_mm.max_pfn;
	boot_mm.e_map += ((1 << PAGE_SHIFT) - 1);
	boot_mm.e_map = (unsigned char *)((unsigned int)(boot_mm.e_map) & (~((1 << PAGE_SHIFT) -1)));

	boot_mm.cnt_infos = 0;
	memset(boot_mm.s_map, PAGE_USED, boot_mm.e_map - boot_mm.s_map);
	insert_mminfo(&boot_mm, 0, (unsigned int)(boot_mm.s_map - 1), _MM_KERNEL);
	insert_mminfo(&boot_mm, (unsigned int)(boot_mm.s_map), (unsigned int)(boot_mm.e_map - 1), _MM_MMMAP);
	boot_mm.last_alloc = (((unsigned int)(boot_mm.e_map) >> PAGE_SHIFT) - 1);

	for(index=((unsigned int)(boot_mm.e_map) >> PAGE_SHIFT);index < boot_mm.max_pfn; ++index){
		boot_mm.s_map[index] = PAGE_FREE;
	}


//set_maps函数的作用是将页框号为s_pfn开始的cnt个连续内存页面在内存位图中对应项的值设置为val
void set_maps(unsigned int s_pfn, unsigned int cnt, unsigned int val)
{
	while(cnt) {
		boot_mm.s_map[s_pfn] = val;
		--cnt;
		++s_pfn;
	}
}	

//当实现完了bootmem管理器后，作为一款内存管理器还需要一组内存分配与释放接口，首先是找到空闲页
//函数根据内存分配位图找出count个连续的空闲物理页面，并且这些物理页面的页框号必须在s_pfn与e_pfn之间
unsigned char *find_pages(unsigned int count, unsigned int s_pfn, unsigned int e_pfn, unsigned int align_pfn)
{
	unsigned int index, tmp;
	unsigned int cnt;

	s_pfn += (align_pfn - 1);//进行对齐处理
	s_pfn &= ~(align_pfn - 1);

	for(index=s_pfn; index < e_pfn;) {
		if(boot_mm.s_map[index]){
			++index;
			continue;
		}
		tmp = index;
		cnt = count;
		while(cnt) {
			if(tmp>=e_pfn)
				goto end;
			
			if(boot_mm.s_map[tmp])
				goto next;

			++tmp;
			--cnt;
		}

		boot_mm.last_alloc = index + count - 1;
		set_maps(index, count, PAGE_USED);

		return (unsigned char *)(index << PAGE_SHIFT);
next:
		index = tmp + align_pfn;
	}
end:
	return 0;
}

void bootmap_info()
{
	unsigned int index;
	printk("Mem Map :\n");
	for(index=0; index < boot_mm.cnt_infos; ++index) {
		printk("\t%x-%x : %s\n", boot_mm.info[index].start_pfn, boot_mm.info[index].end_pfn, mem_msg[boot_mm.info[index].type]);
	}
}

unsigned char *bootmm_alloc_pages(unsigned int size, unsigned int type, unsigned int align)
{
	unsigned int index, tmp;
	unsigned int cnt, t_cnt;
	unsigned char *res;

	size += ((1 << PAGE_SHIFT) - 1);
	size &= (~((1  << PAGE_SHIFT) -1));
	cnt = size >> PAGE_SHIFT;

	res = find_pages(cnt, bootmm.last_alloc + 1, boot_mm.max_pfn, align >> PAGE_SHIFT);
	if(res){
		insert_mminfo(&boot_mm, (unsigned int)res, (unsigned int)res + size -1,type);
		return res;
	}

	res = find_pages(cnt, 0, boot_mm.last_alloc, align >> PAGE_SHIFT);//从上一次内存分配的地方开始找
	if(res)
		insert_mminfo(&boot_mm, (unsigned int)res, (unsigned int)res + size -1, type);
	return res;
}

//内存释放功能
void bootmm_free_pages(unsigned int start, unsigned int size) 
{
	unsigned int index, cnt;

	size &= ~((1 << PAGE_SHIFT) -1);//通过size计算出相应的页框数目
	cnt = size >> PAGE_SHIFT;
	if(!cnt)
		return;

	start &= ~((1 << PAGE_SHIFT) -1);
	for(index = 0; index < boot_mm.cnt_infos; ++index) {
		if(boot_mm.info[index].end < start)
			continue;
		if(boot_mm.info[index].start > start)
			continue;
		if(start + size -1 > boot_mm.info[index].end)
			continue;
		break;
	}
	if(index == boot_mm.cnt_infos) {
		printk("bootmm_free_pages : not alloc space(%x, %x) \n", start, size);
		return;
	}

	set_maps(start >> PAGE_SHIFT, cnt, PAGE_FREE);
	if(boot_mm.info[index].start == start) {
		if(boot_mm.info[index].end == (start + size - 1))
			remove_mminfo(&boot_mm, index);
		else
			set_mminfo(&(boot_mm.info[index]), boot_mm.info[index].end, start + size - 1, boot_mm.info[index].type);
	}else{
		if(boot_mm.info[index].end == (start + size - 1))
			set_mminfo(&(boot_mm.info[index]), start + size,
					boot_mm.info[index].end,
					boot_mm.info[index].type);
		else{
			split_mminfo(&boot_mm, index, start);
			split_mminfo(&boot_mm, index+1, start+size);
			remove_mminfo(&boot_mm, index + 1);
		}
	}
}



}
