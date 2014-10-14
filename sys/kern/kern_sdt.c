/*-
 * Copyright 2006-2008 John Birrell <jb@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kdb.h>
#include <sys/linker.h>
#include <sys/linker_set.h>
#include <sys/sdt.h>

SDT_PROVIDER_DEFINE(sdt);

#if 1 || !defined(__amd64__)
/*
 * Hook for the DTrace probe function. The SDT provider will set this to
 * dtrace_probe() when it loads.
 */
sdt_probe_func_t sdt_probe_func = sdt_probe_stub;

/*
 * This is a stub for probe calls in case kernel DTrace support isn't
 * enabled. It should never get called because there is no DTrace support
 * to enable it.
 */
void
sdt_probe_stub(uint32_t id, uintptr_t arg0, uintptr_t arg1,
    uintptr_t arg2, uintptr_t arg3, uintptr_t arg4)
{

	printf("sdt_probe_stub: unexpectedly called\n");
	kdb_backtrace();
}
#endif

#ifdef __amd64__
void
sdt_callplace_patch(struct sdt_callplace *callplace)
{
	uint8_t *ptr = (uint8_t *)callplace->call_addr;
	uint8_t *call_ptr = NULL;
	uint32_t off;
	int32_t disp;

	if (callplace->call_size == 0)
		return;	/* already patched */

	KASSERT(callplace->call_size >= 5,
	    ("call_size %u is too small", callplace->call_size));
	for (off = 0; off < callplace->call_size - 4; off++) {
		if (ptr[off] != 0xe8) /* CALL rel32 */
			continue;

		/*
		 * Compare RIP of the next instruction plus displacement
		 * with expected value.
		 */
		disp = *(int32_t *)(ptr + off + 1); /* displacement */
		if ((uintptr_t)(ptr + off + 5 + disp) != callplace->func_addr)
			continue;

		/*
		 * XXX
		 * Compiler can optimize several SDT calls so that they
		 * will have a single beginning label, thus all call places
		 * would point to the same code area with multiple calls.
		 * Deal with them one by one.
		 * See handle_string:return probe in handle_string()
		 * in linux_sysctl.c as an exmaple.
		 */
		call_ptr = ptr + off;
		break;
	}
	KASSERT(call_ptr != NULL,
	    ("no call candidates in %p:%p", (uint8_t *)callplace->call_addr,
	    (uint8_t *)callplace->call_addr + callplace->call_size));
	callplace->call_addr = (uintptr_t)call_ptr;
	for (off = 0; off < 5; off++)
		call_ptr[off] = 0x90;	/* NOP */
	callplace->call_size = 0;	/* mark as already patched */
}

static int
sdt_callplaces_preprocess(linker_file_t lf, void *arg __unused)
{
	struct sdt_callplace **callplace, **begin, **end;

	if (linker_file_lookup_set(lf, "sdt_calls", &begin, &end, NULL) != 0)
		return (0);

	for (callplace = begin; callplace < end; callplace++)
		sdt_callplace_patch(*callplace);
	return (0);
}

static void
sdt_callplaces_init(void *arg __unused)
{
	(void)linker_file_foreach(sdt_callplaces_preprocess, NULL);
}

SYSINIT(sdt_patch_init, SI_SUB_KLD, SI_ORDER_MIDDLE + 1,
    sdt_callplaces_init, NULL);
#endif
