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
#include <sys/cheri.h>

#include <machine/vmparam.h>

#include <dev/virtio/virtio.h>
#include <dev/virtio/virtio-blk.h>
#include <dev/virtio/virtio-net.h>
#include <dev/intc/intc.h>

#include "device-model.h"
#include "bhyve/pci_e82545.h"

#include "virtio.h"

#define	VIRTIO_NET_MMIO_BASE	PHYS_TO_DMAP(0x10007000)
#define	VIRTIO_MAX_BUF		131072

#define	VIRTIO_DEBUG
#undef	VIRTIO_DEBUG

#ifdef	VIRTIO_DEBUG
#define	dprintf(fmt, ...)	printf(fmt, ##__VA_ARGS__)
#else
#define	dprintf(fmt, ...)
#endif

extern struct e82545_softc *e82545_sc;

static struct virtio_device *vd;
static struct virtio_net *vnet;
static char netbuf[VIRTIO_MAX_BUF];

extern void *pvAlmightyDataCap;

static void
net_intr(void *arg, int irq)
{

	dprintf("%s\n", __func__);

	e82545_rx_event(e82545_sc);

	virtionet_handle_interrupt(vnet);
}

void
virtio_init(void)
{
	mdx_device_t dev;
	capability cap;

	cap = pvAlmightyDataCap;
	cap = mdx_setoffset(cap, VIRTIO_NET_MMIO_BASE);
	cap = mdx_setbounds(cap, 0x10000000);

	vd = virtio_setup_vd(cap);
	dprintf("%s: vd is %p\n", __func__, vd);

	vnet = virtionet_open(vd);
	dprintf("%s: vnet is %p\n", __func__, vnet);

	dev = mdx_device_lookup_by_name("plic", 0);
	mdx_intc_setup(dev, 7, net_intr, NULL);
	mdx_intc_enable(dev, 7);
}

int
dm_process_rx(struct iovec *iov, int iovcnt)
{
	int err;
	int i;

	i = 0;

	do {
		err = virtionet_read(vnet, (char *)iov[i].iov_base,
		    iov[i].iov_len);
		dprintf("%s: read %d bytes (base %#lp len %d)\n", __func__,
		    err, iov[i].iov_base, iov[i].iov_len);
		i++;
		if (i >= iovcnt)
			break;
	} while (err != 0);

	return (err);
}

void
dm_process_tx(struct iovec *iov, int iovcnt)
{
	int tot_len;
	void *buf;
	int error;
	int len;
	int i;

	dprintf("%s: cnt %d\n", __func__, iovcnt);

	tot_len = 0;

	for (i = 0; i < iovcnt; i++) {
		buf = iov[i].iov_base;
		len = iov[i].iov_len;
		if (tot_len + len >= VIRTIO_MAX_BUF)
			panic("buffer is too small: %d >= %d",
			    tot_len + len, VIRTIO_MAX_BUF);
		memcpy(&netbuf[tot_len], buf, len);
		tot_len += len;
	}

	dprintf("%s: sending %d packets, tot size %d\n", __func__, i, tot_len);
	error = virtionet_write(vnet, netbuf, tot_len);
	(void) error;
	dprintf("%s: write error %d\n", __func__, error);
}
