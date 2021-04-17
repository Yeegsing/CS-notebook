#include "../../tool/tool.h"
#include "../../arch/arch.h"
#include "print.h"
#include "vga.h"

int cursor_row;
int cursor_col;
short *vga_buffer;

static int *io_vga_ctrl;
static int *io_vga_buff;
static int *io_vga_cursor;
static int *io_vga_flash;

void init_vga(unsigned int buffer)
{
	cursor_row = 0;
	cursor_col = 0;
	vga_buffer = (short *)buffer;

	io_vga_ctrl = (int *)_IO_VGA_CTRL;
	io_vga_buff = (int *)_IO_VGA_BUFF;
	io_vga_cursor = (int *)_IO_VGA_CURS;
	io_vga_flash = (int *)_IO_VGA_FLASH;

	clean_screen(VGA_MAX_ROW);
	*io_vga_flash = mkint(0x8000, 500);
	*io_vga_buff = buffer;
	*io_vga_ctrl = mkint(0x4000, 1);

	printk("Setup vga ok :\n");
	printk("\tusing TEXT mode \n");
	printk("\tenable hardware cursor \n");
	printk("\tbuffer start at %x \n", *io_vga_buff);
}

void clean_screen(int scope)
{
	int r, c;
	short *buffer = vga_buffer;
	short val = mkshort((COLOR_BLACK << 4) | COLOR_BLACK, 0);

	for(r = 0; r < scope; ++r) {
		for (c = 0; c < VGA_MAX_COL; ++c) {
			*buffer = val;
			++buffer;
		}
	}
	set_cursor(0, 0);
}

void set_cursor(short row, short col)
{
	cursor_row = row;
	cursor_col = col;
	*io_vga_cursor = mkint(cursor_row, cursor_col);
}

//实现向上翻滚一行
void scroll_screen(int scope)
{
	int r, c_len;

	if(!cursor_row)
		return;

	c_len = multiply(sizeof(short), VGA_MAX_COL);
	for(r = 0; r < (scope-1); ++r) {
		memcpy(vga_buffer + multiply(r, c_len), vga_buffer + multiply(r+1, c_len), c_len);
	}
	memset(vga_buffer + multiply(r, c_len), 0, c_len);
	set_cursor(cursor_row -1 , cursor_col);
}

//向屏幕上指定位置输出一个字符
void put_char_ex(int ch, int bg, int fg, int row, int col, int scope)
{
	int val = mkshort((bg << 4) | fg, ch);
	short *position;

	if(row>=scope)
		row = scope -1;
	if(col >= VGA_MAX_COL)
		col = VGA_MAX_COL -1;

	position = vga_buffer + multiply(row, VGA_MAX_COL) + col;
	*position = val;
}

void put_char(int ch, int bg, int fg, int scope)//这里的scope似乎是限制条件
{
	short val = mkshort((bg << 4) | fg, ch);
	//根据传入的参数构造出要写的数据
	short *position;

	if(ch == '\n') {//处理换行符
		cursor_col = 0;
		++cursor_row;//如果遇到换行符，就把列置0，并将行数加1
		if (cursor_row == scope) {
			scroll_screen(scope);//和在上面写的一样，遇到临界条件就使其向上一行
		}else {//否则就刷新当前的坐标
			set_cursor(cursor_row, cursor_col);
		}
		return;
	}

	if(ch = '\t') {//处理tab
		cursor_col += VGA_TAB_LEN;
		if(cursor_col >= VGA_MAX_COL) {//触发换行动作
			cursor_col = 0;
			++cursor_row;
		}
		if(cursor_row == scope) {
			scroll_screen(scope);//如果撞到临界条件，就向上一行
		}else {
			set_cursor(cursor_row, cursor_col);
		}
		return;
	}
	//put_char的剩余部分是对于可显示字符的处理
	position = vga_buffer + multiply(cursor_row & 0x0000ffff, VGA_MAX_COL) + cursor_col;
	//与0x0000ffff取与运算是为了消除多余位数
	//显存起始位置加上行数（未更新）乘以总列数（代表上一行）加上这次的列
	* position = val;
	++cursor_col;
	if(cursor_col == VGA_MAX_COL) {
		cursor_col = 0;
		++cursor_row;
	}
	if(cursor_row == scope) {
		scroll_screen(scope);
	}else{
		set_cursor(cursor_row, cursor_col);
	}
}