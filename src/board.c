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
#include <sys/of.h>

#include <machine/vmparam.h>

#include <riscv/include/plic.h>
#include <dev/uart/uart_16550.h>
#include <libfdt/libfdt.h>

#include "board.h"

#define	CLINT_BASE		0x02000000
#define	PLIC_BASE		0x0c000000
#define	UART_BASE		0x10000100
#define	UART_CLOCK_RATE		3686400
#define	DEFAULT_BAUDRATE	115200

extern uint8_t __riscv_boot_ap[MDX_CPU_MAX];

static mdx_device_t dev_uart;

void *pvAlmightyDataCap;
void *pvAlmightyCodeCap;

char
uart_getchar(void)
{
	char a;

	a = mdx_uart_getc(dev_uart);

	return (a);
}

static void
fdt_relocate(void)
{
	void *new_dtbp;
	void *dtbp;
	int error;
	int sz;

	dtbp = mdx_of_get_dtbp();

	sz = fdt_totalsize(dtbp) * 2; /* Reserve space for modifications. */

	new_dtbp = malloc(sz);

	error = fdt_move(dtbp, new_dtbp, sz);
	if (error != 0)
		panic("could not move dtbp");

	mdx_of_install_dtbp(new_dtbp);
}

static void
fdt_patch(void)
{
	int offset;
	void *dtbp;

	offset = mdx_of_chosen_path_offset();
	dtbp = mdx_of_get_dtbp();

	fdt_setprop(dtbp, offset, "compatible", "aaa", 3);
}

void
board_init(void)
{
	capability cap;
	int error;

	pvAlmightyDataCap = mdx_getdefault();
	pvAlmightyCodeCap = NULL;

	/* Initialize malloc */
	cap = mdx_getdefault();
	cap = mdx_setoffset(cap, PHYS_TO_DMAP(0xf8800000));
#ifdef __CHERI_PURE_CAPABILITY__
	malloc_init_purecap(cap);
#else
	malloc_init();
#endif
	malloc_add_region(cap, 0x7800000);

	/*
	 * The chosen device is used by CheriBSD on the 1st core.
	 * Ensure we don't attach it here by replacing compatible string.
	 */
	fdt_relocate();
	fdt_patch();

	mi_startup();

	dev_uart = mdx_device_lookup_by_name("uart_16550", 0);
	mdx_console_register_uart(dev_uart);

#if 0
	/* Register UART */
	cap = mdx_getdefault();
	cap = mdx_setoffset(cap, UART_BASE);
	cap = mdx_setbounds(cap, 1024);

	uart_16550_init(&dev_uart, cap, 0, UART_CLOCK_RATE);
	mdx_uart_setup(&dev_uart, DEFAULT_BAUDRATE,
	    UART_DATABITS_5,
	    UART_STOPBITS_1,
	    UART_PARITY_NONE);
	mdx_console_register_uart(&dev_uart);
#endif

	error = mdx_of_check_header();
	if (error)
		printf("%s: FDT header BAD, error %d\n", __func__, error);
	else
		printf("%s: FDT header OK\n", __func__);

#if 0
	cap = mdx_getdefault();
	cap = mdx_setoffset(cap, PLIC_BASE);
	plic_init(&dev_plic, cap, 0, 1);
	plic_init(&dev_plic, cap, 1, 3);
#endif

	/* Release secondary core(s) */

#ifdef MDX_SCHED_SMP
	int j;

	printf("Releasing CPUs...\n");

	for (j = 0; j < MDX_CPU_MAX; j++)
		__riscv_boot_ap[j] = 1;
#endif
}
