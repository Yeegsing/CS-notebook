#
#except.s
#

.global _except_handler
.global _end_ex

.set noreorder
.set noat
.align 2 #即2^2=4字节对齐

_except_handler:
	#保存上下文

	#
	#处理过程
	#


	#开启中断

	#恢复上下文
	eret
_end_ex: