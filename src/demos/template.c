/* $Id$ */
/* Copyright (c) 2012-2018 Pierre Pronchery <khorben@defora.org> */
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



#ifdef DEBUG
# include <stdio.h>
#endif
#include <System.h>
#include "Locker/demo.h"


/* Template */
/* private */
/* types */
typedef struct _LockerDemo
{
	LockerDemoHelper * helper;
} Template;


/* prototypes */
/* plug-in */
static Template * _template_init(LockerDemoHelper * helper);
static void _template_destroy(Template * template);
static void _template_reload(Template * template);
static int _template_add(Template * template, GdkWindow * window);
static void _template_remove(Template * template, GdkWindow * window);
static void _template_start(Template * template);
static void _template_stop(Template * template);
static void _template_cycle(Template * template);


/* public */
/* variables */
/* plug-in */
LockerDemoDefinition plugin =
{
	"Template",
	NULL,
	NULL,
	_template_init,
	_template_destroy,
	NULL,
	_template_add,
	_template_remove,
	_template_start,
	_template_stop,
	_template_cycle
};


/* private */
/* functions */
/* plug-in */
/* template_init */
static Template * _template_init(LockerDemoHelper * helper)
{
	Template * template;

	if((template = object_new(sizeof(*template))) == NULL)
		return NULL;
	template->helper = helper;
	return template;
}


/* template_destroy */
static void _template_destroy(Template * template)
{
	object_delete(template);
}


/* template_reload */
static void _template_reload(Template * template)
{
}


/* template_add */
static int _template_add(Template * template, GdkWindow * window)
{
	return 0;
}


/* template_remove */
static void _template_remove(Template * template, GdkWindow * window)
{
}


/* template_start */
static void _template_start(Template * template)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
}


/* template_stop */
static void _template_stop(Template * template)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
}


/* template_cycle */
static void _template_cycle(Template * template)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
}
