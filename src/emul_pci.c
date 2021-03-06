/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2018 Ruslan Bukin <br@bsdpad.com>
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
#include <sys/endian.h>
#include <sys/cheri.h>

#include <machine/cpuregs.h>
#include <machine/cpufunc.h>
#include <machine/frame.h>

#include <mips/beri/beri_epw.h>
#include <dev/pci/pcireg.h>

#include "device-model.h"
#include "emul.h"
#include "emul_pci.h"

#include "bhyve/mem.h"
#include "bhyve/pci_e82545.h"
#include "bhyve/bhyve_support.h"

#define	EMUL_PCI_DEBUG
#undef	EMUL_PCI_DEBUG

#ifdef	EMUL_PCI_DEBUG
#define	dprintf(fmt, ...)	printf(fmt, ##__VA_ARGS__)
#else
#define	dprintf(fmt, ...)
#endif

extern void *pvAlmightyDataCap;

static int
emul_mem(struct pci_softc *sc, struct epw_request *req,
    uint64_t offset)
{
	uint64_t data;
	uint64_t val;
	int error;

	if (req->is_write) {
		KASSERT(req->data_len < 8,
		    ("Wrong access width %d", req->data_len));
		val = req->data;

		error = emulate_mem(sc->ctx, 0, req->addr, req->is_write,
		    req->data_len, &val);
		dprintf("Error %d, val %lx\n", error, val);
	} else {
		error = emulate_mem(sc->ctx, 0, req->addr, req->is_write,
		    req->data_len, (uint64_t *)&data);
		if (error == 0)
			req->data = data;
	}

	return (error);
}

void
emul_pci(const struct emul_link *elink, struct epw_softc *epw_sc,
    struct epw_request *req)
{
	struct pci_softc *sc;
	uint64_t offset;
	int bytes;
	int error;
	int bus, slot, func, coff;
	uint32_t data;

	sc = elink->arg;

	KASSERT(elink->type == PCI_GENERIC, ("Unknown device"));

	dprintf("%s: req->addr %lx, base_emul %lx, epw_window %lx\n",
	    __func__, req->addr, elink->base_emul, EPW_WINDOW);
	offset = req->addr - elink->base_emul - EPW_WINDOW;

	error = emul_mem(sc, req, offset);
	if (error == 0) {
		dprintf("%s: dev req (is_write %d) paddr %lx, val %lx\n",
		    __func__, req->is_write, req->addr, val);
		return;
	}

	coff = offset & 0xfff;
	func = (offset >> 12) & 0x7;
	slot = (offset >> 15) & 0x1f;
	bus = (offset >> 20) & 0xff;

	if (req->is_write) {
		bytes = req->data_len;
		printf("%s (%d/%d/%d): %d-bytes write to %lx\n",
		    __func__, bus, slot, func, bytes, offset);
		data = req->data;
		printf("%s write val %x\n", __func__, data);
		bhyve_pci_cfgrw(sc->ctx, 0, bus, slot, func, coff,
		    bytes, &data);
	} else {
		data = 0;
		bytes = req->data_len;

		printf("%s (%d/%d/%d): %d-bytes read from %lx, ",
		    __func__, bus, slot, func, bytes, offset);
		bhyve_pci_cfgrw(sc->ctx, 1, bus, slot, func, coff,
		    bytes, &data);
		printf("val %x\n", data);
		req->data = data;
	}
}

int
emul_pci_init(struct pci_softc *sc)
{
	uint32_t val;

	sc->ctx = malloc(sizeof(struct vmctx));
	if (sc->ctx == NULL)
		return (-1);

	sc->ctx->cap = pvAlmightyDataCap;

	bhyve_pci_init(sc->ctx);

	/* Test requests */
	bhyve_pci_cfgrw(sc->ctx, 1, 0, 0, 0, 0x00, 2,
	    (uint32_t *)&val);
	printf("slot 0 val 0x%x\n", val);

	bhyve_pci_cfgrw(sc->ctx, 1, 0, 1, 0, 0x00, 2,
	    (uint32_t *)&val);
	printf("slot 1 val 0x%x\n", val);

	return (0);
}
