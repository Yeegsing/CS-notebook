static char *imap = "0123456789ABCDEF";

int printk(char *fmt...)
{
	int argint;
	char argch;
	char *argstr;
	char *pfmt;//创建的临时变量，是指针
	int index;
	va_list vp;

	va_start(vp, fmt);
	pfmt = fmt;//都是指针，相当于指向同一个地址
	index = 0;
	while(*pfmt) {
		if(*pfmt == '%') {
			switch(*(++pfmt)){
				case'c':
				case'C':
					argch = va_arg(vp, int);
					index += print_char(argch);
					break;
				case's':
				case'S':
					argstr = va_arg(vp, char *);
					index += print_str(argstr);
					break;
				case'b':
				case'B':
					argint = va_arg(vp, int);
					index += print_binary(argint);
					break;
				case'x':
				case'X':
					argint = va_arg(vp, int);
					index += print_hex(argint);
					break;
				case'%':
					index += print_char('%');
					break;
				default:
					break;
			}
			++pfmt;
		}else{
			index += print_char(*pfmt);
			++pfmt;
		}
	}
	va_end(vp);
	return index;
}

int print_char(char ch)
{
	put_char(ch, COLOR_BLACK, COLOR_WHITE, VGA_ROW_CONSOLE);
	return 1;
}

//print_str依次对字符串中每个字符调用print_char进行处理
int print_str(char *s)
{
	while(*s){
		print_char(*s);
		++s;
	}
}

int print_binary(int i)
{
	if(!i){
		print_str("0b0");
		return;
	}
	print_binary(i >> 1);
	print_char((char)(imap[i%2]));
}