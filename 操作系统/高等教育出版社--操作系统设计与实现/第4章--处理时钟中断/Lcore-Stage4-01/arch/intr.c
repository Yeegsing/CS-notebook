struct intr_block interrupt[_INTR_MAX];

void init_exint()
{
	int entry = _INTR_ENTRY;
	int index;

	for(index = 0; index < ARRAY_SIZE(interrupt); ++index){
		interrupt[index].entry_fn = 0;
		INIT_LIST_HEAD(&interrupt[index].head);
	}

	memcpy(EXCEPT_ENTRY, &_exint_handler, ((char *)&_end_ex - (char *)&_exint_handler));
	//把_exint_handler对应的内容写到EXCEPT_ENTRY里
	//特殊就特殊在这个函数接受的是地址，是地址间的写入

	asm volatile (
		"mtc0 %0, $3"
		:
		:"r"(entry)
		);

	disable_intr(_INTR_GLOBAL | _INTR_CLOCK | _INTR_KEYB | _INTR_SPI);
}

void do_interrupt(int status, int errArg, int errPc, int *regs)
{
	int icr, ier, index;

	if(!(status & _INTR_FLAG))
		return;

//IER是中断使能寄存器
//ICR是中断来源寄存器
	ier = get_ier();
	icr = get_icr();
	icr = (icr & ier);

	if(icr & _INTR_CLOCK) {
		if(interrupt[0].entry_fn) {
			interrupt[0].entry_fn();
			clean_icr(_INTR_CLOCK);
		}
	}

		if(icr & _INTR_KEYB) {
		if(interrupt[3].entry_fn) {
			interrupt[3].entry_fn();
			clean_icr(_INTR_KEYB);
		}
	}

		if(icr & _INTR_SPI) {
		if(interrupt[5].entry_fn) {
			interrupt[5].entry_fn();
			clean_icr(_INTR_SPI);
		}
	}
}

//handler的注册与取消
int register_handler(intr_fn handler, int index)
{
	if(interrupt[index].entry_fn) {
		printk("KERN:register_handler failed [INT(%x) already registered!] \n", index);
		return 1;
	}

	//register_handler函数将中断处理函数handler注册到index号中断上，在该中断发生的时候就会调用handler函数
	interrupt[index].entry_fn = handler;
	return 0;
}

void unregister_handler(int index)
{
	if(!interrupt[index].entry_fn) {
		printk("WARN:INT(%x) already unregistered!\n", index);
		return;
	}

	interrupt[index].entry_fn = 0;
}

//work的注册与取消
int register_work(struct list_head *work_node, int index)
{
	if(interrupt[index].entry_fn) {
		printk("KERN:register_work failed [INT(%x)'s handler not defined!] \n", index);
		return 1;
	}

	//register_work函数将一个任务注册到index号中断上，在该中断发生的时候就会调用刚才注册的函数
	list_add_tail(work_node, &interrupt[index].head);
	return 0;
}

void unregister_work(struct list_head *work_node)
{
	if(!list_empty(work_node)) {
		printk("WARN:the WORK already unregistered!\n", index);
		return;
	}

	list_del_init(work_node);
}