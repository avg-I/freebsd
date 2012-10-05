/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)cpu.h	5.4 (Berkeley) 5/9/91
 * $FreeBSD$
 */

#ifndef _MACHINE_CPU_H_
#define	_MACHINE_CPU_H_

/*
 * Definitions unique to i386 cpu support.
 */
#include <machine/psl.h>
#include <machine/frame.h>
#include <machine/segments.h>

#ifdef _KERNEL

/*
 * CPU states for the purpose of communication using MONITOR+MWAIT.
 */
#define	STATE_RUNNING	0x0
#define	STATE_MWAIT	0x1
#define	STATE_SLEEPING	0x2

#define	cpu_exec(p)	/* nothing */
#define	cpu_swapin(p)	/* nothing */
#define	cpu_getstack(td)		((td)->td_frame->tf_rsp)
#define	cpu_setstack(td, ap)		((td)->td_frame->tf_rsp = (ap))
#define	cpu_spinwait()			ia32_pause()

static __inline void
cpu_memwait_once_flags(volatile const void *addr, u_int ext, u_int hints,
    int (*check)(volatile const void *, uintptr_t), uintptr_t arg)
{
	cpu_monitor(__DEQUALIFY(const void *, addr), 0, 0);
	if (!check(addr, arg))
		cpu_mwait(ext, hints);
}

static __inline void
cpu_memwait_once(volatile const void *addr,
    int (*check)(volatile const void *, uintptr_t), uintptr_t arg)
{
	cpu_memwait_once_flags(addr, 0, 0, check, arg);
}


static __inline void
cpu_memwait(volatile const void *addr,
    int (*check)(volatile const void *, uintptr_t), uintptr_t arg)
{
	while (!check(addr, arg))
		cpu_memwait_once(addr, check, arg);
}

#if 0
#define	cpu_wait_flags(var, oldval, ext, hints)			\
	do {							\
		const volatile __typeof__(var) *varp;		\
		if (sizeof(var) <= cpu_mon_min_size) {		\
			cpu_monitor(__DEVOLATILE(&var), 0, 0);	\
			if (*varp == oldval)			\
				cpu_mwait(ext, hints);		\
		} else						\
			cpu_spinwait();				\
	} while (0)
#endif

#define	TRAPF_USERMODE(framep) \
	(ISPL((framep)->tf_cs) == SEL_UPL)
#define	TRAPF_PC(framep)	((framep)->tf_rip)

/*
 * Struct containing pointers to CPU management functions whose
 * implementation is run time selectable.  Selection can be made,
 * for example, based on detection of a particular CPU variant or
 * hypervisor environment.
 */
struct cpu_ops {
	void (*cpu_init)(void);
	void (*cpu_resume)(void);
	void (*ipi_vectored)(u_int, int);
};

extern struct	cpu_ops cpu_ops;

extern char	btext[];
extern char	etext[];

/* Resume hook for VMM. */
extern	void (*vmm_resume_p)(void);

void	cpu_halt(void);
void	cpu_reset(void);
void	fork_trampoline(void);
void	swi_vm(void *);

/*
 * Return contents of in-cpu fast counter as a sort of "bogo-time"
 * for random-harvesting purposes.
 */
static __inline u_int64_t
get_cyclecount(void)
{

	return (rdtsc());
}

#endif

#endif /* !_MACHINE_CPU_H_ */
