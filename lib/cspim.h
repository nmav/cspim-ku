/* 
 * File:    util.h
 * Author:  zvrba
 * Created: 2008-04-09
 * Modified: 2011-04-01 by Nikos Mavrogiannopoulos
 *
 * ===========================================================================
 * COPYRIGHT (c) 2008 Zeljko Vrba <zvrba.external@zvrba.net>
 * COPYRIGHT (c) 2011 Katholieke Universiteit Leuven
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
 * Common utility functions used by apps.
 */

#ifndef UTIL_H__
#define UTIL_H__

#include <stdint.h>

#define USER_SYSCALL(x) (x+0x1f)

typedef unsigned short RC5_WORD;
#define CIPHER_BLOCKSZ	4			/* (bytes) */
#define RC5_KEYLEN	16			/* (bytes) */
#define RC5_ROUNDS	12			/* # of rounds */

struct cipher_key {
	unsigned char key[RC5_KEYLEN];
	RC5_WORD S[2*(RC5_ROUNDS+1)];
};

/** Allocate space and read the complete ELF into memory. */
void read_elf(const char *fname, char **elf, unsigned *elfsz);

/** Convert key from a string of 32 hex digits. */
int cspim_hex_convert_key(struct cipher_key *pk, const char *hex);

typedef void* cspim_cpu_t;

int cspim_cpu_init(cspim_cpu_t *pcpu, unsigned int memsz, unsigned int stacksz);

/** Prepare CPU for execution with optional encryption key. 
 * The memory address provided must be valid until the deinitialization.
 */
int cspim_cpu_prepare_mem(cspim_cpu_t _pcpu, void* elf, unsigned int elfsz, const char *asckey);

int cspim_cpu_prepare_file(cspim_cpu_t _pcpu, const char *exename, const char *asckey);

/** Deinitializes CPU. */
void cspim_cpu_deinit(cspim_cpu_t pcpu);

/**
 * Execute until exception and report status to stdout.  Handles SPIM
 * syscalls. Returns 0 on successful termination.
 */
int cspim_execute_spim(cspim_cpu_t _pcpu);

typedef int (*cspim_syscall_fn)(void* priv, unsigned int no, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5);
int cspim_execute(cspim_cpu_t _pcpu, int loops, void* priv, cspim_syscall_fn fn);

void cspim_mips_dump_cpu(cspim_cpu_t pcpu);

/*@{*/
/**
 * Copy (potentially unaligned) data from host to the simulator (out), or from
 * simulator to the host (in).  In the case of failure, the state of the MIPS
 * simulator has not been altered.
 *
 * @param pcpu CPU state.
 * @param dst  Destination address in MIPS (out) or host (in).
 * @param src  Source address in host (out) or MIPS (in).
 * @param n    Number of bytes to copy.
 * @return 0 on success, -1 on failure (i.e. the transfer would access memory
 * outside of MIPS segment).
 */
int cspim_mips_write(cspim_cpu_t pcpu, uint32_t dst, void *src, unsigned int n);
int cspim_mips_read(cspim_cpu_t pcpu, void *dst, uint32_t src, unsigned int n);
/*@}*/

/* CIPHER operations 
 */

/**
 * Expand the key.  On entry, the the ks->key field must be initialized with
 * the desired key.
 */
void cipher_setup(struct cipher_key *ks);

/* Counter mode encryption 
 */
void cipher_rctr_encrypt(const struct cipher_key *ks, void* counter, void *src, void *dst);
void cipher_rctr_decrypt(const struct cipher_key *ks, void* counter, void *src, void *dst);


#endif	/* UTIL_H__ */
