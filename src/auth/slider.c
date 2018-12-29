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



#include <System.h>
#ifdef __NetBSD__
# include <sys/param.h>
# include <sys/sysctl.h>
# include <errno.h>
#else
# include <fcntl.h>
# include <unistd.h>
#endif
#include <string.h>
#include "Locker.h"


/* Slider */
/* private */
/* types */
typedef struct _LockerAuth
{
	LockerAuthHelper * helper;
	guint source;
	gboolean locked;

	/* widgets */
	GtkWidget * widget;
	GtkWidget * icon1;
	GtkWidget * scale;
	GtkWidget * icon2;
} Slider;

typedef struct _SliderTheme
{
	char const * name;
	char const * icon1;
	char const * icon2;
} SliderTheme;


/* constants */
static const SliderTheme _slider_themes[] =
{
	{ "changes", "changes-prevent", "changes-allow" },
	{ "dialog", "dialog-error", "dialog-information" },
	{ "emblem", "emblem-important", "emblem-default" },
	{ "filesystem", "emblem-readonly", "emblem-shared" },
	{ "security", "security-high", "security-low" },
	{ "smiley", "face-sad", "face-smile" },
	{ "user", "user-idle", "user-available" },
	{ NULL, NULL, NULL }
};
#define SLIDER_THEME_DEFAULT 0


/* prototypes */
/* plug-in */
static Slider * _slider_init(LockerAuthHelper * helper);
static void _slider_destroy(Slider * slider);
static GtkWidget * _slider_get_widget(Slider * slider);
static int _slider_action(Slider * slider, LockerAction action);

/* useful */
static int _slider_load(Slider * slider);

/* callbacks */
static void _slider_on_scale_value_changed(gpointer data);
static gboolean _slider_on_scale_value_changed_timeout(gpointer data);
static gboolean _slider_on_timeout(gpointer data);


/* public */
/* variables */
/* plug-in */
LockerAuthDefinition plugin =
{
	"Slider",
	NULL,
	NULL,
	_slider_init,
	_slider_destroy,
	_slider_get_widget,
	_slider_action
};


/* private */
/* functions */
/* slider_init */
static Slider * _slider_init(LockerAuthHelper * helper)
{
	Slider * slider;
	GtkWidget * vbox;
	GtkWidget * hbox;

	if((slider = object_new(sizeof(*slider))) == NULL)
		return NULL;
	slider->helper = helper;
	slider->source = 0;
	slider->locked = FALSE;
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	vbox = gtk_vbox_new(FALSE, 4);
#endif
	slider->widget = vbox;
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	slider->icon1 = NULL;
	slider->scale = NULL;
	slider->icon2 = NULL;
	_slider_load(slider);
	/* left image */
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(slider->icon1, "halign", GTK_ALIGN_END,
			"margin-bottom", 96, "margin-top", 96, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(slider->icon1), 1.0, 0.5);
	gtk_misc_set_padding(GTK_MISC(slider->icon1), 0, 96);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), slider->icon1, TRUE, TRUE, 0);
	/* scale */
#if GTK_CHECK_VERSION(3, 0, 0)
	slider->scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,
			0.0, 100.0, 1.0);
#else
	slider->scale = gtk_hscale_new_with_range(0.0, 100.0, 1.0);
#endif
	gtk_range_set_value(GTK_RANGE(slider->scale), 0.0);
	gtk_scale_set_draw_value(GTK_SCALE(slider->scale), FALSE);
	gtk_widget_set_size_request(slider->scale, 240, -1);
	g_signal_connect_swapped(slider->scale, "value-changed", G_CALLBACK(
				_slider_on_scale_value_changed), slider);
	gtk_box_pack_start(GTK_BOX(hbox), slider->scale, FALSE, TRUE, 0);
	/* right image */
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(slider->icon2, "halign", GTK_ALIGN_START,
			"margin-bottom", 96, "margin-top", 96, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(slider->icon2), 0.0, 0.5);
	gtk_misc_set_padding(GTK_MISC(slider->icon2), 0, 96);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), slider->icon2, TRUE, TRUE, 0);
	gtk_widget_show_all(hbox);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	return slider;
}


/* slider_destroy */
static void _slider_destroy(Slider * slider)
{
	gtk_widget_destroy(slider->widget);
	if(slider->source != 0)
		g_source_remove(slider->source);
	object_delete(slider);
}


/* slider_get_widget */
static GtkWidget * _slider_get_widget(Slider * slider)
{
	return slider->widget;
}


/* slider_action */
static int _slider_action(Slider * slider, LockerAction action)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__, action);
#endif
	switch(action)
	{
		case LOCKER_ACTION_DEACTIVATE:
			if(slider->source != 0)
				g_source_remove(slider->source);
			slider->source = 0;
			if(slider->locked)
			{
				gtk_range_set_value(GTK_RANGE(slider->scale),
						0.0);
				gtk_widget_grab_focus(slider->scale);
				gtk_widget_show(slider->widget);
				slider->source = g_timeout_add(3000,
						_slider_on_timeout, slider);
			}
			break;
		case LOCKER_ACTION_LOCK:
			gtk_widget_hide(slider->widget);
			if(slider->source != 0)
				g_source_remove(slider->source);
			slider->source = 0;
			slider->locked = TRUE;
			break;
		case LOCKER_ACTION_UNLOCK:
			slider->locked = FALSE;
			/* fallback */
		case LOCKER_ACTION_ACTIVATE:
		case LOCKER_ACTION_CYCLE:
		case LOCKER_ACTION_START:
			gtk_widget_hide(slider->widget);
			if(slider->source != 0)
				g_source_remove(slider->source);
			slider->source = 0;
			break;
		default:
			break;
	}
	return 0;
}


/* useful */
/* slider_load */
static int _slider_load(Slider * slider)
{
	LockerAuthHelper * helper = slider->helper;
	size_t i = SLIDER_THEME_DEFAULT;
	String const * p;

	/* load the theme configured */
	if((p = helper->config_get(helper->locker, "slider", "theme")) != NULL)
		for(i = 0; _slider_themes[i].name != NULL; i++)
			if(strcmp(_slider_themes[i].name, p) == 0)
				break;
	/* load the first icon */
	if((p = _slider_themes[i].icon1) == NULL
			&& (p = helper->config_get(helper->locker, "slider",
					"icon1")) == NULL)
		p = _slider_themes[SLIDER_THEME_DEFAULT].icon1;
	if(slider->icon1 == NULL)
		slider->icon1 = gtk_image_new_from_icon_name(p,
				GTK_ICON_SIZE_LARGE_TOOLBAR);
	else
		gtk_image_set_from_icon_name(GTK_IMAGE(slider->icon1), p,
				GTK_ICON_SIZE_LARGE_TOOLBAR);
	/* load the second icon */
	if((p = _slider_themes[i].icon2) == NULL
			&& (p = helper->config_get(helper->locker, "slider",
					"icon2")) == NULL)
		p = _slider_themes[SLIDER_THEME_DEFAULT].icon2;
	if(slider->icon2 == NULL)
		slider->icon2 = gtk_image_new_from_icon_name(p,
				GTK_ICON_SIZE_LARGE_TOOLBAR);
	else
		gtk_image_set_from_icon_name(GTK_IMAGE(slider->icon2), p,
				GTK_ICON_SIZE_LARGE_TOOLBAR);
	return 0;
}


/* callbacks */
/* slider_on_scale_value_changed */
static void _slider_on_scale_value_changed(gpointer data)
{
	Slider * slider = data;
	LockerAuthHelper * helper = slider->helper;
	gdouble value;

	if(slider->source != 0)
		g_source_remove(slider->source);
	slider->source = 0;
	value = gtk_range_get_value(GTK_RANGE(slider->scale));
	if(value >= 100.0)
		helper->action(helper->locker, LOCKER_ACTION_UNLOCK);
	else if(value > 0.0)
		slider->source = g_timeout_add(1000,
				_slider_on_scale_value_changed_timeout, slider);
}


/* slider_on_scale_value_changed_timeout */
static gboolean _slider_on_scale_value_changed_timeout(gpointer data)
{
	Slider * slider = data;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	gtk_range_set_value(GTK_RANGE(slider->scale), 0.0);
	slider->source = g_timeout_add(3000, _slider_on_timeout, slider);
	return FALSE;
}


/* slider_on_timeout */
static gboolean _slider_on_timeout(gpointer data)
{
	Slider * slider = data;
	LockerAuthHelper * helper = slider->helper;

	slider->source = 0;
	helper->action(helper->locker, LOCKER_ACTION_ACTIVATE);
	return FALSE;
}
