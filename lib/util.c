/* 
 * File:    util.c
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
 * Common routines for run and torture.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "cspim.h"


static mips_uword cipher_peek(MIPS_CPU *pcpu, mips_uword addr);
static void cipher_poke(MIPS_CPU *pcpu, mips_uword addr, mips_uword w);

void read_elf(const char *fname, char **elf, unsigned *elfsz)
{
	FILE *f;
	long sz;
	
	if(!(f = fopen(fname, "r"))) {
		perror("fopen");
		exit(1);
	}
	if(fseek(f, 0, SEEK_END) < 0) {
		perror("fseek");
		exit(1);
	}
	if((sz = ftell(f)) < 0) {
		perror("ftell");
		exit(1);
	}
	if(fseek(f, 0, SEEK_SET) < 0) {
		perror("fseek");
		exit(1);
	}
	if(!(*elf = malloc(sz))) {
		perror("malloc");
		exit(1);
	}
	if(fread(*elf, 1, sz, f) != sz) {
		perror("fread");
		exit(1);
	}
	
	fclose(f);
	*elfsz = sz;
}

int cspim_hex_convert_key(struct cipher_key *pk, const char *hex)
{
	unsigned v;
	int i;
	
	if(strlen(hex) != 32)
		return 0;
	for(i = 0; i < 32; i += 2) {
		if(sscanf(hex+i, "%2x", &v) != 1)
			return 0;
		pk->key[i/2] = v;
	}
	return 1;
}

/* Returns 0 on success */
int cspim_cpu_prepare_file(cspim_cpu_t _pcpu, const char *exename, const char *asckey)
{
	char *elf;
	unsigned elfsz;
	
	read_elf(exename, &elf, &elfsz);
	
	return cspim_cpu_prepare_mem(_pcpu, elf, elfsz, asckey);
}

/* Returns 0 on success */
int cspim_cpu_prepare_mem(cspim_cpu_t _pcpu, void* elf, unsigned int elfsz, const char *asckey)
{
	struct cipher_key* Gkey;
	MIPS_CPU* pcpu = _pcpu;
	
	Gkey = malloc(sizeof(struct cipher_key));
	if (Gkey == NULL) {
		fprintf(stderr, "error allocating memory\n");
		return -1;
	}
	
	pcpu->priv = NULL;

	if(asckey) {
		if(!cspim_hex_convert_key(Gkey, asckey)) {
			fprintf(stderr, "can't convert key\n");
			return -1;
		}
		cipher_setup(Gkey);
		pcpu->priv = Gkey;
		pcpu->peek_uw = cipher_peek;
		pcpu->poke_uw = cipher_poke;
	}
	if(mips_elf_load(pcpu, elf, elfsz) < 0) {
		fprintf(stderr, "error preparing ELF for execution\n");
		return -1;
	}
	
	return 0;
}

void cspim_cpu_deinit(cspim_cpu_t _pcpu)
{
MIPS_CPU* pcpu = _pcpu;

	free(pcpu->priv);
	pcpu->priv = NULL;
	free(pcpu->base);
}

int cspim_cpu_init(cspim_cpu_t *pcpu, unsigned int memsz, unsigned int stacksz)
{
void * base;
MIPS_CPU* p;

	mips_init();
	
	base = malloc(memsz);
	if (base == NULL)
		return -1;

	*pcpu = mips_init_cpu(base, memsz, stacksz);

	if (*pcpu != NULL) {
		p = *pcpu;
		p->priv = NULL;
		return 0;
	} else
		return -1;

}

int cspim_execute_spim(cspim_cpu_t _pcpu)
{
	enum mips_exception err;
	Elf32_Sym *sym;
	const char *symname;
	int opcode, break_code;
	MIPS_CPU *pcpu = _pcpu;

execute:
	while((err = mips_execute(pcpu)) == MIPS_E_OK)
		;
	break_code = mips_break_code(pcpu, &opcode);
	switch(opcode) {
	case MIPS_I_BREAK:
		fprintf(stderr, "END: BREAK %d\n", break_code);
		return 0;
	case MIPS_I_SYSCALL:
		if(break_code != MIPS_SPIM_SYSCALL) {
			fprintf(stderr, "END: INVALID SYSCALL CODE %d\n", break_code);
			return -1;
		}
		if((err = mips_spim_syscall(pcpu)) != 0) {
			fprintf(stderr, "END: SPIM SERVICE %d FAULTED (%d)\n",
					pcpu->r.ur[2], err);
			return -1;
		}
		mips_resume(pcpu);
		goto execute;
	default:
		fprintf(stderr, "END: EXCEPTION %d AT PC=%08x", err, pcpu->pc);
		if((sym = mips_elf_find_address(pcpu, pcpu->pc)) &&
		   (symname = mips_elf_get_symname(pcpu, sym)))
			fprintf(stderr, " (near %s)", symname);
		fprintf(stderr, "\n");
		break;
	}
	return -1;
}

int cspim_execute(cspim_cpu_t _pcpu, int loops, cspim_syscall_fn fn)
{
	enum mips_exception err;
	Elf32_Sym *sym;
	const char *symname;
	int opcode, break_code;
	int ret;
	MIPS_CPU *pcpu = _pcpu;

execute:
	if (loops == -1) {
		while((err = mips_execute(pcpu)) == MIPS_E_OK)
			;
	} else {
		err = MIPS_E_OK;
		for (;loops >= 0;loops--) {
			err = mips_execute(pcpu);
			if (err != MIPS_E_OK) break;
		}
		if (err == MIPS_E_OK && loops == 0) return 0;
	}
	break_code = mips_break_code(pcpu, &opcode);
	switch(opcode) {
	case MIPS_I_BREAK:
		return 0;
	case MIPS_I_SYSCALL:
		if (fn) {
			ret = fn(pcpu->r.ur[2], pcpu->r.ur[4], 
				pcpu->r.ur[5], pcpu->r.ur[6], 
				pcpu->r.ur[7], pcpu->r.ur[8]);
			pcpu->r.sr[2] = ret;
		} else {
			pcpu->r.sr[2] = -1;
		}
		mips_resume(pcpu);
		goto execute;
	default:
		fprintf(stderr, "END: EXCEPTION %d AT PC=%08x", err, pcpu->pc);
		if((sym = mips_elf_find_address(pcpu, pcpu->pc)) &&
		   (symname = mips_elf_get_symname(pcpu, sym)))
			fprintf(stderr, " (near %s)", symname);
		fprintf(stderr, "\n");
		break;
	}
	return -1;
}

static mips_uword cipher_peek(MIPS_CPU *pcpu, mips_uword addr)
{
	mips_uword ret = mips_identity_peek_uw(pcpu, addr);
	unsigned char ctr[4];
	
	ctr[0] = addr;
	ctr[1] = (addr) >> 8;
	ctr[2] = (addr) >> 16;
	ctr[3] = (addr) >> 24;

	cipher_ctr_decrypt(pcpu->priv, &ctr, &ret, &ret);
	return ret;
}

static void cipher_poke(MIPS_CPU *pcpu, mips_uword addr, mips_uword w)
{
	unsigned char ctr[4];
	
	ctr[0] = addr;
	ctr[1] = (addr) >> 8;
	ctr[2] = (addr) >> 16;
	ctr[3] = (addr) >> 24;

	cipher_ctr_encrypt(pcpu->priv, &ctr, &w, &w);
	mips_identity_poke_uw(pcpu, addr, w);
}

void cspim_mips_dump_cpu(cspim_cpu_t pcpu)
{
	mips_dump_cpu(pcpu);
}

