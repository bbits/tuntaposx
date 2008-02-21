/*
 * ethertap device for MacOSX.
 *
 * Kext definition (it is a mach kmod really...)
 */
/*
 * Copyright (c) 2004, 2005, 2006, 2007, 2008 Mattias Nissler <mattias.nissler@gmx.de>
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this list of
 *      conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice, this list of
 *      conditions and the following disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *   3. The name of the author may not be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "tap.h"

extern "C" {

#include <sys/param.h>

#include <mach/kmod.h>

static tap_manager *mgr;

/*
 * start function. called when the kext gets loaded.
 */
static kern_return_t tap_module_start(struct kmod_info *ki, void *data)
{
	/* initialize locking */
	if (!tt_lock::initialize())
		return KMOD_RETURN_FAILURE;

	/* create a tap manager that will handle the rest */
	mgr = new tap_manager();

	if (mgr != NULL) {
		if (mgr->initialize(TAP_IF_COUNT, TAP_FAMILY_NAME))
			return KMOD_RETURN_SUCCESS;

		delete mgr;
		mgr = NULL;
		/* clean up locking */
		tt_lock::shutdown();
	}

	return KMOD_RETURN_FAILURE;
}

/*
 * stop function. called when the kext should be unloaded. unloading can be prevented by
 * returning failure
 */
static kern_return_t tap_module_stop(struct kmod_info *ki, void *data)
{
	if (mgr != NULL) {
		if (!mgr->shutdown())
			return KMOD_RETURN_FAILURE;

		delete mgr;
		mgr = NULL;
	}

	/* clean up locking */
	tt_lock::shutdown();

	return KMOD_RETURN_SUCCESS;
} 

KMOD_DECL(tap, TAP_KEXT_VERSION)

}

