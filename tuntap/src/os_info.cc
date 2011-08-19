/*
 * Helper routines to determine information about the OS the kext is running under.
 */

/*
 * Copyright (c) 2004, 2005, 2006, 2007, 2008, 2009 Mattias Nissler <mattias.nissler@gmx.de>
 * Copyright (c) 2011 Dave Peck <davepeck [at] gmail [dot] com>
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


#include "os_info.h"
#include "mem.h"

#if 0
#define dprintf(...)			log(LOG_INFO, __VA_ARGS__)
#else
#define dprintf(...)
#endif


extern "C" {
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
}


/*====================================================*
 * Private helpers for working with kmod_info_t				*
 *====================================================*/ 

/* 
 * Given a kmod_info linked list, and the name of an entry, return the version 
 * string for that entry. return NULL if the name wasn't found.
 *
 * the returned string is only valid for as long as the kmod_info list is.
 */
const char* _get_version_string(struct kmod_info *ki, const char *name) {
	char *version_string = NULL;
	
	dprintf("tuntap: _get_version_string: looking for name = %s\n", name);
	
	for (struct kmod_info *current_ki = ki; 
			 current_ki != NULL; 
			 current_ki = current_ki->next) {
		if (current_ki->name != NULL && strcmp(current_ki->name, name) == 0) {
			version_string = current_ki->version;
			dprintf("tuntap: _get_version_string: found version = %s for name = %s\n", version_string, name);
			break;
		}
	}
	
	return version_string;
}


/* 
 * Return the version string for the com.apple.kpi.bsd kext. We choose
 * this kext because it is the most relevant to the operation of tuntaposx.
 * Looking at the XNU sources reveals that this is always set to 
 * the osrevision -- so it should be a good proxy for the real thing.
 * 
 * The returned string is only valid for as long as the kmod_info list is.
 */
const char* _get_kpi_bsd_version_string(struct kmod_info *ki) {
	return _get_version_string(ki, "com.apple.kpi.bsd");
}


/* 
 * Given an osrevision version string of the form x.x.x, return the major
 * (first) version of the string as an int32_t.
 *
 * return 0 if the major version couldn't be converted properly.
 */
int32_t _get_major_version_from_string(const char *version_string) {
	int32_t major_version = 0;
	
	if (version_string != NULL) {
		/* This is ugly and raw because I wanted to avoid using dangerous
		 * stuff like strtok or sscanf. Maybe there's a cleaner answer?
		 */
		dprintf("tuntap: _get_major_version_from_string: version_string = %s\n", version_string);
		const char *first_period = strchr(version_string, '.');
		if (first_period != NULL) {
			size_t major_string_length = (first_period - version_string);
			char *major_string = (char *) mem_alloc(major_string_length + 1);
			if (major_string != NULL) {
				strncpy(major_string, version_string, major_string_length);
				major_string[major_string_length] = '\0';
				dprintf("tuntap: _get_major_version_from_string: major_string = %s\n", major_string);
				major_version = atoi(major_string);
				dprintf("tuntap: _get_major_version_from_string: major_version = %d\n", major_version);
				mem_free(major_string, major_string_length + 1);
			}
		}
	}
	
	return major_version;
}



/*====================================================*
 * Public API for working with kmod_info_t						*
 *====================================================*/ 

/* Return the major version for the com.apple.bsd.kpi kext, which
 * serves as our proxy for the operating system version.
 *
 * Returns 0 on error.
 */
int32_t get_os_major_version(struct kmod_info *ki) {
	const char *version_string = _get_kpi_bsd_version_string(ki);
	return _get_major_version_from_string(version_string);
}


/* Return true if the given version is at precisely OS X Lion */
bool os_major_version_is_lion(int32_t os_major_version) {
	return (os_major_version == 11);
}


/* Return true if the given string is at least OS X Lion */
bool os_major_version_is_lion_or_later(int32_t os_major_version) {
	return (os_major_version >= 11);
}




