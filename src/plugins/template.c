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


/* Template */
/* private */
/* types */
typedef struct _LockerPlugin
{
	LockerPluginHelper * helper;
} Template;


/* prototypes */
/* plug-in */
static Template * _template_init(LockerPluginHelper * helper);
static void _template_destroy(Template * template);
static int _template_event(Template * template, LockerEvent event);


/* public */
/* variables */
/* plug-in */
LockerPluginDefinition plugin =
{
	"Template",
	NULL,
	NULL,
	_template_init,
	_template_destroy,
	_template_event
};


/* private */
/* functions */
/* template_init */
static Template * _template_init(LockerPluginHelper * helper)
{
	Template * template;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if((template = object_new(sizeof(*template))) == NULL)
		return NULL;
	template->helper = helper;
	return template;
}


/* template_destroy */
static void _template_destroy(Template * template)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	object_delete(template);
}


/* template_event */
static int _template_event(Template * template, LockerEvent event)
{
	switch(event)
	{
		case LOCKER_EVENT_ACTIVATED:
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() ACTIVATED\n", __func__);
#endif
			break;
		case LOCKER_EVENT_ACTIVATING:
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() ACTIVATING\n", __func__);
#endif
			break;
		case LOCKER_EVENT_DEACTIVATED:
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() DEACTIVATED\n", __func__);
#endif
			break;
		case LOCKER_EVENT_DEACTIVATING:
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() DEACTIVATING\n", __func__);
#endif
			break;
		case LOCKER_EVENT_LOCKED:
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() LOCKED\n", __func__);
#endif
			break;
		case LOCKER_EVENT_LOCKING:
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() LOCKING\n", __func__);
#endif
			break;
		case LOCKER_EVENT_SUSPENDING:
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() SUSPENDING\n", __func__);
#endif
			break;
		case LOCKER_EVENT_UNLOCKED:
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() UNLOCKED\n", __func__);
#endif
			break;
		case LOCKER_EVENT_UNLOCKING:
#ifdef DEBUG
			fprintf(stderr, "DEBUG: %s() UNLOCKING\n", __func__);
#endif
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
