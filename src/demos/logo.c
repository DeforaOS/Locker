/* $Id$ */
/* Copyright (c) 2012-2022 Pierre Pronchery <khorben@defora.org> */
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
#include <stdlib.h>
#ifdef DEBUG
# include <stdio.h>
#endif
#include <string.h>
#include <time.h>
#include <System.h>
#include "Locker/demo.h"
#include "../../config.h"

/* constants */
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef DATADIR
# define DATADIR	PREFIX "/share"
#endif

/* macros */
#ifndef MIN
# define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif


/* Logo */
/* private */
/* types */
typedef struct _LogoTheme
{
	char const * name;
	char const * background;
	char const * logo;
} LogoTheme;

typedef struct _LogoWindow
{
	GdkWindow * window;
	GdkPixbuf * frame;
#if GTK_CHECK_VERSION(3, 0, 0)
	cairo_t * cairo;
#else
	GdkPixmap * pixmap;
#endif
} LogoWindow;

typedef struct _LockerDemo
{
	LockerDemoHelper * helper;
	GdkPixbuf * background;
	GdkPixbuf * logo;
	LogoWindow * windows;
	size_t windows_cnt;
	guint source;
	guint frame_num;

	/* settings */
	int scroll;
	int opacity;
} Logo;


/* constants */
static const LogoTheme _logo_themes[] =
{
	{ "gnome", DATADIR "/pixmaps/backgrounds/gnome/background-default.jpg",
		DATADIR "/icons/gnome/256x256/places/start-here.png" },
	{ NULL, NULL, NULL }
};
#define LOGO_THEME_DEFAULT 0


/* prototypes */
/* plug-in */
static Logo * _logo_init(LockerDemoHelper * helper);
static void _logo_destroy(Logo * logo);
static void _logo_reload(Logo * logo);
static int _logo_add(Logo * logo, GdkWindow * window);
static void _logo_remove(Logo * logo, GdkWindow * window);
static void _logo_start(Logo * logo);
static void _logo_stop(Logo * logo);
static void _logo_cycle(Logo * logo);

/* useful */
static int _logo_load(Logo * logo);

/* callbacks */
static gboolean _logo_on_timeout(gpointer data);


/* public */
/* variables */
/* plug-in */
LockerDemoDefinition plugin =
{
	"Logo",
	"start-here",
	NULL,
	_logo_init,
	_logo_destroy,
	_logo_reload,
	_logo_add,
	_logo_remove,
	_logo_start,
	_logo_stop,
	_logo_cycle
};


/* private */
/* functions */
/* plug-in */
/* logo_init */
static Logo * _logo_init(LockerDemoHelper * helper)
{
	Logo * logo;

	if((logo = object_new(sizeof(*logo))) == NULL)
		return NULL;
	/* initialization */
	logo->helper = helper;
	logo->background = NULL;
	logo->logo = NULL;
	logo->windows = NULL;
	logo->windows_cnt = 0;
	logo->source = 0;
	logo->frame_num = 0;
	logo->scroll = 0;
	logo->opacity = 255;
	logo->background = NULL;
	logo->logo = NULL;
	_logo_load(logo);
	return logo;
}


/* logo_destroy */
static void _logo_destroy(Logo * logo)
{
#if GTK_CHECK_VERSION(3, 0, 0)
	size_t i;

	for(i = logo->windows_cnt; i > 0; i--)
		if(logo->windows[i - 1].window != NULL)
			_logo_remove(logo, logo->windows[i - 1].window);
#endif
	free(logo->windows);
	object_delete(logo);
}


/* logo_reload */
static void _logo_reload(Logo * logo)
{
	LockerDemoHelper * helper = logo->helper;
	char const * p;
	int opacity;

	/* FIXME implement the rest */
	/* scrolling */
	if((p = helper->config_get(helper->locker, "logo", "scroll")) != NULL)
		logo->scroll = strtol(p, NULL, 10);
	/* opacity */
	if((p = helper->config_get(helper->locker, "logo", "opacity")) != NULL)
	{
		opacity = strtol(p, NULL, 10);
		if(opacity >= 0 && opacity <= 255)
			logo->opacity = opacity;
	}
	if(logo->source != 0)
	{
		g_source_remove(logo->source);
		logo->source = 0;
	}
	logo->source = g_idle_add(_logo_on_timeout, logo);
}


/* logo_add */
static int _logo_add(Logo * logo, GdkWindow * window)
{
	LogoWindow * p;
#if GTK_CHECK_VERSION(3, 0, 0)
	GdkRGBA color = { 0.0, 0.0, 0.0, 0.0 };
#else
	GdkColor color = { 0x0, 0x0, 0x0, 0x0 };
#endif

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() window=%p\n", __func__, (void *)window);
#endif
	if((p = realloc(logo->windows, sizeof(*p) * (logo->windows_cnt + 1)))
			== NULL)
		return -1;
	logo->windows = p;
	p = &logo->windows[logo->windows_cnt++];
	/* set the default color */
#if GTK_CHECK_VERSION(3, 0, 0)
	gdk_window_set_background_rgba(window, &color);
#else
	gdk_window_set_background(window, &color);
	gdk_window_clear(window);
#endif
	p->window = window;
	p->frame = NULL;
#if GTK_CHECK_VERSION(3, 0, 0)
	p->cairo = gdk_cairo_create(window);
#else
	p->pixmap = NULL;
#endif
	return 0;
}


/* logo_remove */
static void _logo_remove(Logo * logo, GdkWindow * window)
{
	size_t i;

	for(i = 0; i < logo->windows_cnt; i++)
		if(logo->windows[i].window == window)
		{
			logo->windows[i].window = NULL;
			if(logo->windows[i].frame != NULL)
				g_object_unref(logo->windows[i].frame);
			logo->windows[i].frame = NULL;
#if GTK_CHECK_VERSION(3, 0, 0)
			if(logo->windows[i].cairo != NULL)
				cairo_destroy(logo->windows[i].cairo);
			logo->windows[i].cairo = NULL;
#else
			if(logo->windows[i].pixmap != NULL)
				gdk_pixmap_unref(logo->windows[i].pixmap);
			logo->windows[i].pixmap = NULL;
#endif
		}
	/* FIXME reorganize the array and free memory */
	for(i = 0; i < logo->windows_cnt; i++)
		if(logo->windows[i].window != NULL)
			break;
	if(i == logo->windows_cnt)
	{
		/* there are no windows left */
		_logo_stop(logo);
		free(logo->windows);
		logo->windows = NULL;
		logo->windows_cnt = 0;
	}
}


/* logo_start */
static void _logo_start(Logo * logo)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(logo->source == 0)
		logo->source = g_idle_add(_logo_on_timeout, logo);
}


/* logo_stop */
static void _logo_stop(Logo * logo)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(logo->source != 0)
		g_source_remove(logo->source);
	logo->source = 0;
}


/* logo_cycle */
static void _logo_cycle(Logo * logo)
{
	/* cycle only when not scrolling */
	if(logo->scroll != 0)
		return;
	if(logo->source != 0)
		g_source_remove(logo->source);
	logo->source = g_idle_add(_logo_on_timeout, logo);
}


/* useful */
/* logo_load */
static int _logo_load(Logo * logo)
{
	int ret = 0;
	LockerDemoHelper * helper = logo->helper;
	size_t i = LOGO_THEME_DEFAULT;
	String const * p;
	GdkPixbuf * pixbuf;
	GError * error = NULL;

	/* load the theme configured */
	if((p = helper->config_get(helper->locker, "logo", "theme")) != NULL)
		for(i = 0; _logo_themes[i].name != NULL; i++)
			if(strcmp(_logo_themes[i].name, p) == 0)
				break;
	/* load the background */
	if((p = _logo_themes[i].background) == NULL
			&& (p = helper->config_get(helper->locker, "logo",
					"background")) == NULL)
		p = _logo_themes[LOGO_THEME_DEFAULT].background;
	if((pixbuf = gdk_pixbuf_new_from_file(p, &error)) == NULL)
	{
		ret = -helper->error(NULL, error->message, 1);
		g_error_free(error);
		error = NULL;
	}
	else
	{
		if(logo->background != NULL)
			g_object_unref(logo->background);
		logo->background = pixbuf;
	}
	/* load the logo */
	if((p = _logo_themes[i].logo) == NULL
			&& (p = helper->config_get(helper->locker, "logo",
					"logo")) == NULL)
		p = _logo_themes[LOGO_THEME_DEFAULT].logo;
	if((pixbuf = gdk_pixbuf_new_from_file(p, &error)) == NULL)
	{
		ret = -helper->error(NULL, error->message, 1);
		g_error_free(error);
		error = NULL;
	}
	else
	{
		if(logo->logo != NULL)
			g_object_unref(logo->logo);
		logo->logo = pixbuf;
	}
	logo->scroll = 0;
	logo->opacity = 255;
	_logo_reload(logo);
	return ret;
}


/* callbacks */
/* logo_on_timeout */
static void _timeout_window(Logo * logo, LogoWindow * window);

static gboolean _logo_on_timeout(gpointer data)
{
	Logo * logo = data;
	size_t i;

	for(i = 0; i < logo->windows_cnt; i++)
		_timeout_window(logo, &logo->windows[i]);
	logo->frame_num += logo->scroll;
	logo->source = g_timeout_add((logo->scroll != 0) ? 40 : 10000,
			_logo_on_timeout, logo);
	return FALSE;
}

static void _timeout_window(Logo * logo, LogoWindow * window)
{
	GdkWindow * w;
	GdkRectangle rect;
#if !GTK_CHECK_VERSION(3, 0, 0)
	int depth;
#endif
	GdkPixbuf * frame;
	int width = 0;
	int height = 0;
	int i;
	int j;
	int x = 0;
	int y = 0;
	int offset_x = 0;
	int offset_y = 0;
	int src_x;
	int src_y;
	int src_w;
	int src_h;
	int seed = time(NULL) ^ getpid() ^ getppid() ^ getuid() ^ getgid();
	const int black = 0x000000ff;

	if((w = window->window) == NULL)
		return;
#if GTK_CHECK_VERSION(3, 0, 0)
	gdk_window_get_geometry(w, &rect.x, &rect.y, &rect.width, &rect.height);
#else
	gdk_window_get_geometry(w, &rect.x, &rect.y, &rect.width, &rect.height,
			&depth);
#endif
	/* reallocate the frame and background if necessary */
	if(window->frame == NULL
			|| gdk_pixbuf_get_width(window->frame) != rect.width
			|| gdk_pixbuf_get_height(window->frame) != rect.height)
	{
		if(window->frame != NULL)
			g_object_unref(window->frame);
		window->frame = gdk_pixbuf_new(GDK_COLORSPACE_RGB, 1, 8,
				rect.width, rect.height);
#if GTK_CHECK_VERSION(3, 0, 0)
		cairo_destroy(window->cairo);
		window->cairo = gdk_cairo_create(window->window);
#else
		if(window->pixmap != NULL)
			gdk_pixmap_unref(window->pixmap);
		window->pixmap = gdk_pixmap_new(w, rect.width, rect.height, -1);
#endif
	}
	frame = window->frame;
	gdk_pixbuf_fill(frame, black);
	/* draw the background */
	if(logo->background != NULL)
	{
		width = gdk_pixbuf_get_width(logo->background);
		height = gdk_pixbuf_get_height(logo->background);
		if((logo->scroll != 0) && width > 0 && height > 0)
		{
			offset_x = logo->frame_num % width;
			offset_y = logo->frame_num % height;
		}
	}
	src_y = offset_y;
	for(j = 0; height > 0 && j < rect.height; j += src_h)
	{
		src_h = MIN(height - src_y, rect.height - j);
		src_x = offset_x;
		for(i = 0; width > 0 && i < rect.width; i += src_w)
		{
			src_w = MIN(width - src_x, rect.width - i);
			gdk_pixbuf_copy_area(logo->background, src_x, src_y,
					src_w, src_h, frame, i, j);
			src_x = 0;
		}
		src_y = 0;
	}
	/* draw the logo */
	if(logo->logo != NULL)
	{
		width = gdk_pixbuf_get_width(logo->logo);
		width = MIN(rect.width, width);
		height = gdk_pixbuf_get_height(logo->logo);
		height = MIN(rect.height, height);
		if(logo->scroll == 0)
		{
			if(rect.width > width)
				x = (rand() ^ seed) % (rect.width - width);
			if(rect.height > height)
				y = (rand() ^ seed) % (rect.height - height);
		}
		else
		{
			if(rect.width > width)
				x = (rect.width - width) / 2;
			if(rect.height > height)
				y = (rect.height - height) / 2;
		}
		gdk_pixbuf_composite(logo->logo, frame, 0, 0,
				rect.width, rect.height, x, y,
				1.0, 1.0, GDK_INTERP_NEAREST, logo->opacity);
	}
#if GTK_CHECK_VERSION(3, 0, 0)
	gdk_cairo_set_source_pixbuf(window->cairo, frame, 0, 0);
	cairo_paint(window->cairo);
#else
	gdk_draw_pixbuf(window->pixmap, NULL, frame, 0, 0, 0, 0, rect.width,
			rect.height, GDK_RGB_DITHER_NONE, 0, 0);
	gdk_window_set_back_pixmap(w, window->pixmap, FALSE);
	gdk_window_clear(w);
#endif
}
