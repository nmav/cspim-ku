#include <syscalls.h>
#include "md5.h"

/* This program will calculate the md5sum
 * of the text received via syscall 1
 * and will copy it via syscall 2.
 */

int main(void)
{
unsigned char string[256];
struct md5_ctx ctx;
unsigned int size;
int ret;

	md5_init(&ctx);

	/* pass the string address above */
#ifdef TEST
#define STR "A long text to hash, and hash, and hash and hash..."
	size = sizeof(STR)-1;
	memcpy(string, STR, size);

	md5_update(&ctx, size, string);
	md5_digest(&ctx, 16, string);

	return 0;
#else
	ret = SYSCALL2(1, (unsigned int)string, sizeof(string));
	if (ret > 0) {
		size = ret;
		md5_update(&ctx, size, string);
		md5_digest(&ctx, 16, string);

		ret = SYSCALL2(2, (unsigned int)string, 16);
		if (ret == 0)
			return 0;
	}
#endif	
	return 1;
}
