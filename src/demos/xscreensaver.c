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



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <gdk/gdkx.h>
#include <System.h>
#include "Locker/demo.h"
#include "../../config.h"

#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif


/* XScreensaver */
/* private */
/* types */
typedef struct _XScreensaverWindow
{
	GdkWindow * window;
	GPid pid;
} XScreensaverWindow;

typedef struct _LockerDemo
{
	LockerDemoHelper * helper;

	/* windows */
	XScreensaverWindow * windows;
	size_t windows_cnt;
} XScreensaver;


/* prototypes */
/* plug-in */
static XScreensaver * _xscreensaver_init(LockerDemoHelper * helper);
static void _xscreensaver_destroy(XScreensaver * xscreensaver);
static int _xscreensaver_add(XScreensaver * xscreensaver, GdkWindow * window);
static void _xscreensaver_remove(XScreensaver * xscreensaver,
		GdkWindow * window);
static void _xscreensaver_start(XScreensaver * xscreensaver);
static void _xscreensaver_stop(XScreensaver * xscreensaver);


/* public */
/* variables */
/* plug-in */
LockerDemoDefinition plugin =
{
	"XScreensaver",
	"xscreensaver",
	NULL,
	_xscreensaver_init,
	_xscreensaver_destroy,
	NULL,
	_xscreensaver_add,
	_xscreensaver_remove,
	_xscreensaver_start,
	_xscreensaver_stop,
	NULL
};


/* private */
/* functions */
/* plug-in */
/* xscreensaver_init */
static XScreensaver * _xscreensaver_init(LockerDemoHelper * helper)
{
	XScreensaver * xscreensaver;

	if((xscreensaver = object_new(sizeof(*xscreensaver))) == NULL)
		return NULL;
	xscreensaver->helper = helper;
	xscreensaver->windows = NULL;
	xscreensaver->windows_cnt = 0;
	return xscreensaver;
}


/* xscreensaver_destroy */
static void _xscreensaver_destroy(XScreensaver * xscreensaver)
{
	size_t i;

	/* kill the remaining children */
	for(i = 0; i < xscreensaver->windows_cnt; i++)
		if(xscreensaver->windows[i].pid > 0)
			kill(xscreensaver->windows[i].pid, SIGTERM);
	free(xscreensaver->windows);
	object_delete(xscreensaver);
}


/* xscreensaver_add */
static XScreensaverWindow * _add_allocate(XScreensaver * xscreensaver);

static int _xscreensaver_add(XScreensaver * xscreensaver, GdkWindow * window)
{
	int ret = 0;
	LockerDemoHelper * helper = xscreensaver->helper;
	XScreensaverWindow * w;
	unsigned long id = GDK_WINDOW_XID(window);
	GError * error = NULL;
	char * argv[] = { NULL, "-window-id", NULL, NULL };
	char const * p;
	char buf[16];

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %u\n", __func__, id);
#endif
	if((w = _add_allocate(xscreensaver)) == NULL)
		return -1; /* XXX report error */
	if((p = helper->config_get(helper->locker, "xscreensaver",
					"xscreensaver")) != NULL)
		argv[0] = strdup(p);
	else
		argv[0] = strdup(PREFIX "/libexec/xscreensaver/bsod");
	snprintf(buf, sizeof(buf), "%lu", id);
	argv[2] = buf;
	if(argv[0] == NULL)
		return -helper->error(NULL, strerror(errno), 1);
	if(g_spawn_async(NULL, argv, NULL, 0, NULL, NULL, &w->pid, &error)
			!= TRUE)
	{
		ret = -helper->error(NULL, error->message, 1);
		g_error_free(error);
	}
	w->window = window;
	return ret;
}

static XScreensaverWindow * _add_allocate(XScreensaver * xscreensaver)
{
	XScreensaverWindow * w;
	size_t i;

	/* look for a free window */
	for(i = 0; i < xscreensaver->windows_cnt; i++)
		if(xscreensaver->windows[i].window == NULL)
			return &xscreensaver->windows[i];
	/* allocate a window */
	i = xscreensaver->windows_cnt + 1;
	if((w = realloc(xscreensaver->windows, sizeof(*w) * i)) == NULL)
		return NULL;
	xscreensaver->windows = w;
	w = &xscreensaver->windows[xscreensaver->windows_cnt++];
	w->window = NULL;
	w->pid = -1;
	return w;
}


/* xscreensaver_remove */
static void _xscreensaver_remove(XScreensaver * xscreensaver,
		GdkWindow * window)
{
	size_t i;
	XScreensaverWindow * w;

	for(i = 0; i < xscreensaver->windows_cnt; i++)
		if(xscreensaver->windows[i].window == window)
		{
			w = &xscreensaver->windows[i];
			w->window = NULL;
			kill(w->pid, SIGTERM);
			w->pid = -1;
			return;
		}
	/* FIXME free some memory */
}


/* xscreensaver_start */
static void _xscreensaver_start(XScreensaver * xscreensaver)
{
	size_t i;

	for(i = 0; i < xscreensaver->windows_cnt; i++)
		if(xscreensaver->windows[i].pid > 0)
			kill(xscreensaver->windows[i].pid, SIGCONT);
}


/* xscreensaver_stop */
static void _xscreensaver_stop(XScreensaver * xscreensaver)
{
	size_t i;

	for(i = 0; i < xscreensaver->windows_cnt; i++)
		if(xscreensaver->windows[i].pid > 0)
			kill(xscreensaver->windows[i].pid, SIGSTOP);
}
