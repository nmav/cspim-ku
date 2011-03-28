/* 
 * File:    syscalls.c
 * Author:  zvrba
 * Created: 2008-03-20
 *
 * ===========================================================================
 * COPYRIGHT (c) 2008 Zeljko Vrba <zvrba.external@zvrba.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * ===========================================================================
 */
/**
 * @file
 *
 * MIPS I system call interface; hosted implementation.  All SPIM system calls
 * are implemented, except:
 * - floating-point routines (not implemented in the simulator)
 * - exit, exit2 (the support library uses BREAK to exit the program)
 *
 * Entry: service: $v0 ($2), args: $a0-$a3 ($4-$7), ret: $v0 ($2)
 * Services:
 * - 01: void print_int(int n);
 * - 04: void print_stirng(const char *str);
 * - 05: int read_int(void);
 * - 08: void read_string(char *buf, unsigned len);
 * - 09: void *sbrk(unsigned amount);
 * - 11: void print_char(char ch);
 * - 12: int read_char(void);
 * - 13: int open(const char *fname, int flags, int mode);
 * - 14: int read(int fd, void *buf, unsigned len);
 * - 15: int write(int fd, void *buf, unsigned len);
 * - 16: int close(int fd);
 *
 * Some additional system services, not found in SPIM, are implemented:
 * - 17: unsigned lseek(int fd, unsigned offset, int whence);
 *
 * Note: the SPIM close() variant returns void.  We return the value returned
 * by the underlying close system call.
 *
 * A separate file descriptor table is maintained for the simulated CPU.
 * Direct access to host's file descriptors is allowed only for stdin, stdout
 * and stderr.  Otherwise, a private mapping from the simulated to the host's
 * file descriptors is maintained.  The simulated program may access up to
 * MIPS_MAXFDS file descriptors (compile-time constant); this limit includes
 * also fds 0, 1 and 2.  FDs 0,1,2 cannot be closed, thus not reused either.
 */
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "../cpu.h"

#define SYSCALL_MAX 17

static int do_print_int(MIPS_CPU*); /* 1 */
static int do_print_string(MIPS_CPU*); /* 4 */
static int do_read_int(MIPS_CPU*); /* 5 */
static int do_read_string(MIPS_CPU*); /* 8 */
static int do_sbrk(MIPS_CPU*); /* 9 */
static int do_print_char(MIPS_CPU*); /* 11 */
static int do_read_char(MIPS_CPU*); /* 12 */
static int do_open(MIPS_CPU*); /* 13 */
static int do_read(MIPS_CPU*); /* 14 */
static int do_write(MIPS_CPU*); /* 15 */
static int do_close(MIPS_CPU*); /* 16 */
static int do_lseek(MIPS_CPU*); /* 17 */
static int find_fd_slot(MIPS_CPU*);

typedef int (*syscall_handler)(MIPS_CPU*);

static syscall_handler sys_dispatch[SYSCALL_MAX+1] = {
	0, do_print_int, 0, 0, do_print_string, do_read_int, 0, 0,
	do_read_string, do_sbrk, 0, do_print_char, do_read_char,
	do_open, do_read, do_write, do_close, do_lseek
};

void mips_dump_cpu(MIPS_CPU *pcpu)
{
	int i;
	
	printf("***MIPS@%p***\n", pcpu);
	printf("BASE=%lx@%p BRK=%08x STKSZ=%08x\n",
		   (unsigned long)pcpu->memsz, pcpu->base, pcpu->brk,
		   (unsigned int)pcpu->stksz);
	for(i = 0; i < 8; i++) {
		int j = 4*i;
		
		printf("R%02d=%08x R%02d=%08x R%02d=%08x R%02d=%08x\n",
				j,   pcpu->r.ur[j],   j+1, pcpu->r.ur[j+1],
				j+2, pcpu->r.ur[j+2], j+3, pcpu->r.ur[j+3]);
	}
	printf("HI =%08x LO =%08x\n", pcpu->hi, pcpu->lo);
	printf("PC =%08x DS =%08x\n", pcpu->pc, pcpu->delay_slot);
}

int mips_spim_syscall(MIPS_CPU *pcpu)
{
	int  opcode;
	mips_uword service;
	
	if((mips_break_code(pcpu, &opcode) < 0) || (opcode != MIPS_I_SYSCALL))
		return -1;
	service = pcpu->r.ur[2];
	if((service > SYSCALL_MAX) || !sys_dispatch[service])
		return -1;
	sys_dispatch[service](pcpu);
	return 0;
}

static int do_print_int(MIPS_CPU *pcpu)
{
	assert(pcpu->r.ur[2] == 1);
	printf("%d", pcpu->r.sr[4]);
	fflush(stdout);
	return 0;
}

static int do_print_string(MIPS_CPU *pcpu)
{
	mips_uword ptr = pcpu->r.ur[4];
	mips_ubyte ch;
	int err;
	
	assert(pcpu->r.ur[2] == 4);

	if((err = setjmp(pcpu->exn)) == 0) {
		while(1) {
			ch = mips_peek_ub(pcpu, ptr++);
			if(!ch)
				break;
			putchar(ch);
		}
		fflush(stdout);
	}
	return err;
}

static int do_read_int(MIPS_CPU *pcpu)
{
	char buf[18];
	int x;
	
	assert(pcpu->r.ur[2] == 5);
	fgets(buf, sizeof(buf), stdin);
	if(sscanf(buf, "%d", &x) == 1)
		pcpu->r.sr[2] = x;
	return 0;
}

static int do_read_string(MIPS_CPU *pcpu)
{
	mips_uword buf = pcpu->r.ur[4];
	mips_uword len = pcpu->r.ur[5];
	unsigned i = 0;
	int ch, err;
	
	assert(pcpu->r.ur[2] == 8);
	if((err = setjmp(pcpu->exn)) == 0) {
		while(len-- > 1) {
			ch = getchar();
			if(ch != EOF)
				mips_poke_ub(pcpu, buf+i, ch);
			++i;
			if((ch == '\n') || (ch == EOF))
				break;
		}
		mips_poke_ub(pcpu, buf+i, 0);
	}
	return err;
}

static int do_sbrk(MIPS_CPU *pcpu)
{
	mips_uword amount = pcpu->r.ur[2];
	mips_uword oldbrk = pcpu->brk;
	mips_uword newbrk;
	
	assert((pcpu->r.ur[2] == 9) && (oldbrk % 4 == 0));
	amount = pcpu->r.ur[2];
	
	if(amount & 3)
		amount += 4 - (amount & 3);
	newbrk = oldbrk + amount;

	/* Do not allow break to grow into the stack. */
	
	if(newbrk >= pcpu->memsz - pcpu->stksz) {
		pcpu->r.sr[2] = -1;
	} else {
		pcpu->brk = newbrk;
		pcpu->r.sr[2] = oldbrk;
	}
	return 0;
}

static int do_print_char(MIPS_CPU *pcpu)
{
	mips_uword ch = pcpu->r.ur[4];
	
	assert(pcpu->r.ur[2] == 11);
	putchar(ch);
	fflush(stdout);
	return 0;
}

static int do_read_char(MIPS_CPU *pcpu)
{
	assert(pcpu->r.ur[2] == 12);
	pcpu->r.sr[2] = getchar();
	return 0;
}

static int do_open(MIPS_CPU *pcpu)
{
	mips_uword name  = pcpu->r.ur[4];
	mips_uword flags = pcpu->r.ur[5];
	mips_uword mode  = pcpu->r.ur[6];
	char fname[256];
	unsigned i = 0;
	int fd, err;
	
	assert(pcpu->r.ur[2] == 13);

	switch(flags) {
	case O_RDONLY:
		break;
	case O_WRONLY:
	case O_RDWR:
		flags |= O_CREAT;
		break;
	default:
		pcpu->r.sr[2] = -1;
		return 0;
	};

	if((err = setjmp(pcpu->exn)) == 0) {
		while(1) {
			if(i >= sizeof(fname))
				return MIPS_E_ADDRESS;
			fname[i] = mips_peek_ub(pcpu, name+i);
			if(!fname[i])
				break;
			++i;
		}
	}
	if(err)
		return err;
	
	if((fd = find_fd_slot(pcpu)) >= 0) {
		int osfd = open(fname, flags, mode);
		if(osfd >= 0)
			pcpu->fds[fd] = osfd;
	}
	pcpu->r.sr[2] = fd;	
	return 0;
}

static int do_read(MIPS_CPU *pcpu)
{
	int fd = pcpu->r.sr[4];
	mips_uword buf = pcpu->r.ur[5];
	mips_uword len = pcpu->r.ur[6];
	char *p;
	int err;
	
	assert(pcpu->r.ur[2] == 14);
	if(buf + len >= pcpu->memsz)
		return MIPS_E_ADDRESS;
	
	if((fd < 0) || (fd >= MIPS_MAXFDS) || (pcpu->fds[fd] < 0)) {
		pcpu->r.sr[2] = -1;
		return 0;
	}

	if(!(p = malloc(len)))
		return MIPS_E_ABORT;
	
	/* Check for the "impossible": successful read returning more than
	 * requested.  If everyhting is OK, copyout shall not throw due to
	 * prior check for buffer size. */
	
	err = 0;
	if((pcpu->r.sr[2] = read(pcpu->fds[fd], p, len)) > 0) {
		if(pcpu->r.sr[2] > len) {
			err = MIPS_E_ABORT;
		} else {
			mips_copyout(pcpu, buf, p, pcpu->r.sr[2]);
		}
	}
	
	free(p);
	return err;
}

static int do_write(MIPS_CPU *pcpu)
{
	int fd = pcpu->r.sr[4];
	mips_uword buf = pcpu->r.ur[5];
	mips_uword len = pcpu->r.ur[6];
	char *p;
	int err;
	
	assert(pcpu->r.ur[2] == 15);
	if(buf + len >= pcpu->memsz)
		return MIPS_E_ADDRESS;
	
	if((fd < 0) || (fd >= MIPS_MAXFDS) || (pcpu->fds[fd] < 0)) {
		pcpu->r.sr[2] = -1;
		return 0;
	}

	if(!(p = malloc(len)))
		return MIPS_E_ABORT;
	mips_copyin(pcpu, p, buf, len);
	
	/* Again, check for the "impossible". */

	err = 0;
	if((pcpu->r.sr[2] = write(pcpu->fds[fd], p, len)) > 0) {
		if(pcpu->r.sr[2] > len) 
			err = MIPS_E_ABORT;
	}

	free(p);
	return err;
}

static int do_close(MIPS_CPU *pcpu)
{
	int fd = pcpu->r.sr[4];

	// Do not allow closing of fds 0,1,2.
	
	assert(pcpu->r.ur[2] == 16);
	if((fd <= 2) || (fd >= MIPS_MAXFDS) || (pcpu->fds[fd] < 0)) {
		pcpu->r.sr[2] = -1;
	} else {
		pcpu->r.sr[2] = close(pcpu->fds[fd]);
		pcpu->fds[fd] = -1;
	}
	return 0;
}

static int do_lseek(MIPS_CPU *pcpu)
{
	int fd = pcpu->r.sr[4];
	mips_sword off = pcpu->r.sr[5];
	int whence = pcpu->r.sr[6];
	
	/* Note: fds 0-2 are not seekable! */
	
	assert(pcpu->r.ur[2] == 17);
	if((fd <= 2) || (fd >= MIPS_MAXFDS) || (pcpu->fds[fd] < 0)) {
		pcpu->r.sr[2] = -1;
	} else {
		pcpu->r.sr[2] = lseek(pcpu->fds[fd], off, whence);
	}
	return 0;
}

static int find_fd_slot(MIPS_CPU *pcpu)
{
	int i;
	
	for(i = 0; i < MIPS_MAXFDS; i++)
		if(pcpu->fds[i] == -1)
			return i;
	return -1;
}
