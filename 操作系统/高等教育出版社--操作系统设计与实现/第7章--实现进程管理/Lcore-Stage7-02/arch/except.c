void do_pg_unpresent(unsigned unsigned int errArg, unsigned unsigned int errPc, unsigned unsigned int *regs)
{
	void *newpg;

	if(errArg < _KERNEL_VIRT_END){
		printk("Error : task(pid:%x) want to access addr:%x(pc:%x),", current->pid, errArg, arrPc);
		printk("but it belongs to kernel address spcae.\n");
		printk("Kill this task(pid:%x)!\n", current->pid);
		die();
	}else if(errArg < _TASK_HEAP_END){
	printk("Warn : task(pid:%x) miss code/data addr: %x, pc:%x", current->pid, errArg, errArg, arrPc);
	die();
	}else if(errArg < ROM_START){
	newpg = kmalloc(PAGE_SIZE);
	if(!newpg){
		printk("do_pg_unpresent : kmalloc failed. (task-pid:%x, errarg:%x, errpc:%x)\n", current->pid, errArg, errPc);
		die();
	}

	errArg &= ~(PAGE_SIZE - 1);

	if(add_vmas(current, errArg, PAGE_SIZE)){
		printk("do_pg_unpresent : kmalloc failed. (task-pid:%x, errarg:%x, errpc:%x)\n", current->pid, errArg, errPc);
		kfree(newpg);
		die();
	}
	do_one_mapping(current->pgd, errArg, PAGE_SIZE, USER_DEFAULT_ATTR);
	}else{
		print/..
			die();
	}
}

//将实现对页写保护异常的处理，比如说在进程之间共享页面
//同时进行读操作不会产生异常，但是同时进行写操作却需要复制一个副本，在副本上进行相关修改
void do_copyonwrite(unsigned int errArg, unsigned int errPc, unsigned int *regs)
{
	//errArg表示缺页异常引起的线性地址，errPc表示引起缺页异常的指令指针
	//regs指向发生缺页异常进程的寄存器信息
	//实验平台支持的是两级页表机制
	//即发生页保护异常时第一级页表是存在即可写的，由第二级页表来进行写保护控制
	unsigned int *pgd = current->pgd;
	unsigned int pgd_index = errArg>>PGD_SHIFT;
	unsigned int pte_index = (errArg >> PAGE_SHIFT) & 0x3FF;
	unsigned int *pt;
	unsigned int index, pg;
	struct page *old;
	void *new;

	if(errArg < _KERNEL_VIRT_END || errArg >= ROM_START){
		printk("Error : task(pid:%x) want write addr:%x),", current->pid, errArg, errPc);
		print("it must through kernel mode.\n");//?
		goto kill;
	}

	if(!pgd[pgd_index]){//首先检查第一级页表中对应的表项是否是空的
		printk("Impossible : pde is 0 while WriteFault\n");
		goto kill;
	}

	if(!is_P(&(pgd[pgd_index]))){//再检查该表项信息中的P位（即存在位）是否为0，如果是0就说明第二级页表不存在
		printk("Impossible : unpresent while WriteFault\n");
		goto kill;
	}

	if(!is_W(&(pgd[pgd_index]))){//最后检查第一级中对应的页表项是否可写，如果不可写，同样杀死进程。
		printk("Impossible : pde non-wrtie while WriteFault\n");
		goto kill;
	}

	pt = (unsigned int *)(pgd[pgd_index] & (~((1 << PAGE_SHIFT)-1)));

	if(!pt[pte_index]){
		printk("Impossible : pte is 0 while WriteFault\n");
		goto kill;
	}

	if(!is_P(&(pt[pte_index]))){
		printk("Impossible : unpresent while WriteFault\n");
		goto kill;
	}


	if(!is_W(&(pt[pte_index]))){
		printk("Impossible : pte wrtie while WriteFault\n");
		goto kill;
	}

	pg = pt[pte_index] & (~((1 << PAGE_SHIFT) -1 ));//得到发生页保护异常的线性地址对应的物理页面的struct page结构信息
	old = pages + (pg >> PAGE_SHIFT);
	if(old->refrence == 1){//如果该页面的引用计数为1，表示该物理页面没有被共享，只有该线性地址映射了它
		set_W(&(pt[pte_index]));//于是将写权限位置为1
		goto ok;
	}

	new = kmalloc(PAGE_SIZE);//如果被共享了，就需要重新分配一块空闲的物理页面给它
	if(!new){
		printk("do_copyonwrite : kmalloc failed!\n");
		goto kill;
	}

	dec_refrence(old, 1);//同时将引用计数减1
	memcpy(new, (void *)pg, PAGE_SIZE);//并且将此前物理页面的内容复制到新分配的物理内存中
	pt[pte_index] &= ((1 << PAGE_SHIFT) - 1);
	pt[pte_index] |= (unsigned int)new;
	set_W(&(pt[pte_index]));

ok:
	flush_tlb(pgd);
	return;

kill:
	pritnk("KILL this task(pid:%x)!\n", current->pid);
	die();
}
