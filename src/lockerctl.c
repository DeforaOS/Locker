/* $Id$ */
/* Copyright (c) 2011-2015 Pierre Pronchery <khorben@defora.org> */
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



#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <libintl.h>
#include <gtk/gtk.h>
#include <Desktop.h>
#include "../include/Locker.h"
#include "../config.h"
#define _(string) gettext(string)

/* constants */
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef DATADIR
# define DATADIR	PREFIX "/share"
#endif
#ifndef LOCALEDIR
# define LOCALEDIR	DATADIR "/locale"
#endif
#ifndef PROGNAME
# define PROGNAME	"lockerctl"
#endif


/* lockerctl */
/* private */
/* functions */
/* lockerctl */
static int _lockerctl(int action)
{
	desktop_message_send(LOCKER_CLIENT_MESSAGE, LOCKER_MESSAGE_ACTION,
			action, TRUE);
	return 0;
}


/* usage */
static int _usage(void)
{
#ifdef EMBEDDED
	fprintf(stderr, _("Usage: %s [-D|-E|-S|-c|-l|-s|-u|-z]\n"
"  -D	Temporarily disable the screensaver\n"
"  -E	Enable the screensaver again\n"
"  -S	Display or change settings\n"
"  -c	Cycle the screen saver\n"
"  -l	Lock the screen\n"
"  -s	Activate the screen saver\n"
"  -u	Unlock the screen\n"
"  -z	Suspend the device\n"), PROGNAME);
#else
	fprintf(stderr, _("Usage: %s [-D|-E|-S|-c|-l|-s|-u|-z]\n"
"  -D	Temporarily disable the screensaver\n"
"  -E	Enable the screensaver again\n"
"  -S	Display or change settings\n"
"  -c	Cycle the screen saver\n"
"  -l	Lock the screen\n"
"  -s	Activate the screen saver\n"
"  -u	Unlock the screen\n"
"  -z	Suspend the computer\n"), PROGNAME);
#endif
	return 1;
}


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	int o;
	int action = -1;

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
	gtk_init(&argc, &argv);
	while((o = getopt(argc, argv, "DESclsuz")) != -1)
		switch(o)
		{
			case 'D':
				if(action != -1)
					return _usage();
				action = LOCKER_ACTION_DISABLE;
				break;
			case 'E':
				if(action != -1)
					return _usage();
				action = LOCKER_ACTION_ENABLE;
				break;
			case 'S':
				if(action != -1)
					return _usage();
				action = LOCKER_ACTION_SHOW_PREFERENCES;
				break;
			case 'c':
				if(action != -1)
					return _usage();
				action = LOCKER_ACTION_CYCLE;
				break;
			case 'l':
				if(action != -1)
					return _usage();
				action = LOCKER_ACTION_LOCK;
				break;
			case 's':
				if(action != -1)
					return _usage();
				action = LOCKER_ACTION_ACTIVATE;
				break;
			case 'u':
				if(action != -1)
					return _usage();
				action = LOCKER_ACTION_UNLOCK;
				break;
			case 'z':
				if(action != -1)
					return _usage();
				action = LOCKER_ACTION_SUSPEND;
				break;
			default:
				return _usage();
		}
	if(action == -1 || optind != argc)
		return _usage();
	return (_lockerctl(action) == 0) ? 0 : 2;
}
