struct task_struct *sched_remove_first(struct list_head *head)
{
	struct task_struct *task;
	struct list_head *ptr;

	if(list_empty(head))
		return NULL;

	ptr = head->next;
	list_del_init(ptr);
	task = container_of(ptr, struct task_struct, sched);

	return task;
}

void sched(unsigned int *regs, unsigned int status, unsigned int errArg, unsigned int errPc)
{
	struct task_struct *next;
	struct task_struct *old;

	current->state = _TASK_READY;
	sched_insert_tail(&ready_tasks, &(current->sched));

	next = sched_remove_first(&ready_tasks);
	if(!next){
		printk("scheduler : next task is NULL!\n");
		die();
	}

	if(next != current){
		old = current;
		current = next;
		switch_to(&(old->context), &(current->context));
	}
}

//Lcore内核使用的是最简单的时间片轮转调度策略
void timer_sched(unsigned int *regs, unsigned int status, unsigned int errArg, unsigned int errPc)
{
	--(current->counter);//时间片减
	if(current->counter)//时间没用完，还能运行
		return;

	current->counter = _DEFAULT_TICKS >> 1;//否则就需要调度，首先重新分配时间片
	sched(regs, status, errArg, errPc)//再调度sched函数执行调度功能
}

void init_sched()
{
	INIT_LIST_HEAD(&ready_tasks);
	INIT_LIST_HEAD(&block_tasks);

	current = &(init->task);

	sched_work.work = timer_sched;
	INIT_LIST_HEAD(&sched_work.node);
	if(register_work(&(sched_work.node), timer_index))
		return;

	printk("Init Sched ok\n");
	printk("\tcurrent task : %x\n", current);
}
