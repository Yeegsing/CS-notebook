#include "tool.h"

void *memcpy(void *dest, void *src, int len)
{
	char *deststr = dest;
	char *srcstr = src;
	while(len--) {
		*deststr = *srcstr;
		++deststr;
		++srcstr;
	}
	return dest;
}