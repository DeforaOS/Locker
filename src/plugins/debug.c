/* $Id$ */
/* Copyright (c) 2011-2012 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Locker */
/* Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY ITS AUTHORS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */



#include <System.h>
#include <stdio.h>
#include "Locker.h"


/* Debug */
/* private */
/* types */
typedef struct _LockerPlugin
{
	LockerPluginHelper * helper;
} Debug;


/* prototypes */
/* plug-in */
static Debug * _debug_init(LockerPluginHelper * helper);
static void _debug_destroy(Debug * debug);
static int _debug_event(Debug * debug, LockerEvent event);


/* public */
/* variables */
/* plug-in */
LockerPluginDefinition plugin =
{
	"Debug",
	"applications-development",
	NULL,
	_debug_init,
	_debug_destroy,
	_debug_event
};


/* private */
/* functions */
/* debug_init */
static Debug * _debug_init(LockerPluginHelper * helper)
{
	Debug * debug;

	if((debug = object_new(sizeof(*debug))) == NULL)
		return NULL;
	debug->helper = helper;
	fprintf(stderr, "DEBUG: %s()\n", __func__);
	return debug;
}


/* debug_destroy */
static void _debug_destroy(Debug * debug)
{
	fprintf(stderr, "DEBUG: %s()\n", __func__);
	object_delete(debug);
}


/* debug_event */
static int _debug_event(Debug * debug, LockerEvent event)
{
	switch(event)
	{
		case LOCKER_EVENT_ACTIVATED:
			fprintf(stderr, "DEBUG: %s() ACTIVATED\n", __func__);
			break;
		case LOCKER_EVENT_ACTIVATING:
			fprintf(stderr, "DEBUG: %s() ACTIVATING\n", __func__);
			break;
		case LOCKER_EVENT_DEACTIVATED:
			fprintf(stderr, "DEBUG: %s() DEACTIVATED\n", __func__);
			break;
		case LOCKER_EVENT_DEACTIVATING:
			fprintf(stderr, "DEBUG: %s() DEACTIVATING\n", __func__);
			break;
		case LOCKER_EVENT_LOCKED:
			fprintf(stderr, "DEBUG: %s() LOCKED\n", __func__);
			break;
		case LOCKER_EVENT_LOCKING:
			fprintf(stderr, "DEBUG: %s() LOCKING\n", __func__);
			break;
		case LOCKER_EVENT_SUSPENDING:
			fprintf(stderr, "DEBUG: %s() SUSPENDING\n", __func__);
			break;
		case LOCKER_EVENT_UNLOCKED:
			fprintf(stderr, "DEBUG: %s() UNLOCKED\n", __func__);
			break;
		case LOCKER_EVENT_UNLOCKING:
			fprintf(stderr, "DEBUG: %s() UNLOCKING\n", __func__);
			break;
#ifdef DEBUG
		default:
			fprintf(stderr, "DEBUG: %s() Unknown event (%u)\n",
					__func__, event);
			break;
#endif
	}
	return 0;
}
