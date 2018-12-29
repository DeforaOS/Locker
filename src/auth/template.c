/* $Id$ */
/* Copyright (c) 2012-2015 Pierre Pronchery <khorben@defora.org> */
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
#include "Locker.h"


/* Template */
/* private */
/* types */
typedef struct _LockerAuth
{
	LockerAuthHelper * helper;

	/* widgets */
	GtkWidget * widget;
} Template;


/* prototypes */
/* plug-in */
static Template * _template_init(LockerAuthHelper * helper);
static void _template_destroy(Template * template);
static GtkWidget * _template_get_widget(Template * template);
static int _template_action(Template * template, LockerAction action);


/* public */
/* variables */
/* plug-in */
LockerAuthDefinition plugin =
{
	"Template",
	"missing-image",
	NULL,
	_template_init,
	_template_destroy,
	_template_get_widget,
	_template_action
};


/* private */
/* functions */
/* template_init */
static Template * _template_init(LockerAuthHelper * helper)
{
	Template * template;

	if((template = object_new(sizeof(*template))) == NULL)
		return NULL;
	template->helper = helper;
	template->widget = gtk_button_new_from_stock(GTK_STOCK_OK);
	gtk_widget_show(template->widget);
	return template;
}


/* template_destroy */
static void _template_destroy(Template * template)
{
	gtk_widget_destroy(template->widget);
	object_delete(template);
}


/* template_get_widget */
static GtkWidget * _template_get_widget(Template * template)
{
	return template->widget;
}


/* template_action */
static int _template_action(Template * template, LockerAction action)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__, action);
#endif
	switch(action)
	{
		case LOCKER_ACTION_DEACTIVATE:
			gtk_widget_show(template->widget);
			break;
		case LOCKER_ACTION_LOCK:
			gtk_widget_hide(template->widget);
			break;
		case LOCKER_ACTION_START:
		case LOCKER_ACTION_UNLOCK:
			gtk_widget_hide(template->widget);
			break;
		default:
			break;
	}
	return 0;
}
