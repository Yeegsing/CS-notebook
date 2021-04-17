//时间中断实现

static int sys_time_total;//用来记录总共更新了多少次时间
static int sys_time_hours_x;
static int sys_time_hours_y;
static int sys_time_minutes_x;
static int sys_time_seconds_x;
static int sys_time_seconds_y;
static int sys_time_msecs;//用来记录当前系统总共运行时间



void timer_handler()
{
	struct list_head *pos;
	struct intr_work *entry;

	//在time_handler函数中将遍历其任务链表，然后执行该任务
	list_for_each(pos, &(interrupt[time_index].head)) {
		entry = list_entry(pos, struct intr_work, node);
		entry->work();
	}
}

//下一步需要实现的是将time_handler函数和interrupt数组关联起来，以告知系统在do_interrupt函数中由谁来调用处理
//时钟中断


//init_time将在系统初始化函数init_kernel中被调用，用来对时间中断处理进行初始化
void init_time(int interval)
{
	time_index = highest_set(_INTR_CLOCK);

	if(register_handler(timer_handler, time_index))
		return;

	info_work.work = flush_systime;//加入一个任务实体
	INIT_LIST_HEAD(&info_work.node);
	if(register_work(&(info_work.node), time_index))
		return;

	set_timer(interval);//用来设置时钟中断的触发中断，就是会每隔interval毫秒触发一次时钟中断
	init_systime();//初始化系统时间来管理相关的全局变量，后面会讨论

	printk("Setup Timer ok:\n");
	printk("\tregister TIMER's handler at %x \n", timer_handler);
	printk("\tregister first work(%x) \n", &info_work);
	printk("\tset timer %x ms \n", interval);
}

void init_systime()
{
	sys_time_total = 0;
	sys_time_hours_x = 0;
	sys_time_hours_y = 0;
	sys_time_minutes_x = 0;
	sys_time_seconds_x = 0;
	sys_time_seconds_y = 0;
	sys_time_msecs = 0;

	echo_delimiter();
	echo_welcome();
	echo_version();
	echo_time();
}

//echo_time函数是用来更新时间显示的
static void echo_time()
{
	char t[16];
	int col;
	t[0] = imap[sys_time_hours_x];
	t[1] = imap[sys_time_hours_y];
	t[2] = ':';
	t[3] = imap[sys_time_minutes_x];
	t[4] = imap[sys_time_minutes_y];
	t[5] = ':';
	t[6] = imap[sys_time_seconds_x];
	t[7] = imap[sys_time_seconds_y];
	t[8] = 0;

	for(col = TIME_COL; t[col-TIME_COL]; ++col) {
		put_char_ex(t[col - TIME_COL], COLOR_BLACK, COLOR_RED, VGA_ROW_CONSOLE+1, col, VGA_MAX_ROW);
	}
}

//下面是系统时间更新函数flush_systime的具体实现
void flush_systime()
{
	++sys_time_total;
	sys_time_msecs += _CLOCK_INTERVAL;

	while(sys_time_msecs >= 1000) {
		++sys_time_seconds_y;
		if(sys_time_seconds_y == 10) {
			sys_time_seconds_y = 0;
			++sys_time_seconds_x;
			if(sys_time_seconds_x ==6) {
				sys_time_minutes_x = 0;
				++sys_time_seconds_y;
				if(sys_time_minutes_y == 10) {
					sys_time_minutes_y = 0;
					++sys_time_minutes_x;
					if(sys_time_minutes_x ==6) {
						sys_time_minutes_x = 0;
						++sys_time_hours_y;
						if(sys_time_hours_y == 10) {
							sys_time_hours_y = 0;
							++sys_time_hours_x;
						}
					}
				}
			}
		}
		sys_time_msecs -= 1000;//过去了一秒
		echo_time();//时间刷新函数
	}
}