unsigned int get_emptypid()
{
	unsigned int no = 0;
	unsigned int tmp;
	unsigned int index, bits;
	for(index = 0; index < IDMAP_MAX; ++index) {
		tmp = idmap[index];
		for(bits = 0; bits < sizeof(unsigned char); ++bits){
		if(!(tmp & 0x01)){
			idmap[index] |= bits_map[bits];
			break;
		}
		tmp >>= 1;
		++no;
	}
	if(bits < sizeof(unsigned char))
		break;	
	}
return no;
}

unsigned int do_one_mapping(unsigned int *pgd, unsigned int va, unsigned int pa, unsigned int attr)
{
	unsigned int pde_index = (va >> PGD_SHIFT) & INDEX_MASK;
	unsigned int pte_index = (va >> PTE_SHIFT) & INDEX_MASK;
	unsigned int *pt;

	pt = (unsigned int *)kmalloc(PAGE_SIZE);
	if(pt == NULL)
		return 1;

	pgd[pde_index] = (unsigned int)pt & (~OFFSET_MASK);
	pgd[pde_index] |= attr;
	pa &= (~OFFSET_MASK);
	pa |= attr;
	pt[pte_index] = pa;

	return 0;
}


//进行初始化
void init_task(void *phy_code)
{
	INIT_LIST_HEAD(&tasks);
	memset(idmap, 0, 16 * sizeof(unsigned char));

	init = (union task_union *)(KERN_STACK_BOTTOM - KERN_STACK_SIZE);
	if(!init)
		goto error;

	init->task.pid = get_emptypid();
	if(inavailed_pid(init->task.pid))
		goto error;

	init->task.parent = init->task.pid;
	init->task.user_stack = _TASK_USER_STACK;
	init->task.user_code = _TASK_CODE_START;
	init->task.state = _TASK_UNINIT;
	init->task.counter = _DEFAULT_TICKS;
	init->task.pgd = pgd;

	INIT_LIST_HEAD(&(init->task.stack_vma_head));
	INIT_LIST_HEAD(&(init->task.user_vma_head));
	INIT_LIST_HEAD(&(init->task.heap_vma_head));
	INIT_LIST_HEAD(&(init->task.node));
	INIT_LIST_HEAD(&(init->task.sched));
	add_tasks(&(init->task.node));

	if(add_vmas(&(init->task), init->task.user_code, PAGE_SIZE)){
		printk("init_task : add_vmas failed!\n");
		die();
	}

	if(do_one_mapping(init->task.pgd, init->task.user_code, phy_code, USER_DEFAULT_ATTR)){
		printk("Init task-0 failed : do_one_mapping failed!\n");
		die();
	}

	memset(init->task.sigaciton, 0, MAX_SIGNS * sizeof(struct signal));
	init->task.state = _TASK_RUNNING;

	printk("Init task-0 ok : \n");
	printk("\tpid : %x\n", init->task.pid);
	printk("\tphy_code : %x\n", phy_code);
	printk("\tkernel stack's end at %x\n", init);
	return;
error:
	printk("Init task-0 failed!\n");
	die();
}

void * copy_pagetables()
{
	//通过kmalloc函数分配一个页面作为子进程的第一级页表，得到当前进程的第一级页表，
	//并将其内容复制到刚才分配的内存页中
	unsigned unsigned int *old_pgd = NULL;
	unsigned unsigned int *pgd = NULL;
	unsigned unsigned int *tmp_pt;
	unsigned unsigned int *old_pt;
	unsigned unsigned int index, ptnr;
	unsigned unsigned int attr;

	pgd = (unsigned unsigned int *)kmalloc(PAGE_SIZE);
	if(pgd == NULL)
		goto error1;

	old_pgd = current->pgd;
	memcpy(pgd, old_pgd, PAGE_SIZE);

	//设置子进程的第二级页表
	//通过一个for循环遍历父进程第一季页表中的用户控件部分
	for(index = (_USER_VIRT_START >> PAGE_SHIFT), ptnr = 0;
			index < (PAGE_SIZE >> 2); ++index){
		if(old_pgd[index]) {
			tmp_pt = (unsigned unsigned int *)kmalloc(PAGE_SIZE);
			if(tmp_pt == NULL)
				goto error2;

			++ptnr;
			pgd[index] &= OFFSET_MASK;
			pgd[index] |= (unsigned unsigned int)tmp_pt;
			//填充新页表
			old_pt = old_pgd[index] & (~OFFSET_MASK);
			memcpy(tmp_pt, old_pt, PAGE_SIZE);
			//取消写权限
			clean_W(&(pgd[index]));
		}
	}

	//在上面代码对子进程进行设置的同时，为了保证进程数据的正确性
	//还需要取消父进程中相关页表的写权利，否则父进程仍然可以修改
	for(index = (_USER_VIRT_START >> PAGE_SHIFT), ptnr = 0;
			index < (PAGE_SIZE >> 2); ++index){
		if(old_pgd[index])
			clean_W(&(old_pgd[index]));

		if(is_P(&(old_pgd[index]))){
			inc_refrence(pages + (old_pgd[index] >> PAGE_SHIFT), 1);
			inc_refrence_by_pt(old_pgd[index] & (~OFFSET_MASK));
		}
	}

	return pgd;

	//出错之后的处理
error2:
	if(ptnr){
		for(index = (_USER_VIRT_START >> PAGE_SHIFT);
				(index < (PAGE_SIZE >> 2)) && ptnr; ++index, --ptnr) {
			if(pgd[index]){
				old_pt = old_pgd[index] & (~OFFSET_MASK);
				tmp_pt = pgd[index] & (~OFFSET_MASK);
				if(old_pt == tmp_pt)
					kfree(tmp_pt);
			}
		}
	}
error1:
	kfree(pgd);
	return NULL;
}



//get_emptypid的作用实现
//外层循环时从前往后从进程号位图idmap中取出一个字节
//内层循环是对这个字节的每一位进行检查，如果发现该位为0，则说明其对应的进程号没有被占用
unsigned int get_emptypid()
{
	unsigned int no = 0;
	unsigned int tmp;
	unsigned int index, bits;
	for(index = 0; index < IDMAP_MAX; ++index){
		tmp = idmap[index];
		for(bits = 0; bits < sizeof(unsigned char); ++bits)
		{
		if(!(tmp & 0x01)){
			idmap[index] |= bits_map[bits];
			break;
		}
		tmp >>= 1;
		++no;
		}
	if(bits < sizeof(unsigned char))
		break;

	}
	return no;
}

void do_fork(unsigned unsigned int *args)
{
	union task_union *new;
	struct regs_context *ctx;
	unsigned unsigned int res;

	ctx = (struct regs_context *)args;
	new = copy_mem((union task_union *)current);
	if(!new) {
		ctx->v0 = -1;
	}else{
		ctx->v0 = new->task.pid;
		ctx = (unsigned unsigned int)args - (unsigned unsigned int)current;
		ctx = (struct regs_context *)((unsigned unsigned int)new + (unsigned unsigned int)ctx);
		ctx->v0 = 0;
	}
}

union task_union *copy_mem(union task_union *old)
{
	union task_union *new = NULL;
	unsigned unsigned int *pgd;
	unsigned unsigned int new_pid = get_emptypid();
	if(inavailed_pid(new_pid)){
		printk("copy_mem failed : inavailed new pid\n");
		goto error1;
	}

	new = (union task_union *)kmalloc(sizeof(union task_union));
	if(!new){
		printk("copy_mem failed : kmalloc return NULL\n");
		goto error2;
	}

	if(!(pgd = copy_pagetables())){
		printk("copy_mem failed : copy_pagetables failed\n");
		goto error3;
	}

	memcpy(new, old, sizeof(union task_union));
	new->task.state = _TASK_BLOCKED;
	new->task.parent = old->task.pid;
	new->task.pid = new_pid;
	new->task.counter = _DEFAULT_TICKS;
	new->task.pgd = pgd;
	INIT_LIST_HEAD(&(new->task.node));
	INIT_LIST_HEAD(&(new->task.sched));
	add_tasks(&(new->task.node));
	//一般新创建的进程优先执行，所以放到队列头部
	sched_insert_head(&ready_tasks, &(new->task.sched));
	new->task.state = _TASK_READY;

	return new;
error1:
	return NULL;
error2:
	put_pid(new_pid);
error3:
	kfree(new);
}


