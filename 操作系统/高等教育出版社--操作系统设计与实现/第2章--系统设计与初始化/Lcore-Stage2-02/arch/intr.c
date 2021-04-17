#include "arch.h"
#include "intr.h"

void init_exint()
{
	int entry = _INTR_ENTRY;

	memcpy(EXCEPT_ENTRY, &_exint_handler, ((char *)&_end_ex -(char *)&_exint_handler));

	asm volatile(
		"mtc0 %0, $ 3"
		:
		:"r"(entry)
		)
}