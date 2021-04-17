/*
*清空屏幕的内容
*/

//较之先前的版本，这个clean_screen多了一个scope
//来确保可清楚0~scope-1的行
void clean_screen(unsigned int scope)
{
	unsigned int r, c;
	unsigned short *buffer = vga_buffer;
	unsigned short val = mkshort((COLOR_BLACK << 4) | COLOR_BLACK, 0);

	for(r = 0; r < scope; ++r) {
		for(c = 0; c < VGA_MAX_COL; ++c) {
			*buffer = val;
			++buffer;
		}
	}
	set_cursor(0, 0);
}

/*
*向上翻滚一行
*/
void scroll_screen(unsigned int scope)
{
	unsigned int r, c_len;

	if(!cursor_row) 
		return;

	c_len = multiply(sizeof(unsigned short), VGA_MAX_COL);
	for(r = 0; r < (scope -1 ); ++r) {
		memcpy(vga_buffer + multiply(r, VGA_MAX_COL), 
		vga_buffer + multiply(r+1, VGA_MAX_COL), c_len);
	}
	memset(vga_buffer + multiply(r, VGA_MAX_COL), 0, c_len);
	set_cursor(cursor_row - 1, cursor_rol);
}

/*
*向屏幕上输出一个字符
*/
void put_char_ex(unsigned int ch, unsigned int bg, unsigned int fg, unsigned int row, unsigned int col,
	unsigned int scope)
{
	unsigned short val = mkshort((bg<<4) |fg, ch);//????
	unsigned short *position;

	if(row>=scope)
		row=scope-1;//如果尝试向第scope行输入，将会强制切换到向第scope-1行输入
	if(col>=VGA_MAX_COL)
		col=VGA_MAX_COL -1;

	position = vga_buffer + multiply(row, VGA_MAX_COL) + col;
	*position = val;
}