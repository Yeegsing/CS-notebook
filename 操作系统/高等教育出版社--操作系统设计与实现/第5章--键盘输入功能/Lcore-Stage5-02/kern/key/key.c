struct keyb_buffer {
	unsigned char buffer[KEYBUFF_SIZE];
	struct lock_t key_spin;//是一个自旋锁，用于处理同步互斥
	struct list_head wait;//是一个链表头，用于记录那些因为等待键盘输入而被挂起的进程
	unsigned int head, tail;//head表示buffer数组中下一个添加数据的位置，而tail表示buffer中最后一个字符的位置
	unsigned int count;
};

struct keyb_buffer keyb_buffer;
static unsigned int key_index;
unsigned char *io_key_data;
unsigned int f0;
unsigned char keymap[112] = {
};

struct intr_work scancode_work;

void init_keyboard()
{
	io_key_data = (unsigned char *)_IO_KEYB_DATA;

	key_index = highest_set(_INTR_KEYB);
	if(register_handler(keyb_handler, key_index))
		return;

	scancode_work.work = get_scancode;
	INIT_LIST_HEAD(&scancode_work.node);
	if(register_work(&(scancode_work.node), key_index))
		return;

	keyb_buffer.count = 0;//加入了对keyb_buffer全局变量的初始化
	keyb_buffer.head = 0;
	keyb_buffer.tail = 0;
	memset(keyb_buffer.buffer, 0, KEYBUFF_SIZE);
	init_lock(&(keyb_buffer.key_spin));//初始化自旋锁
	INIT_LIST_HEAD(&(keyb_buffer.wait));//初始化等待链表头

	f0 = 0;

	printk("Setup Keyboard ok: \n");
	printk("\tkeyboard-buffer size %x Bytes \n", KEYBUFF_SIZE);

	//实现get_scancode
	void get_scancode()
	{
		unsigned char ch = *io_key_data;

		if(keyb_buffer.count >= KEYBUFF_SIZE)//如果缓冲区已满，则丢弃读到的数据
			goto out;

		if(ch == 0xf0){
			f0 = 1;
			goto out;
		}else if(ch >= 0x70) {
			goto out;
		}

		if(!f0)
			goto out;

		if(!keymap[ch])
			goto out;

		keyb_buffer.buffer[keyb_buffer.head] = keymap[ch];
		if(!(keyb_buffer.count)) {
			return;//唤醒队列中所有的进程
		}
	}
	++keyb_buffer.count;
	++keyb_buffer.head;
	keyb_buffer.head %= KEYBUFF_SIZE;

	f0 = 0;
out:
	return;
}

//实现get_ch函数
void get_ch(unsigned char *buf)
{
	lockup(&(keyb_buffer.key_spin));
	if(!(keyb_buffer.count)) {
		return;
	}

	*buf = keyb_buffer.buffer[keyb_buffer.tai];
	--keyb_buffer.count;
	++keyb_buffer.tail;
	keyb_buffer.tail %= KEYBUFF_SIZE;
	unlock(&(keyb_buffer.key_spin));
}
