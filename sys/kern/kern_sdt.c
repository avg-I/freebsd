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
#include <sys/eventhandler.h>
#include <sys/kernel.h>
#include <sys/kdb.h>
#include <sys/linker.h>
#include <sys/linker_set.h>
#include <sys/stddef.h>
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
static void
sdt_callplace_patch(struct sdt_callplace *callplace)
{
	uint8_t *ptr = (uint8_t *)callplace->offset;
	ptrdiff_t off;

	for (off = -1; off < 4; off++)
		ptr[off] = 0x90;	/* NOP */
}

static int
sdt_lf_patch(linker_file_t lf, void *arg __unused)
{
	struct sdt_callplace **callplace, **begin, **end;

	if (linker_file_lookup_set(lf, "sdt_calls", &begin, &end, NULL) != 0)
		return (0);

	for (callplace = begin; callplace < end; callplace++)
		sdt_callplace_patch(*callplace);
	return (0);
}

static void
sdt_lf_patch_wrapper(void *arg, linker_file_t lf)
{

	sdt_lf_patch(lf, arg);
}

static void
sdt_callplaces_init(void *arg __unused)
{

	(void)EVENTHANDLER_REGISTER(kld_load, sdt_lf_patch_wrapper, NULL,
	    EVENTHANDLER_PRI_FIRST);
	(void)linker_file_foreach(sdt_lf_patch, NULL);
}

SYSINIT(sdt_patch_init, SI_SUB_KLD, SI_ORDER_MIDDLE + 1,
    sdt_callplaces_init, NULL);
#endif
