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



#ifndef DESKTOP_LOCKER_PLUGIN_H
# define DESKTOP_LOCKER_PLUGIN_H

# include "locker.h"


/* LockerPlugin */
/* public */
/* types */
typedef const struct _LockerPluginDefinition LockerPluginDefinition;

typedef struct _LockerPlugin LockerPlugin;

typedef struct _LockerPluginHelper
{
	Locker * locker;
	int (*error)(Locker * locker, char const * message, int ret);
	void (*about_dialog)(Locker * locker);
	int (*action)(Locker * locker, LockerAction action);
	char const * (*config_get)(Locker * locker, char const * section,
			char const * variable);
	int (*config_set)(Locker * locker, char const * section,
			char const * variable, char const * value);
} LockerPluginHelper;

struct _LockerPluginDefinition
{
	char const * name;
	char const * icon;
	char const * description;
	LockerPlugin * (*init)(LockerPluginHelper * helper);
	void (*destroy)(LockerPlugin * plugin);
	int (*event)(LockerPlugin * plugin, LockerEvent event);
};

#endif /* !DESKTOP_LOCKER_PLUGIN_H */
