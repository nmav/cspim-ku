#include <syscalls.h>

int main(void)
{
void * ptr, *ptr2;
//	print_string("Hello world!.. we should exit with invalid SYSCALL, code=17\n");
	//to test traps on invalid memory access
	//asm volatile("xor $1, $1, $1 ; jr $1");
	
	SYSCALL1(USER_SYSCALL(15), 20);
	SYSCALL2(USER_SYSCALL(10), 20, 30);

	ptr = malloc(1024);
	print_string("allocated 1024: ");
	print_int((int)ptr);
	print_string("\n");

	ptr2 = malloc(4096);
	print_string("allocated 4096: ");
	print_int((int)ptr2);
	print_string("\n");
	
	free(ptr);
	free(ptr2);

	ptr = malloc(1024);
	print_string("allocated 1024: ");
	print_int((int)ptr);
	print_string("\n");

	ptr2 = malloc(2048);
	print_string("allocated 2048: ");
	print_int((int)ptr2);
	print_string("\n");

	return 0;
}
