#ifndef _LCORE_VGA_H
#define _LCORE_VGA_h

#define VGA_MAX_ROW 30
#define VGA_MAX_COL 80

#define VGA_TAB_LEN 4

/*
*屏幕上半部（28行）用于控制台的输入输出
*下半部分（2行）用于系统实时信息显示
*/
#define VGA_ROW_SYSTEM 2
#define VGA_ROW_CONSOLE(VGA_MAX_ROW - VGA_ROW_SYSTEM)

extern int cursor_row;
extern int cursor_col;
extern short *vga_buffer;

extern void init_vga(unsigned int buffer);
extern void clean_screen(int scope);
extern void set_cursor(short row, short col);