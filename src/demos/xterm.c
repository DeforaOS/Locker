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
#ifndef BINDIR
# define BINDIR		PREFIX "/bin"
#endif


/* XTerm */
/* private */
/* types */
typedef struct _XTermWindow
{
	GdkWindow * window;
	GPid pid;
} XTermWindow;

typedef struct _LockerDemo
{
	LockerDemoHelper * helper;

	/* windows */
	XTermWindow * windows;
	size_t windows_cnt;
} XTerm;


/* prototypes */
/* plug-in */
static XTerm * _xterm_init(LockerDemoHelper * helper);
static void _xterm_destroy(XTerm * xterm);
static int _xterm_add(XTerm * xterm, GdkWindow * window);
static void _xterm_remove(XTerm * xterm, GdkWindow * window);
static void _xterm_start(XTerm * xterm);
static void _xterm_stop(XTerm * xterm);


/* public */
/* variables */
/* plug-in */
LockerDemoDefinition plugin =
{
	"XTerm",
	"terminal",
	NULL,
	_xterm_init,
	_xterm_destroy,
	NULL,
	_xterm_add,
	_xterm_remove,
	_xterm_start,
	_xterm_stop,
	NULL
};


/* private */
/* functions */
/* plug-in */
/* xterm_init */
static XTerm * _xterm_init(LockerDemoHelper * helper)
{
	XTerm * xterm;

	if((xterm = object_new(sizeof(*xterm))) == NULL)
		return NULL;
	xterm->helper = helper;
	xterm->windows = NULL;
	xterm->windows_cnt = 0;
	return xterm;
}


/* xterm_destroy */
static void _xterm_destroy(XTerm * xterm)
{
	size_t i;

	/* kill the remaining children */
	for(i = 0; i < xterm->windows_cnt; i++)
		if(xterm->windows[i].pid > 0)
			kill(xterm->windows[i].pid, SIGTERM);
	free(xterm->windows);
	object_delete(xterm);
}


/* xterm_add */
static XTermWindow * _add_allocate(XTerm * xterm);

static int _xterm_add(XTerm * xterm, GdkWindow * window)
{
	int ret = 0;
	LockerDemoHelper * helper = xterm->helper;
	XTermWindow * w;
	unsigned long id = GDK_WINDOW_XID(window);
	GError * error = NULL;
	char * argv[] = { NULL, "-into", NULL, "-e", NULL, NULL };
	char const * p;
	char buf[16];

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %u\n", __func__, id);
#endif
	if((w = _add_allocate(xterm)) == NULL)
		return -1; /* XXX report error */
	if((p = helper->config_get(helper->locker, "xterm", "xterm")) != NULL)
		argv[0] = strdup(p);
	else
		argv[0] = strdup(BINDIR "/xterm");
	if((p = helper->config_get(helper->locker, "xterm", "command")) != NULL)
		argv[4] = strdup(p);
	else
		argv[4] = strdup("top");
	snprintf(buf, sizeof(buf), "%lu", id);
	argv[2] = buf;
	if(argv[0] == NULL || argv[4] == NULL)
	{
		free(argv[0]);
		free(argv[4]);
		return -helper->error(NULL, strerror(errno), 1);
	}
	if(g_spawn_async(NULL, argv, NULL, 0, NULL, NULL, &w->pid, &error)
			!= TRUE)
	{
		ret = -helper->error(NULL, error->message, 1);
		g_error_free(error);
	}
	w->window = window;
	return ret;
}

static XTermWindow * _add_allocate(XTerm * xterm)
{
	XTermWindow * w;
	size_t i;

	/* look for a free window */
	for(i = 0; i < xterm->windows_cnt; i++)
		if(xterm->windows[i].window == NULL)
			return &xterm->windows[i];
	/* allocate a window */
	i = xterm->windows_cnt + 1;
	if((w = realloc(xterm->windows, sizeof(*w) * i)) == NULL)
		return NULL;
	xterm->windows = w;
	w = &xterm->windows[xterm->windows_cnt++];
	w->window = NULL;
	w->pid = -1;
	return w;
}


/* xterm_remove */
static void _xterm_remove(XTerm * xterm, GdkWindow * window)
{
	size_t i;
	XTermWindow * w;

	for(i = 0; i < xterm->windows_cnt; i++)
		if(xterm->windows[i].window == window)
		{
			w = &xterm->windows[i];
			w->window = NULL;
			kill(w->pid, SIGTERM);
			w->pid = -1;
			return;
		}
	/* FIXME free some memory */
}


/* xterm_start */
static void _xterm_start(XTerm * xterm)
{
	size_t i;

	for(i = 0; i < xterm->windows_cnt; i++)
		if(xterm->windows[i].pid > 0)
			kill(xterm->windows[i].pid, SIGCONT);
}


/* xterm_stop */
static void _xterm_stop(XTerm * xterm)
{
	size_t i;

	for(i = 0; i < xterm->windows_cnt; i++)
		if(xterm->windows[i].pid > 0)
			kill(xterm->windows[i].pid, SIGSTOP);
}
