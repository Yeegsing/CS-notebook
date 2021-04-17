//作用是创建一个vma对象，两个参数va代表要穿件的线性区间对象的起始地址，vend表示要创建的线性区间对象的结束地址
struct vma *new_vma(unsigned int va, unsigned int vend)
{
	struct vma *tmp;

	tmp = kmalloc(sizeof(struct vma));//使用第六章实现的内存分配接口kmalloc对vma对象所占用的内存空间进行分配
	if(!tmp)
		return NULL;

	INIT_LIST_HEAD(&(tmp->node));//分配成功就对其进行初始化操作
	tmp->start = va;
	tmp->vend = vend;
	tmp->cnt = ((vend-va) >> PAGE_SHIFT) + 1;

	return tmp;
}

//把一个vma对象分割为两个vma对象
//将vma对象分成vma->start~(va-1)和va~vma->vend两个对象
struct vma *seprate_vma(struct vma *vma, unsigned int va)
{
	struct vma *new;

	if(!(new = new_vma(va, vma->vend))){
		printk("seprate_vma : new_vma failed!\n");
		return NULL;
	}

	vma->vend = va - 1;
	list_add(&(new->node), vma->node.next);

	return new;
}

//1.用来检查传入的线性区间范围是否合法，如果合法返回0，否则返回1
//2.对传入的线性地址区间进行对齐操作，并获得其所属类型对应的链表首部
unsigned int verify_vma(struct task_struct * task, unsigned int *va, unsigned int *vend, struct list_head **target)
{
	*target = NULL;

	(*va) &= ~(PAGE_SIZE - 1);
	(*vend) += (PAGE_SIZE - 1);
	(*vend) &= ~(PAGE_SIZE - 1);
	(*vend)--;
	//以上代码块是在对数据进行对齐操作
	if((*va) < _KERNEL_VIRT_END) {
		printk("add_vmas : va(%x) inavailed(kernel space)!\n")
			return 1;
	}else if((*va) < _TASK_CODE_START) {
		if((*vend) >= _TASK_CODE_START){
			printk("add_vmas: vma(%x:%x) exceed!\n", (*va), (*vend));
			return 1;
		}else{
			*target = &(task->stack_vma_head);
		}
	}else if((*va) < _TASK_HEAP_END) {
		if((*vend) >= _TASK_HEAP_END) {
			printk("add_vmas : vma(%x,%x) exceed!\n", (*va), (*vend));
			return 1;
		}else{
			*target = &(task->user_vma_head);
		}
	}else if((*va) < ROM_START) {
		if((*vend) >= ROM_START) {
			printk("add_vmas : vma(%x:%x) exceed!\n", (*va), (*vend));
			return 1;
		}else{
			*target = &(task->heap_vma_head);
		}
	}else{
		printk("add_vmas : vm(%x) inavailed(rom/io space)!\n");
		return 1;
	}
	return 0;
}

//add_vmas的作用是向进程的线性地址区间中添加一个vma对象
unsigned int add_vmas (struct task_struct *task, unsigned int va, unsigned int size)
{
	struct list_head *target;
	struct list_head *pos;
	struct list_head *n;
	struct vma *tmp;
	struct list_head *prev;
	unsigned int vend = va + size;

	if(verify_vma(task, &va, &vend, &target))
		return 1;

	prev = target;
	list_for_each_safe(pos, n, target) {
		tmp = container_of(pos, struct vma, node);
		if(va < tmp->start){
			if(vend < tmp->start){
				if((vend+1) == tmp->start) {
					tmp->start = va;
					tmp->cnt = ((tmp->vend - tmp->start) >> PAGE_SHIFT) +1;
					goto ok;
				}else{
					prev = tmp->node.prev;
					goto new;
				}
			}else if(vend >= tmp->vend) {
				del_vma(tmp);
				continue;
			}else{
				tmp->start = va;
				tmp->cnt = ((tmp->vend - tmp->start) >> PAGE_SHIFT) + 1;
				goto ok;
			}
		}else if(va < tmp->vend){
			if(vend <= tmp->vend){
				goto ok;
			}else{
				va = tmp->start;
				del_vma(tmp);
				continue;
			}
		}else{
			if((va-1) == tmp->vend){
				va = tmp->start;
				del_vma(tmp);
				continue;
			}else{
				prev = &(tmp->node);
				continue;
			}
		}
	}
new:
	if(!(tmp = new_vma(va, vend))){
		printk("insert_vma: new_vma failed!\n");
		return 1;
	}

	list_add(&(tmp->node), prev);
ok:
	return 0;
}


//实现一个删除vmas的函数
unsigned int delete_vmas(struct task_struct *task, unsigned int va, unsigned int vend)
{
	struct list_head *pos;
	struct vma *tmo, *n;
	struct list_head *list;

	if(verify_vma(task, &va, &vend, &list))
		return 1;

	list_for_each(pos, list) {
		tmp = container_of(pos, struct vma, node);
		if(va < tmp->start) {
			return 1;
		}else if(va == tmp->start){
			if(vend == tmp->vend){
				del_vma(tmp);
				return 0;
			}else if(vend < tmp->vend){
				tmp->start = vend + 1;
				return 0;
			}else{
				return 1;
			}
		}else if(va < tmp->vend){
			if(vend == tmp->vend){
				tmp->vend = va - 1;
				return 0;
			}else if(vend < tmp->vend){
				n = seprate_vma(tmp, va);
				if(!n){
					printk("delete_vmas : seprate_vma failed!\n");
					return 1;
				}
				tmp = n;
				n = seprate_vma(n, vend + 1);
				if(!n) {
					printk("delete_vmas : seprate_vma failed!\n");
					return 1;
				}
				del_vma(tmp);
				return 0;
			}else{
				return 1;
			}
		}else{
			continue;
		}
	}
	return 0;
}
