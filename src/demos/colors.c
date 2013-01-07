/* $Id$ */
/* Copyright (c) 2012 Pierre Pronchery <khorben@defora.org> */
/* This file is part of DeforaOS Desktop Locker */
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. */



#include <sys/types.h>
#include <stdlib.h>
#ifdef DEBUG
# include <stdio.h>
#endif
#include <string.h>
#include <errno.h>
#include <System.h>
#include "Locker/demo.h"


/* Colors */
/* private */
/* types */
typedef struct _LockerDemo
{
	LockerDemoHelper * helper;

	GdkWindow ** windows;
	size_t windows_cnt;

	unsigned int source;
} Colors;


/* prototypes */
/* plug-in */
static Colors * _colors_init(LockerDemoHelper * helper);
static void _colors_destroy(Colors * colors);
static int _colors_add(Colors * colors, GdkWindow * window);
static void _colors_remove(Colors * colors, GdkWindow * window);
static void _colors_start(Colors * colors);
static void _colors_stop(Colors * colors);

/* callbacks */
static gboolean _colors_on_timeout(gpointer data);


/* public */
/* variables */
/* plug-in */
LockerDemoDefinition plugin =
{
	"Colors",
	"stock_select-colors",
	NULL,
	_colors_init,
	_colors_destroy,
	_colors_add,
	_colors_remove,
	_colors_start,
	_colors_stop
};


/* private */
/* functions */
/* plug-in */
/* colors_init */
static Colors * _colors_init(LockerDemoHelper * helper)
{
	Colors * colors;

	if((colors = object_new(sizeof(*colors))) == NULL)
		return NULL;
	colors->helper = helper;
	colors->windows = NULL;
	colors->windows_cnt = 0;
	colors->source = 0;
	return colors;
}


/* color_destroy */
static void _colors_destroy(Colors * colors)
{
	_colors_stop(colors);
	object_delete(colors);
}


/* color_add */
static int _colors_add(Colors * colors, GdkWindow * window)
{
	GdkWindow ** p;

	if((p = realloc(colors->windows, sizeof(*p) * (colors->windows_cnt + 1)))
			== NULL)
		return -error_set_code(1, "%s", strerror(errno));
	colors->windows = p;
	colors->windows[colors->windows_cnt++] = window;
	return 0;
}


/* color_remove */
static void _colors_remove(Colors * colors, GdkWindow * window)
{
	size_t i = 0;
	GdkWindow ** p;

	while(i < colors->windows_cnt)
		if(colors->windows[i] != window)
		{
			i++;
			continue;
		}
		else
			memmove(&colors->windows[i], &colors->windows[i + 1],
					sizeof(*colors->windows)
					* (--colors->windows_cnt - i));
	/* free some memory */
	if((p = realloc(colors->windows, sizeof(*p) * colors->windows_cnt))
			!= NULL)
		colors->windows = p;
}


/* color_start */
static void _colors_start(Colors * colors)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	_colors_on_timeout(colors);
	if(colors->source == 0)
		colors->source = g_timeout_add(500, _colors_on_timeout, colors);
}


/* color_stop */
static void _colors_stop(Colors * colors)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(colors->source != 0)
		g_source_remove(colors->source);
	colors->source = 0;
}


/* callbacks */
/* color_on_timeout */
static gboolean _colors_on_timeout(gpointer data)
{
	Colors * colors = data;
	GdkColormap * colormap;
	GdkColor color;
	size_t i;

	color.pixel = 0;
	color.red = rand();
	color.green = rand();
	color.blue = rand();
	for(i = 0; i < colors->windows_cnt; i++)
	{
		if((colormap = gdk_drawable_get_colormap(colors->windows[i]))
				== NULL)
			colormap = gdk_screen_get_default_colormap(
					gdk_screen_get_default());
		gdk_colormap_alloc_color(colormap, &color, FALSE, TRUE);
		gdk_window_set_background(colors->windows[i], &color);
		gdk_window_clear(colors->windows[i]);
		gdk_colormap_free_colors(colormap, &color, 1);
	}
	return TRUE;
}