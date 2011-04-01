#include <syscalls.h>

int main(void)
{
//	print_string("Hello world!.. we should exit with invalid SYSCALL, code=17\n");
	//to test traps on invalid memory access
	//asm volatile("xor $1, $1, $1 ; jr $1");
	
	SYSCALL1(15, 20);
	SYSCALL2(10, 20, 30);
	return 1;
}
