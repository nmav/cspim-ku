/* 
 * File:    syscalls.h
 * Author:  nmav
 * Created: 2011
 *
 * Interface to system calls.
 */

#ifndef CSPIM_MIPS_H
#define CSPIM_MIPS_H

#define USER_SYSCALL(x) (x+0x1f)

#define SYSCALL1( n, arg1) \
({ \
register int rv; \
register int i1 asm("$4") = arg1; \
  asm volatile("li $2, %1;" \
	 "SYSCALL;" \
	 : "=r" (rv) \
	 : "K" (n),"r"(i1));\
  rv; \
})

#define SYSCALL2( n,  arg1,  arg2) \
({ \
register int rv; \
register int i1 asm("$4") = arg1; \
register int i2 asm("$5") = arg2; \
  asm volatile("li $2, %1;" \
	 "SYSCALL;" \
	 : "=r" (rv) \
	 : "K" (n),"r"(i1),"r"(i2)); \
  rv; \
})

#define SYSCALL3( n,  arg1,  arg2,  arg3) \
({ \
register int rv; \
register int i1 asm("$4") = arg1; \
register int i2 asm("$5") = arg2; \
register int i3 asm("$6") = arg3; \
  asm volatile("li $2, %1;" \
	 "SYSCALL;" \
	 : "=r" (rv) \
	 : "K" (n),"r"(i1),"r"(i2),"r"(i3)); \
  rv; \
})

#define SYSCALL4( n,  arg1,  arg2,  arg3,  arg4) \
({ \
register int rv; \
register int i1 asm("$4") = arg1; \
register int i2 asm("$5") = arg2; \
register int i3 asm("$6") = arg3; \
register int i4 asm("$7") = arg4; \
  asm volatile("li $2, %1;" \
	 "SYSCALL;" \
	 : "=r" (rv) \
	 : "K" (n),"r"(i1),"r"(i2),"r"(i3),"r"(i4)); \
  rv; \
})

#define SYSCALL5( n,  arg1,  arg2,  arg3,  arg4,  arg5) \
({ \
register int rv; \
register int i1 asm("$4") = arg1; \
register int i2 asm("$5") = arg2; \
register int i3 asm("$6") = arg3; \
register int i4 asm("$7") = arg4; \
register int i5 asm("$8") = arg5; \
  asm volatile("li $2, %1;" \
	 "SYSCALL;" \
	 : "=r" (rv) \
	 : "K" (n),"r"(i1),"r"(i2),"r"(i3),"r"(i4),"r"(i5)); \
  rv; \
})

/* Definitions of common functions */
#define memset _mips_memset
#define memcpy _mips_memcpy

static inline void *_mips_memcpy(void *dest, const void *src, unsigned int n)
{
  int i;
  unsigned char *_dst = dest;
  const unsigned char *_src = src;
  for (i=0;i<n;i++)
    _dst[i] = _src[i];
  
  return dest;
}

static inline void *_mips_memset(void *s, int c, unsigned int n)
{
  int i;
  unsigned char *dst = s;
  for (i=0;i<n;i++)
    dst[i] = c;
  
  return s;
}

/* Definitions for SPSIM syscalls */

/* These constants must match the native system's constants!  And these modes
 * seem to be the only ones that are equal among unices (at least Solaris and
 * linux).  Therefore, open also adds O_CREAT if it detects writing mode. */

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR   2

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

void print_int(int);
void print_string(const char*);
int read_int(void);
void read_string(char*, unsigned);
void *sbrk(unsigned);
void print_char(char);
int read_char(void);
int open(const char*, int, int);
int read(int, void*, unsigned);
int write(int, void*, unsigned);
int close(int);
int lseek(int, unsigned, int);

#define malloc(x) _mips_malloc(x)
#define free(x) _mips_free(x)

void * _mips_malloc(unsigned int);
void _mips_free(void*);

inline static void* calloc(unsigned int x, unsigned int s)
{
  unsigned int size = s*x;
  void * ptr = malloc(size);
  if (ptr == 0)
    memset(ptr, 0, size);
  
  return ptr;
}

#endif	/* SPIM_MIPS_H */
