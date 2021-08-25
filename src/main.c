/*-
 * Copyright (c) 2021 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/console.h>
#include <sys/systm.h>
#include <sys/thread.h>
#include <sys/spinlock.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/sem.h>
#include <sys/list.h>
#include <sys/smp.h>
#include <sys/cheri.h>
#include <sys/io.h>

#include <mips/beri/beri_epw.h>

#include <machine/cpuregs.h>
#include <machine/cpufunc.h>

#include <dev/virtio/virtio.h>
#include <dev/virtio/virtio-blk.h>
#include <dev/virtio/virtio-net.h>

#include <app/fpu_test/fpu_test.h>
#include <app/callout_test/callout_test.h>

#ifdef __CHERI_PURE_CAPABILITY__
#include <cheri_init_globals.h>
#endif

#include "board.h"
#include "device-model.h"
#include "main.h"

static struct epw_softc epw_sc;

extern void *pvAlmightyDataCap;

#ifdef __CHERI_PURE_CAPABILITY__
void
start_purecap(void)
{

	cheri_init_globals_3( __builtin_cheri_global_data_get(),
		__builtin_cheri_program_counter_get(),
		__builtin_cheri_global_data_get());
}
#endif

int
main(void)
{
	capability base, window;
	capability cap;

	printf("%s: starting on hart %d\n", __func__, PCPU_GET(cpuid));

	cap = pvAlmightyDataCap;
	base = mdx_setoffset(cap, 0x50000000);
	window = mdx_setoffset(cap, 0x60000000);

	epw_init(&epw_sc, base, window);

	/* Enable EPW */
	epw_control(&epw_sc, 1);

	dm_init(&epw_sc);
	dm_loop(&epw_sc);

	panic("dm_loop returned");

	uint8_t *addr;
	addr = (void *)0x50000000;

	while (1) {
		printf("hello world %x\n", *addr);
	}

	callout_test();

	/* NOT REACHED */

	panic("reached unreachable place");

	return (0);
}

/* Also provide a simple purecap strlen function */
__SIZE_TYPE__ strlen(const char* str)
{
	const char* p = str;

	while (*p != '\0')
		p++;

	return p - str;
}
