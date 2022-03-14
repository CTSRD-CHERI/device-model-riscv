/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2021 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 *
 * This software was developed by SRI International and the University of
 * Cambridge Computer Laboratory (Department of Computer Science and
 * Technology) under DARPA contract HR0011-18-C-0016 ("ECATS"), as part of the
 * DARPA SSITH research programme.
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
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/cheri.h>

#include <machine/cpuregs.h>

#include "device-model.h"
#include "emul.h"
#include "emul_pci.h"
#include "emul_iommu.h"
#include "bhyve/bhyve_support.h"
#include "bhyve/pci_e82545.h"

#include "virtio.h"
#include "board.h"
#include "prompt.h"

#define	DM_DEBUG
#undef	DM_DEBUG

#ifdef	DM_DEBUG
#define	dprintf(fmt, ...)	printf(fmt, ##__VA_ARGS__)
#else
#define	dprintf(fmt, ...)
#endif

static struct pci_softc pci0_sc;
static int req_count;

#define	DM_EMUL_NDEVICES	2

const struct emul_link emul_map[DM_EMUL_NDEVICES] = {
	{ 0x010000, 0x50000, emul_pci, &pci0_sc, PCI_GENERIC },
	{ 0x510000, 0x10000, emul_iommu, NULL, DM_IOMMU },
};

static int
dm_request(struct epw_softc *sc, struct epw_request *req)
{
	const struct emul_link *elink;
	uint64_t offset;
	int i;

	offset = req->addr - EPW_WINDOW;

	dprintf("%s: offset %lx\n", __func__, offset);

	/* Check if this is an emulation request. */
	for (i = 0; i < DM_EMUL_NDEVICES; i++) {
		elink = &emul_map[i];
		if (offset >= elink->base_emul &&
		    offset < (elink->base_emul + elink->size)) {
			elink->request(elink, sc, req);
			return (0);
		}
	}

	printf("%s: unknown request to offset 0x%lx\n", __func__, offset);

	return (-1);
}

void
dm_init(struct epw_softc *sc)
{
	int error;

	req_count = 0;

#ifdef CONFIG_EMUL_PCI
	error = emul_pci_init(&pci0_sc);
	if (error)
		panic("Can't init PCI\n");
#endif

#ifdef MDX_VIRTIO
	csr_set(sie, SIE_SEIE);
	printf("%s: initializing virtio\n", __func__);
	virtio_init();
#endif

	prompt_init(sc);
}

void
dm_loop(struct epw_softc *sc)
{
	struct epw_request req;

	printf("%s: enter\r\n", __func__);

	while (1) {
		dprintf("trying to get epw_request\r\n");
		if (epw_request(sc, &req) != 0) {
			dprintf("EPW request received\n");
			critical_enter();
			if (req_count++ % 500 == 0)
				printf("%s: req count %d\n",
				    __func__, req_count);

			dm_request(sc, &req);
			epw_reply(sc, &req);
			critical_exit();
		}

#ifdef CONFIG_IOMMU
		__asm __volatile("sfence.vma");
#endif
		e1000_poll();

#ifdef CONFIG_IOMMU
		__asm __volatile("sfence.vma");
#endif
		blockif_thr(NULL);
		prompt_poll(sc);
		/* Optionally we can sleep a bit here. */
	}
}
