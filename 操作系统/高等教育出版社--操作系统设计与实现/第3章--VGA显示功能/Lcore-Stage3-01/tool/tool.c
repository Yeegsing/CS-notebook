//memset的作用是将以dest为起始地址的内存区域的前n个字节设置为字符ch
void *memset(void *dest, int ch, int n)
{
	char *deststr = dest;

	while(n--){
		*deststr = ch;
		++deststr;
	}
	return dest;
}

//乘除模拟函数
int multiply(int a, int b)
{
	int res = 0;

	while(a != 0) {
		res += b;
		--a;
	}
	return res;
}

int division(int n, int div)
{
	int res = 0;

	while (n > div) {
		n -= div;
		++res;
	}
	return res;
}