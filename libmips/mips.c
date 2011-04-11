#include "spim.h"

#define SYSCALL(n)								\
	({											\
		register void *rv asm("$2");			\
		asm volatile("li $2, %1;"				\
					 "SYSCALL 0x9107C"			\
					 : "=r" (rv)				\
					 : "K" (n));				\
		rv;										\
	})

void print_int(int n)
{
	SYSCALL(1);
}

void print_string(const char *str)
{
	SYSCALL(4);
}

int read_int(void)
{
	return (int)SYSCALL(5);
}

void read_string(char *buf, unsigned len)
{
	SYSCALL(8);
}

void *sbrk(unsigned amount)
{
	return SYSCALL(9);
}

void print_char(char ch)
{
	SYSCALL(11);
}

int read_char(void)
{
	return (int)SYSCALL(12);
}

int open(const char *buf, int mode, int flags)
{
	return (int)SYSCALL(13);
}

int read(int fd, void *buf, unsigned len)
{
	return (int)SYSCALL(14);
}

int write(int fd, void *buf, unsigned len)
{
	return (int)SYSCALL(15);
}

int close(int fd)
{
	return (int)SYSCALL(16);
}

int lseek(int fd, unsigned off, int whence)
{
	return (int)SYSCALL(17);
}

struct mem {
  struct mem* next;
  void* start;
  unsigned int size;
  int in_use;
};

struct mem *head = 0;
struct mem *tail = 0;

void * _mips_malloc(unsigned int size)
{
struct mem* ptr;

	/* recycle if possible */
	for (ptr = head;ptr != 0; ptr = ptr->next) {
		if (ptr->in_use == 0 && ptr->size >= size) {
			ptr->in_use = 1;
			return ptr->start;
		}
	}

	ptr = sbrk(size+ sizeof(struct mem));
	if (ptr == 0)
		return 0;

	if (head == 0) {
		head = ptr;
		head->next = 0;
		head->start = head + sizeof(struct mem);
		head->size = size;
		head->in_use = 1;
		tail = head;
	} else {
		tail->next = ptr;
		tail->next->next = 0;
		tail->next->start = ptr + sizeof(struct mem);
		tail->next->size = size;
		tail->next->in_use = 1;

		tail = tail->next;
	}
	
	return tail->start;
}

void _mips_free(void* mem)
{
struct mem *ptr;

	if (head == 0) return;
	
	for (ptr = head;ptr != 0; ptr = ptr->next) {
		if (ptr->start == mem) {
			ptr->in_use = 0;
			return;
		}
	}
}

#if 0
void __start(void)
{
	asm volatile("jal main ; break 0");
}
#endif
