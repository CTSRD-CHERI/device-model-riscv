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
#include <sys/tick.h>

#include <mips/beri/beri_epw.h>

#include <machine/cpuregs.h>
#include <machine/cpufunc.h>

#include <dev/virtio/virtio.h>
#include <dev/virtio/virtio-blk.h>
#include <dev/virtio/virtio-net.h>

#include <app/fpu_test/fpu_test.h>
#include <app/callout_test/callout_test.h>

#include "board.h"
#include "device-model.h"

#define	VIRTIO_NET_MMIO_BASE	0x10007000

static struct epw_softc epw_sc;

static void
virtio_test(void)
{
	struct virtio_device *vd;
	struct virtio_net *net;

	vd = virtio_setup_vd((void *)VIRTIO_NET_MMIO_BASE);

	net = virtionet_open(vd);

	printf("net is %p\n", net);
}

int
main(void)
{

#ifdef MDX_VIRTIO
	virtio_test();
#endif

	printf("%s\n", __func__);

	capability base, window;
	base = (capability)0x50000000;
	window = (capability)0x60000000;

printf("%s 1\n", __func__);
	epw_init(&epw_sc, base, window);
printf("%s 2\n", __func__);
	/* Enable EPW */
	epw_control(&epw_sc, 1);
printf("%s 3\n", __func__);

	dm_init(&epw_sc);
printf("%s 4\n", __func__);
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
