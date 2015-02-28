/* $Id$ */
/* Copyright (c) 2012-2014 Pierre Pronchery <khorben@defora.org> */
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
} Colors;


/* prototypes */
/* plug-in */
static Colors * _colors_init(LockerDemoHelper * helper);
static void _colors_destroy(Colors * colors);
static int _colors_add(Colors * colors, GdkWindow * window);
static void _colors_remove(Colors * colors, GdkWindow * window);
static void _colors_start(Colors * colors);
static void _colors_stop(Colors * colors);
static void _colors_cycle(Colors * colors);


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
	_colors_stop,
	_colors_cycle
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
	_colors_cycle(colors);
}


/* color_stop */
static void _colors_stop(Colors * colors)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
}


/* colors_cycle */
static void _colors_cycle(Colors * colors)
{
#if GTK_CHECK_VERSION(3, 4, 0)
	GdkRGBA color = { 0.0, 0.0, 0.0, 0.0 };
#else
	GdkColormap * colormap;
	GdkColor color = { 0x0, 0x0, 0x0, 0x0 };
#endif
	size_t i;

	color.red = rand();
	color.green = rand();
	color.blue = rand();
#if GTK_CHECK_VERSION(3, 4, 0)
	color.red /= RAND_MAX;
	color.green /= RAND_MAX;
	color.blue /= RAND_MAX;
#endif
	for(i = 0; i < colors->windows_cnt; i++)
	{
#if GTK_CHECK_VERSION(3, 4, 0)
		/* FIXME untested */
		gdk_window_set_background_rgba(colors->windows[i], &color);
#else
		if((colormap = gdk_drawable_get_colormap(colors->windows[i]))
				== NULL)
			colormap = gdk_screen_get_default_colormap(
					gdk_screen_get_default());
		gdk_colormap_alloc_color(colormap, &color, FALSE, TRUE);
		gdk_window_set_background(colors->windows[i], &color);
		gdk_window_clear(colors->windows[i]);
		gdk_colormap_free_colors(colormap, &color, 1);
#endif
	}
}
