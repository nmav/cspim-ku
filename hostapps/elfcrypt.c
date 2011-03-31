/* 
 * File:    elfcrypt.c
 * Author:  zvrba
 * Created: 2008-04-09
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
 * Encrypt all loadable segments in an ELF file.  The key is given on the
 * command-line as a 32-digit hex string (total 128 bits).  The ELF file is
 * encrypted with simple ECB scheme just as proof-of-concept.  Other modes,
 * such as CTR, are also possible.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "types.h"
#include "elf.h"
#include "rc5-16.h"
#include "util.h"

static void check_eh_limits(const Elf32_Ehdr*, size_t);
static void crypt_segment(const struct rc5_key *pk, char *elf,
	mips_uword base, unsigned offset, unsigned size);

static void randomize_key(struct rc5_key *key)
{
	FILE* fd;
	int i;
	
	fd = fopen("/dev/urandom", "r");
	if (fd == NULL) {
		fprintf(stderr, "Cannot open /dev/urandom");
		exit(1);
	}
	
	fread(key->key, 1, sizeof(key->key), fd);
	
	fclose(fd);

	printf("Generated key: ");
	for (i=0;i<sizeof(key->key);i++)
		printf("%.2x", key->key[i]);
	printf("\n");
}

int main(int argc, char **argv)
{
	struct rc5_key key;
	char *elf;
	size_t elfsz;
	Elf32_Ehdr *eh;
	Elf32_Phdr *ph;
	FILE *out;
	unsigned i, cnt, head_size;
	mips_uword vaddr;
	unsigned size, offset;
	
	if(argc < 3) {
		fprintf(stderr, "USAGE: %s ELF-IN ELF-OUT HEXKEY\n", argv[0]);
		return 1;
	}
	
	if (argc == 3) {
		randomize_key(&key);
	} else {
		if(!rc5_convert_key(&key, argv[3])) {
			fprintf(stderr, "can't convert key to internal form\n");
			return 1;
		}
	}
	rc5_setup(&key);

	read_elf(argv[1], &elf, &elfsz);
	eh = (Elf32_Ehdr*)elf;
	check_eh_limits(eh, elfsz);

	head_size = eh->e_ehsize + eh->e_phnum*eh->e_phentsize;

	/* Iterate segments, check bounds and encrypt PT_LOAD. */
	for(i = 0, cnt = 0; i < eh->e_phnum; i++) {
		ph = (Elf32_Phdr*)(elf + eh->e_phoff + i*sizeof(Elf32_Phdr));

		if(ph->p_type != PT_LOAD)
			continue;

		++cnt;
		if(ph->p_offset + ph->p_filesz >= elfsz) {
			fprintf(stderr, "input segment %u is out of ELF bounds\n", i);
			exit(1);
		}

		if((ph->p_filesz % RC5_BLOCKSZ) || (ph->p_memsz % RC5_BLOCKSZ)) {
			/* 4 bytes is the cipher block size */
			fprintf(stderr, "input segment file/memory size is not a multiple of 4\n");
			exit(1);
		}
		if (ph->p_offset <= head_size) { /* Avoid encrypting the ELF headers */
			int head_end = head_size - ph->p_offset;
			
			vaddr = ph->p_vaddr+head_end;
			size = ph->p_filesz-head_end;
			offset = ph->p_offset+head_end;
		} else {
			vaddr = ph->p_vaddr;
			size = ph->p_filesz;
			offset = ph->p_offset;
		}
		printf("Encrypting %u bytes at vaddr %.8x (offset: %u)\n", size, vaddr, offset);
		crypt_segment(&key, elf, vaddr, offset, size);
	}

	if(!cnt) {
		fprintf(stderr, "no PT_LOAD segments found\n");
		exit(1);
	}

	if(!(out = fopen(argv[2], "w"))) {
		fprintf(stderr, "can't write output\n");
		exit(1);
	}
	if((fwrite(elf, 1, elfsz, out) != elfsz) || (fclose(out) == EOF)) {
		fprintf(stderr, "output incomplete\n");
		exit(1);
	}
	return 0;
}

static void crypt_segment(const struct rc5_key *pk, char *elf,
	mips_uword base, unsigned offset, unsigned size)
{
	char *p = elf + offset;
	unsigned i;

	for(i = 0; i < size; i += RC5_BLOCKSZ)
		rc5_ecb_encrypt(pk, p+i, p+i);
}

static void check_eh_limits(const Elf32_Ehdr *eh, size_t elfsz)
{
	if(memcmp(eh->e_ident, "\177ELF", 4) != 0) {
		fprintf(stderr, "input doesn't have the ELF magic\n");
		exit(1);
	}
	if((eh->e_ident[EI_CLASS] != ELFCLASS32) ||
	   (eh->e_ident[EI_DATA] != ELFDATA2LSB)) {
		fprintf(stderr, "input isn't LSB/32\n");
		exit(1);
	}
	if((eh->e_ehsize != sizeof(*eh)) ||
	   (eh->e_phentsize != sizeof(Elf32_Phdr))) {
		fprintf(stderr, "mismatch between file and program structures\n");
		exit(1);
	}
	if(eh->e_type != ET_EXEC) {
		fprintf(stderr, "input isn't ET_EXEC\n");
		exit(1);
	}
	if(!eh->e_phnum) {
		fprintf(stderr, "input has no program headers\n");
		exit(1);
	}
	if(eh->e_phoff + eh->e_phnum*eh->e_phentsize > elfsz) {
		fprintf(stderr, "invalid bounds of program header table\n");
		exit(1);
	}
}
