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
/* TODO:
 * - re-factor and re-indent adequately
 * - improve performance */



#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <gdk/gdkx.h>
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
#ifndef DEMODIR
# define DEMODIR	DATADIR "/gtk-2.0/demo"
#endif

/* macros */
#ifndef MIN
# define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif


/* Gtk-demo */
/* private */
/* types */
typedef enum _GtkDemoImage
{
	GDI_BACKGROUND = 0,
	GDI_APPLE_RED,
	GDI_GNOME_APPLETS,
	GDI_GNOME_CALENDAR,
	GDI_GNOME_FOOT,
	GDI_GNOME_GMUSH,
	GDI_GNOME_GIMP,
	GDI_GNOME_GSAME,
	GDI_GNU_KEYS
} GtkDemoImage;
#define GDI_LAST GDI_GNU_KEYS
#define GDI_COUNT (GDI_LAST + 1)

typedef struct _GtkDemoWindow
{
	GdkWindow * window;
	GdkPixbuf * frame;
#if GTK_CHECK_VERSION(3, 0, 0)
	cairo_t * cairo;
#else
	GdkPixmap * pixmap;
#endif
} GtkDemoWindow;

typedef struct _LockerDemo
{
	LockerDemoHelper * helper;
	GdkPixbuf * images[GDI_COUNT];
	GtkDemoWindow * windows;
	size_t windows_cnt;
	guint source;
	guint frame_num;
	int cycle;
	int scroll;
} GtkDemo;


/* constants */
static const char * _gtkdemo_images[GDI_COUNT] =
{
	DEMODIR "/background.jpg",
	DEMODIR "/apple-red.png",
	DEMODIR "/gnome-applets.png",
	DEMODIR "/gnome-calendar.png",
	DEMODIR "/gnome-foot.png",
	DEMODIR "/gnome-gmush.png",
	DEMODIR "/gnome-gimp.png",
	DEMODIR "/gnome-gsame.png",
	DEMODIR "/gnu-keys.png"
};


/* prototypes */
/* plug-in */
static GtkDemo * _gtkdemo_init(LockerDemoHelper * helper);
static void _gtkdemo_destroy(GtkDemo * gtkdemo);
static int _gtkdemo_add(GtkDemo * gtkdemo, GdkWindow * window);
static void _gtkdemo_remove(GtkDemo * gtkdemo, GdkWindow * window);
static void _gtkdemo_start(GtkDemo * gtkdemo);
static void _gtkdemo_stop(GtkDemo * gtkdemo);
static void _gtkdemo_cycle(GtkDemo * gtkdemo);

/* callbacks */
static gboolean _gtkdemo_on_idle(gpointer data);
static gboolean _gtkdemo_on_timeout(gpointer data);


/* public */
/* variables */
/* plug-in */
LockerDemoDefinition plugin =
{
	"Gtk-demo",
	NULL,
	NULL,
	_gtkdemo_init,
	_gtkdemo_destroy,
	NULL,
	_gtkdemo_add,
	_gtkdemo_remove,
	_gtkdemo_start,
	_gtkdemo_stop,
	_gtkdemo_cycle
};


/* private */
/* functions */
/* plug-in */
/* gtkdemo_init */
static GdkPixbuf * _init_image(GtkDemo * gtkdemo, size_t i);

static GtkDemo * _gtkdemo_init(LockerDemoHelper * helper)
{
	GtkDemo * gtkdemo;
	size_t i;

	if((gtkdemo = object_new(sizeof(*gtkdemo))) == NULL)
		return NULL;
	gtkdemo->helper = helper;
	for(i = 0; i < GDI_COUNT; i++)
		gtkdemo->images[i] = _init_image(gtkdemo, i);
	gtkdemo->windows = NULL;
	gtkdemo->windows_cnt = 0;
	gtkdemo->source = 0;
	gtkdemo->frame_num = 0;
	gtkdemo->cycle = 1;
	gtkdemo->scroll = 0;
	return gtkdemo;
}

static GdkPixbuf * _init_image(GtkDemo * gtkdemo, size_t i)
{
	const unsigned int flags = GTK_ICON_LOOKUP_GENERIC_FALLBACK;
	GdkPixbuf * pixbuf;
	GtkIconTheme * icontheme;
	GError * error = NULL;

	if((pixbuf = gdk_pixbuf_new_from_file(_gtkdemo_images[i], &error))
			!= NULL)
		return pixbuf;
	gtkdemo->helper->error(NULL, error->message, 1);
	g_error_free(error);
	if(i == GDI_BACKGROUND)
		/* no background */
		return NULL;
	error = NULL;
	icontheme = gtk_icon_theme_get_default();
	if((pixbuf = gtk_icon_theme_load_icon(icontheme, "gtk-missing-image",
					48, flags, &error)) != NULL)
		return pixbuf;
	gtkdemo->helper->error(NULL, error->message, 1);
	g_error_free(error);
	return NULL;
}


/* gtkdemo_destroy */
static void _gtkdemo_destroy(GtkDemo * gtkdemo)
{
	size_t i;

	_gtkdemo_stop(gtkdemo);
	for(i = 0; i < GDI_COUNT; i++)
		if(gtkdemo->images[i] != NULL)
			g_object_unref(gtkdemo->images[i]);
	object_delete(gtkdemo);
}


/* gtkdemo_add */
static int _gtkdemo_add(GtkDemo * gtkdemo, GdkWindow * window)
{
	int ret = 0;
	GtkDemoWindow * p;
#if GTK_CHECK_VERSION(3, 0, 0)
	GdkRGBA color = { 0.0, 0.0, 0.0, 0.0 };
#else
	GdkColor color = { 0x0, 0x0, 0x0, 0x0 };
	GdkPixmap * pixmap;
#endif
	GdkPixbuf * background = gtkdemo->images[GDI_BACKGROUND];
	GdkRectangle rect;
	int width;
	int height;
#if !GTK_CHECK_VERSION(3, 0, 0)
	int depth;
#endif

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() window=%p\n", __func__, (void *)window);
#endif
	if((p = realloc(gtkdemo->windows, sizeof(*p) * (gtkdemo->windows_cnt
						+ 1))) == NULL)
		return -1;
	gtkdemo->windows = p;
	p = &gtkdemo->windows[gtkdemo->windows_cnt++];
	gdk_window_get_geometry(window, &rect.x, &rect.y, &rect.width,
			&rect.height
#if !GTK_CHECK_VERSION(3, 0, 0)
			, &depth
#endif
			);
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() (%dx%d), (%dx%d)@%dbpp\n", __func__,
			rect.x, rect.y, rect.width, rect.height,
# if GTK_CHECK_VERSION(3, 0, 0)
			0);
# else
			depth);
# endif
#endif
	/* set the default color */
#if GTK_CHECK_VERSION(3, 0, 0)
	gdk_window_set_background_rgba(window, &color);
	p->cairo = gdk_cairo_create(window);
#else
	gdk_window_set_background(window, &color);
#endif
	/* draw background */
	width = (background != NULL) ? gdk_pixbuf_get_width(background) : 0;
	height = (background != NULL) ? gdk_pixbuf_get_height(background) : 0;
	p->frame = gdk_pixbuf_new(GDK_COLORSPACE_RGB, 1, 8, rect.width,
			rect.height);
	if(width > 0 && height > 0)
#if GTK_CHECK_VERSION(3, 0, 0)
	{
		gdk_pixbuf_copy_area(background, 0, 0, width, height, p->frame,
				0, 0);
		gdk_cairo_set_source_pixbuf(p->cairo, p->frame, 0, 0);
		cairo_paint(p->cairo);
	}
#else
	{
		pixmap = gdk_pixmap_new(window, width, height, -1);
		gdk_draw_pixbuf(pixmap, NULL, background, 0, 0, 0, 0, width,
				height, GDK_RGB_DITHER_NONE, 0, 0);
		gdk_window_set_back_pixmap(window, pixmap, FALSE);
		gdk_pixmap_unref(pixmap);
		gdk_window_clear(window);
	}
	p->pixmap = gdk_pixmap_new(window, rect.width, rect.width, -1);
#endif
	p->window = window;
	return ret;
}


/* gtkdemo_remove */
static void _gtkdemo_remove(GtkDemo * gtkdemo, GdkWindow * window)
{
	size_t i;

	for(i = 0; i < gtkdemo->windows_cnt; i++)
		if(gtkdemo->windows[i].window == window)
		{
			gtkdemo->windows[i].window = NULL;
			g_object_unref(gtkdemo->windows[i].frame);
			gtkdemo->windows[i].frame = NULL;
#if GTK_CHECK_VERSION(3, 0, 0)
			cairo_destroy(gtkdemo->windows[i].cairo);
#else
			gdk_pixmap_unref(gtkdemo->windows[i].pixmap);
			gtkdemo->windows[i].pixmap = NULL;
#endif
		}
	/* FIXME reorganize the array and free memory */
	for(i = 0; i < gtkdemo->windows_cnt; i++)
		if(gtkdemo->windows[i].window != NULL)
			break;
	if(i == gtkdemo->windows_cnt)
	{
		/* there are no windows left */
		_gtkdemo_stop(gtkdemo);
		free(gtkdemo->windows);
		gtkdemo->windows = NULL;
		gtkdemo->windows_cnt = 0;
	}
}


/* gtkdemo_start */
static void _gtkdemo_start(GtkDemo * gtkdemo)
{
	LockerDemoHelper * helper = gtkdemo->helper;
	char const * p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	gtkdemo->scroll = 0;
	if((p = helper->config_get(helper->locker, "gtk-demo", "scroll"))
			!= NULL && strtol(p, NULL, 10) == 1)
		gtkdemo->scroll = 1;
	if(gtkdemo->source == 0)
		_gtkdemo_on_timeout(gtkdemo);
}


/* gtkdemo_stop */
static void _gtkdemo_stop(GtkDemo * gtkdemo)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(gtkdemo->source != 0)
		g_source_remove(gtkdemo->source);
	gtkdemo->source = 0;
}


/* gtkdemo_cycle */
static void _gtkdemo_cycle(GtkDemo * gtkdemo)
{
	gtkdemo->cycle = -gtkdemo->cycle;
}


/* callbacks */
/* gtkdemo_on_idle */
static gboolean _gtkdemo_on_idle(gpointer data)
{
	GtkDemo * gtkdemo = data;

	gtkdemo->source = g_timeout_add(40, _gtkdemo_on_timeout, gtkdemo);
	return FALSE;
}


/* gtkdemo_on_timeout */
static void _timeout_window(GtkDemo * gtkdemo, GtkDemoWindow * window);
static void _timeout_window_image(GtkDemo * gtkdemo, GtkDemoWindow * window,
		double f, gint back_width, gint back_height, double xmid,
		double ymid, double fsin2pi, double fcos2pi, double radius,
		size_t i);

static gboolean _gtkdemo_on_timeout(gpointer data)
{
	GtkDemo * gtkdemo = data;
	size_t i;

	for(i = 0; i < gtkdemo->windows_cnt; i++)
		_timeout_window(gtkdemo, &gtkdemo->windows[i]);
	gtkdemo->frame_num++;
	gtkdemo->source = g_idle_add(_gtkdemo_on_idle, gtkdemo);
	return FALSE;
}

static void _timeout_window(GtkDemo * gtkdemo, GtkDemoWindow * window)
{
	GdkWindow * w;
	GdkPixbuf * background = gtkdemo->images[GDI_BACKGROUND];
	gint back_width = 0;
	gint back_height = 0;
	guint offset_x = 0;
	guint offset_y = 0;
	GdkRectangle rect;
	int src_x;
	int src_y;
	int width;
	int height;
#if !GTK_CHECK_VERSION(3, 0, 0)
	int depth;
#endif
	int j;
#define CYCLE_LEN 60
	double f;
	double fsin2pi;
	double fcos2pi;
	int i;
	double xmid, ymid;
	double radius;

	if(window->window == NULL)
		return;
	w = window->window;
	gdk_window_get_geometry(w, &rect.x, &rect.y, &rect.width, &rect.height
#if !GTK_CHECK_VERSION(3, 0, 0)
			, &depth
#endif
			);
	/* reallocate the frame and background if necessary */
	if(gdk_pixbuf_get_width(window->frame) != rect.width
			|| gdk_pixbuf_get_height(window->frame) != rect.height)
	{
		g_object_unref(window->frame);
		window->frame = gdk_pixbuf_new(GDK_COLORSPACE_RGB, 1, 8,
				rect.width, rect.height);
#if GTK_CHECK_VERSION(3, 0, 0)
		cairo_destroy(window->cairo);
		window->cairo = gdk_cairo_create(w);
#else
		gdk_pixmap_unref(window->pixmap);
		window->pixmap = gdk_pixmap_new(w, rect.width, rect.width, -1);
#endif
	}
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() frame=%p\n", __func__,
			(void *)window->frame);
#endif

	if(background != NULL)
	{
		back_width = gdk_pixbuf_get_width(background);
		back_height = gdk_pixbuf_get_height(background);
		if(gtkdemo->scroll && back_width > 0 && back_height > 0)
		{
			offset_x = gtkdemo->frame_num % back_width;
			offset_y = gtkdemo->frame_num % back_height;
		}
	}
	else
		gdk_pixbuf_fill(window->frame, 0x000000ff);
	src_y = offset_y;
	for(j = 0; back_height > 0 && j < rect.height; j += height)
	{
		height = MIN(back_height - src_y, rect.height - j);
		src_x = offset_x;
		for(i = 0; back_width > 0 && i < rect.width; i += width)
		{
			width = MIN(back_width - src_x, rect.width - i);
			gdk_pixbuf_copy_area(background, src_x, src_y,
					width, height, window->frame, i, j);
			src_x = 0;
		}
		src_y = 0;
	}

	f = (double) (gtkdemo->frame_num % CYCLE_LEN) / CYCLE_LEN;
	fsin2pi = sin(f * 2.0 * G_PI);
	fcos2pi = cos(f * 2.0 * G_PI);

	back_width = rect.width;
	back_height = rect.height;
	xmid = back_width / 2.0;
	ymid = back_height / 2.0;

	radius = MIN(xmid, ymid) / 2.0;

	for(i = 1; i < GDI_COUNT; i++)
		_timeout_window_image(gtkdemo, window, f, back_width,
				back_height, xmid, ymid, fsin2pi, fcos2pi,
				radius, i);
#if GTK_CHECK_VERSION(3, 0, 0)
	gdk_cairo_set_source_pixbuf(window->cairo, window->frame, 0, 0);
	cairo_paint(window->cairo);
#else
	gdk_draw_pixbuf(window->pixmap, NULL, window->frame, 0, 0, 0, 0,
			rect.width, rect.height, GDK_RGB_DITHER_NONE, 0, 0);
	gdk_window_set_back_pixmap(w, window->pixmap, FALSE);
	gdk_window_clear(w);
#endif
}

static void _timeout_window_image(GtkDemo * gtkdemo, GtkDemoWindow * window,
		double f, gint back_width, gint back_height, double xmid,
		double ymid, double fsin2pi, double fcos2pi, double radius,
		size_t i)
{
	double ang;
	int xpos, ypos;
	int iw, ih;
	double r;
	GdkRectangle r1, r2, dest;
	double k;

	if(gtkdemo->images[i] == NULL)
		return;

	ang = 2.0 * G_PI * (double) (i - 1) / (GDI_COUNT - 1) - f * 2.0 * G_PI;
	ang = gtkdemo->cycle * ang;

	iw = gdk_pixbuf_get_width(gtkdemo->images[i]);
	ih = gdk_pixbuf_get_height(gtkdemo->images[i]);

	r = radius + (radius / 3.0) * fsin2pi;

	xpos = floor (xmid + r * cos (ang) - iw / 2.0 + 0.5);
	ypos = floor (ymid + r * sin (ang) - ih / 2.0 + 0.5);

	k = (i & 1) ? fsin2pi : fcos2pi;
	k = 2.0 * k * k;
	k = MAX (0.25, k);

	r1.x = xpos;
	r1.y = ypos;
	r1.width = iw * k;
	r1.height = ih * k;

	r2.x = 0;
	r2.y = 0;
	r2.width = back_width;
	r2.height = back_height;

	if(gdk_rectangle_intersect(&r1, &r2, &dest))
		gdk_pixbuf_composite(gtkdemo->images[i], window->frame,
				dest.x, dest.y, dest.width, dest.height,
				xpos, ypos, k, k, GDK_INTERP_NEAREST, ((i & 1)
					? MAX(127, fabs(255 * fsin2pi))
					: MAX(127, fabs(255 * fcos2pi))));
}
