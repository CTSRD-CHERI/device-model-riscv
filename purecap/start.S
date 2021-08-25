/*-
 * Copyright (c) 2019 Ruslan Bukin <br@bsdpad.com>
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

#include <machine/asm.h>

ASM_ENTRY(_start)
	fence.i
	fence

#ifdef __CHERI_PURE_CAPABILITY__
.option push
.option nocapmode
	lla	t0, 1f
	cspecialr	ct1, pcc
	csetaddr	ct1, ct1, t0
	li		t0, 1
	csetflags	ct1, ct1, t0
#ifdef __riscv_xcheri_mode_dependent_jumps
	jr.cap	ct1
#else
	cjr	ct1
#endif
1:
.option pop
#endif

	/* Setup supervisor trap vector */
	cllc		ct0, cpu_exception_handler
	cspecialw	stcc, ct0
	cmove		ct1, cnull
	cspecialw	stdc, ct1

#if 0
	la	t0, cpu_exception_handler
	csrw	stvec, t0

	li	t0, 0
	bne	a0, t0, mpentry
#endif

#if 0
	la	t0, cpu_lottery
	li	t1, 1
	amoadd.w t0, t1, 0(t0)
	bnez	t0, mpentry
#endif

	/* Clear BSS. */
	cllc	cs0,	_sbss
	cllc	cs1,	_ebss
1:
	csb		zero, 0(cs0)
	cincoffset	cs0, cs0, 1
	bltu		s0, s1, 1b

#if 0
	la	t0, _sbss
	la	t1, _ebss
1:
	beq	t0, t1, 2f
	li	t2, 0
	sb	t2, 0(t0)
	addi	t0, t0, 1
	j	1b
2:
#endif

	li		a0,	0

	/* Initialize stack pointer */
	cllc		cs3, idle_thread_stack
	li		t1, MDX_THREAD_STACK_SIZE
	mulw		t2, t1, a0
	cincoffset	cs3, cs3, t2	# Start of stack
	csetbounds	cs3, cs3, t1
	cincoffset	csp, cs3, t1	# End of stack

	/* Initialize cap relocs. */
	cllc		ca1, start_purecap
	cjalr		ca1

	clgc		cra, md_init
	cjalr		cra

	fence

	j	md_init

	/* NOT REACHED */

ASM_ENTRY(mpentry)
#ifdef MDX_SCHED_SMP
	la	t0, __riscv_boot_ap
	add	t0, t0, a0

1:
	lb	t1, 0(t0)
	beqz	t1, 1b

	la	t0, idle_thread_stack
	li	t1, MDX_THREAD_STACK_SIZE
	mulw	t2, t1, a0
	add	t2, t2, t1
	add	sp, t2, t0

	fence

	j	md_init_secondary
#else
1:
	j	1b
#endif
END(mpentry)

cpu_lottery:
	.space 4
