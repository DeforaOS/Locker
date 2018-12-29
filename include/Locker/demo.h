/* $Id$ */
/* Copyright (c) 2011-2018 Pierre Pronchery <khorben@defora.org> */
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



#ifndef DESKTOP_LOCKER_DEMO_H
# define DESKTOP_LOCKER_DEMO_H

# include <gtk/gtk.h>
# include "locker.h"


/* LockerDemo */
/* public */
/* types */
typedef struct _LockerDemo LockerDemo;

typedef struct _LockerDemoHelper
{
	Locker * locker;
	int (*error)(Locker * locker, char const * message, int ret);
	char const * (*config_get)(Locker * locker, char const * section,
			char const * variable);
	int (*config_set)(Locker * locker, char const * section,
			char const * variable, char const * value);
} LockerDemoHelper;

typedef const struct _LockerDemoDefinition
{
	char const * name;
	char const * icon;
	char const * description;
	LockerDemo * (*init)(LockerDemoHelper * helper);
	void (*destroy)(LockerDemo * demo);
	void (*reload)(LockerDemo * demo);
	int (*add)(LockerDemo * demo, GdkWindow * window);
	void (*remove)(LockerDemo * demo, GdkWindow * window);
	void (*start)(LockerDemo * demo);
	void (*stop)(LockerDemo * demo);
	void (*cycle)(LockerDemo * demo);
} LockerDemoDefinition;

#endif /* !DESKTOP_LOCKER_DEMO_H */
