/*
*可变参数相关
*/
typedef char * va_list;
#define _INTSIZEOF(n) ((sizeof(n)+sizeof(int)-1)&~(sizeof(int)-1))
//这个宏是用来计算n类型的大小，并采用int的大小对齐
#define va_start(ap, v) (ap = (va_list)&v + _INTSIZEOF(v))
//第一个可选参数地址
#define va_arg(apt, t) ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)))
//下一个可选参数地址
#define va_end(ap) ( ap = (va_list)0)
//将指针置为无效