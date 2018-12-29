/* $Id$ */
/* Copyright (c) 2012 Pierre Pronchery <khorben@defora.org> */
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



#include "Locker.h"
#include <stdio.h>
#include <System.h>


/* Suspend */
/* private */
/* types */
typedef struct _LockerPlugin
{
	LockerPluginHelper * helper;
	gint source;
} Suspend;


/* prototypes */
/* plug-in */
static Suspend * _suspend_init(LockerPluginHelper * helper);
static void _suspend_destroy(Suspend * suspend);
static int _suspend_event(Suspend * suspend, LockerEvent event);

/* callbacks */
static gboolean _suspend_on_timeout(gpointer data);


/* public */
/* variables */
/* plug-in */
LockerPluginDefinition plugin =
{
	"Suspend",
	NULL,
	NULL,
	_suspend_init,
	_suspend_destroy,
	_suspend_event
};


/* private */
/* functions */
/* suspend_init */
static Suspend * _suspend_init(LockerPluginHelper * helper)
{
	Suspend * suspend;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if((suspend = object_new(sizeof(*suspend))) == NULL)
		return NULL;
	suspend->helper = helper;
	suspend->source = 0;
	return suspend;
}


/* suspend_destroy */
static void _suspend_destroy(Suspend * suspend)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(suspend->source != 0)
		g_source_remove(suspend->source);
	object_delete(suspend);
}


/* suspend_event */
static int _suspend_event(Suspend * suspend, LockerEvent event)
{
	switch(event)
	{
		case LOCKER_EVENT_DEACTIVATED:
		case LOCKER_EVENT_UNLOCKED:
			/* cancel any pending suspend */
			if(suspend->source != 0)
				g_source_remove(suspend->source);
			suspend->source = 0;
			break;
		case LOCKER_EVENT_ACTIVATED:
		case LOCKER_EVENT_LOCKED:
			/* queue a suspend if not already */
			if(suspend->source != 0)
				break;
			/* XXX let the delay be configurable */
			suspend->source = g_timeout_add(10000,
					_suspend_on_timeout, suspend);
			break;
		default:
			/* ignore the other events */
			break;
	}
	return 0;
}


/* callbacks */
/* suspend_on_timeout */
static gboolean _suspend_on_timeout(gpointer data)
{
	Suspend * suspend = data;
	LockerPluginHelper * helper = suspend->helper;

	suspend->source = 0;
	helper->action(helper->locker, LOCKER_ACTION_SUSPEND);
	return FALSE;
}
