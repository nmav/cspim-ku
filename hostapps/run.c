/* 
 * File:    run.c
 * Author:  zvrba
 * Created: 2008-02-20
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
 * Load and execute a little-endian MIPS file that exercies SPIM stuff.  The
 * execution stops at first exception which is printed to stderr.  If the
 * exception is caused by BREAK or unsupported SYSCALL instruction, the code
 * is in addition printed to stdout.  Also supports execution of executables
 * encrypted with elfcrypt.
 */

#include <stdio.h>
#include <stdlib.h>
#include "cspim.h"

#define MEMSZ (2U << 24)
#define STKSZ (16U << 10)

static int syscall_fn(unsigned int no, uint32_t arg1, uint32_t arg2, uint32_t arg3,
  uint32_t arg4, uint32_t arg5)
{
	fprintf(stderr, "executed syscall %u with arg1 %.8x and arg2 %.8x\n", no,
		(unsigned int)arg1, (unsigned int)arg2);

	return 0;
}

int main(int argc, char **argv)
{
	char *base;
	cspim_cpu_t pcpu;
	
	if((argc != 2) && (argc != 3)) {
		fprintf(stderr, "USAGE: %s ELF [KEY]\n", argv[0]);
		exit(1);
	}
	if(!(base = malloc(MEMSZ))) {
		perror("malloc");
		exit(1);
	}

	cspim_cpu_init(&pcpu, MEMSZ, STKSZ);
	cspim_cpu_prepare_file(pcpu, argv[1], (argc == 3) ? argv[2] : NULL);
	if (cspim_execute(pcpu, -1, syscall_fn) < 0) {
  	    cspim_mips_dump_cpu(pcpu);
        }

	cspim_cpu_deinit(pcpu);

    return 0;
}

