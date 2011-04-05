/* 
 * File:    run.c
 * Author:  nmav
 * Created: 2011-04-04
 *
 * ===========================================================================
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cspim.h"

#define MEMSZ (16*1024*1024)
#define STKSZ (64*1024)

typedef struct {
	cspim_cpu_t pcpu;
	char* text;
	unsigned char md5[16];
	int ok;
} priv_st;

static int syscall_fn(void* _priv, unsigned int no, uint32_t arg1, uint32_t arg2, uint32_t arg3,
  uint32_t arg4, uint32_t arg5)
{
	priv_st* priv = _priv;
	int len;
	
	switch(no) {
		case 1: /* write the text to be hashed */
			len = strlen(priv->text);
			cspim_mips_write(priv->pcpu, arg1, priv->text, len);
			return len;
		case 2: /* read the hashed text */
			if (arg2 != 16) {
				fprintf(stderr, "illegal size of %u\n", arg2);
				exit(1);
			}
			cspim_mips_read(priv->pcpu, priv->md5, arg1, arg2);
			priv->ok = 1;
			return 0;
		default:
			fprintf(stderr, "syscall %u with arg1 %.8x and arg2 %.8x\n", no,
				(unsigned int)arg1, (unsigned int)arg2);
			return -1;
	}
}

int main(int argc, char **argv)
{
	char *base;
	cspim_cpu_t pcpu;
	priv_st priv;
	int i;
	unsigned char res[] = "\x84\xd2\xac\xe4\x96\x06\xee\x28\x92\x4b\x24\xcf\xc9\x5b\x9d\x3c";

	memset(&priv, 0, sizeof(priv));
	
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
	
	priv.pcpu = pcpu;
	priv.text = "A long text to hash, and hash, and hash and hash...";

	if (cspim_execute(pcpu, -1, &priv, syscall_fn) < 0) {
  	    cspim_mips_dump_cpu(pcpu);
        }

	cspim_cpu_deinit(pcpu);
	
	if (priv.ok) {
		printf("Hashed: '%s'\nResult: ", priv.text);
		for (i=0;i<sizeof(priv.md5);i++)
			printf("%.2x:", priv.md5[i]);
		printf("\n");
		if (memcmp(priv.md5, res, 16) != 0) {
			fprintf(stderr, "MD5 result does not match the expected\n");
		}
	} else {
		fprintf(stderr, "Could not hash\n");
		exit(1);
	}

	return 0;
}

