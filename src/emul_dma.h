/*-
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 A. Theodore Markettos
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

#ifndef	_EMUL_DMA_H_
#define	_EMUL_DMA_H_

#include <stdint.h>

#define DMA_INDIR

typedef enum {
    DMA_SUCCESS,
    DMA_FAULT
} dma_status;

// IO virtual address for DMA
typedef uintptr_t dma_iova_t;
typedef uintptr_t dma_iopa_t;

// constructor for a DMA write function for values of 'type'
#define CTR_DMA_WR(type) \
static inline dma_status dma_wr_##type(dma_iova_t iova, type data) {      \
    uintptr_t iova_ptr = (uintptr_t) iova;                              \
    type *pa = (type *) iova_ptr;                                       \
    *pa = data;                                                         \
    return DMA_SUCCESS;                                                 \
}

// constructor for a DMA read function for values of 'type'.
// at present we just return the result, rather than handling faults
#define CTR_DMA_RD(type) \
static inline type dma_rd_##type(dma_iova_t iova) {                       \
    uintptr_t iova_ptr = (uintptr_t) iova;                              \
    type *pa = (type *) iova_ptr;                                       \
    type data = *pa;                                                    \
    return data;                                                        \
}

// construct functions to transfer all the data types
CTR_DMA_WR(uint64_t);
CTR_DMA_WR(int64_t);
CTR_DMA_WR(uint32_t);
CTR_DMA_WR(int32_t);
CTR_DMA_WR(uint16_t);
CTR_DMA_WR(int16_t);
CTR_DMA_WR(uint8_t);
CTR_DMA_WR(int8_t);

CTR_DMA_RD(uint64_t);
CTR_DMA_RD(int64_t);
CTR_DMA_RD(uint32_t);
CTR_DMA_RD(int32_t);
CTR_DMA_RD(uint16_t);
CTR_DMA_RD(int16_t);
CTR_DMA_RD(uint8_t);
CTR_DMA_RD(int8_t);


// same semantics as memcpy()
static inline dma_status dma_wr_block(dma_iova_t iova, void *input, size_t len) {
    void *pa = (void *) iova;
    memcpy(pa, input, len);
    return DMA_SUCCESS;
}

static inline dma_status dma_rd_block(void *output, dma_iova_t iova, size_t len) {
    void *pa = (void *) iova;
    memcpy(output, pa, len);
    return DMA_SUCCESS;
}

static inline dma_iova_t dma_rd_ptr(dma_iova_t iova, size_t index) {
    // IOMMU address translation step: convert the iova into a void**
    uintptr_t iova_ptr = (uintptr_t) iova;
    void **pa = (void **) iova_ptr;
    // deference the void** to return another pointer, adding the offset
    // XXX: assume sizeof(dma_iova_t)==sizeof(void**), else we're in trouble
    void *data = pa[index];
    // convert the pointer to an IOVA type
    dma_iova_t data_iova = (dma_iova_t) data;
    return data_iova;
}

static inline dma_iova_t dma_iova_incbase(dma_iova_t iova, size_t offset, size_t granule)
{
    return iova + offset * granule;
}

#endif	/* !_EMUL_DMA_H_ */
