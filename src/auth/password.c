/* $Id$ */
/* Copyright (c) 2011-2022 Pierre Pronchery <khorben@defora.org> */
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
#include <pwd.h>
#ifdef __linux__
# include <crypt.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <libintl.h>
#include <System.h>
#include <Desktop.h>
#include "Locker.h"
#include "../../config.h"
#define _(string) gettext(string)


/* Password */
/* private */
/* types */
typedef struct _LockerAuth
{
	LockerAuthHelper * helper;

	guint source;

	/* widgets */
	GtkWidget * widget;
	GtkWidget * password;
	GtkWidget * button;
	GtkWidget * error;
} Password;


/* prototypes */
/* plug-in */
static Password * _password_init(LockerAuthHelper * helper);
static void _password_destroy(Password * password);
static GtkWidget * _password_get_widget(Password * password);
static int _password_action(Password * password, LockerAction action);

/* callbacks */
static void _password_on_password_activate(gpointer data);
static gboolean _password_on_timeout(gpointer data);
static gboolean _password_on_timeout_clear(gpointer data);


/* public */
/* variables */
/* plug-in */
LockerAuthDefinition plugin =
{
	"Password",
	NULL,
	NULL,
	_password_init,
	_password_destroy,
	_password_get_widget,
	_password_action,
};


/* private */
/* functions */
/* password_init */
static Password * _password_init(LockerAuthHelper * helper)
{
	Password * password;
	PangoFontDescription * bold;
	const GdkRGBA white = { 1.0, 1.0, 1.0, 1.0 };
	const GdkRGBA red = { 1.0, 0.0, 0.0, 1.0 };
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * hbox2;
	GtkWidget * widget;
	char buf[256];
	struct passwd * pw;
	char const * username;

	if((password = object_new(sizeof(*password))) == NULL)
		return NULL;
	password->helper = helper;
	password->source = 0;
	bold = pango_font_description_new();
	pango_font_description_set_weight(bold, PANGO_WEIGHT_BOLD);
	password->widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	/* top padding (centering) */
	widget = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(password->widget), widget, TRUE, TRUE, 0);
	/* dialog */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	/* left padding (centering) */
	widget = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	/* authentication icon */
	widget = gtk_image_new_from_stock(GTK_STOCK_DIALOG_AUTHENTICATION,
			GTK_ICON_SIZE_DIALOG);
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "valign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.5, 0.0);
#endif
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	/* hostname */
	if(gethostname(buf, sizeof(buf)) != 0)
		snprintf(buf, sizeof(buf), "%s", "DeforaOS " PACKAGE);
	else
		buf[sizeof(buf) - 1] = '\0';
	widget = gtk_label_new(buf);
	gtk_widget_override_color(widget, GTK_STATE_FLAG_NORMAL, &white);
	gtk_widget_override_font(widget, bold);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* screen */
	if((pw = getpwuid(getuid())) != NULL)
		username = pw->pw_name;
	else
		username = getenv("USER");
	snprintf(buf, sizeof(buf), (username != NULL)
			? _("This screen is locked by %s")
			: _("This screen is locked"), username);
	widget = gtk_label_new(buf);
	gtk_widget_override_color(widget, GTK_STATE_FLAG_NORMAL, &white);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* prompt */
	widget = gtk_label_new(_("Enter password: "));
	gtk_widget_override_color(widget, GTK_STATE_FLAG_NORMAL, &white);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	/* entry */
	hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	password->password = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(password->password), FALSE);
	g_signal_connect_swapped(password->password, "activate", G_CALLBACK(
				_password_on_password_activate), password);
	gtk_box_pack_start(GTK_BOX(hbox2), password->password, FALSE, TRUE, 0);
	/* button */
	password->button = gtk_button_new_from_stock(GTK_STOCK_OK);
	g_signal_connect_swapped(password->button, "clicked", G_CALLBACK(
				_password_on_password_activate), password);
	gtk_box_pack_start(GTK_BOX(hbox2), password->button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, TRUE, 0);
	/* error */
	password->error = gtk_label_new("");
	gtk_widget_override_color(password->error, GTK_STATE_FLAG_NORMAL, &red);
	gtk_widget_override_font(password->error, bold);
	gtk_box_pack_start(GTK_BOX(vbox), password->error, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 0);
	/* right padding (centering) */
	widget = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(password->widget), hbox, FALSE, TRUE, 0);
	/* bottom padding (centering) */
	widget = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(password->widget), widget, TRUE, TRUE, 0);
	gtk_widget_show_all(password->widget);
	gtk_widget_hide(password->widget);
	pango_font_description_free(bold);
	return password;
}


/* password_destroy */
static void _password_destroy(Password * password)
{
	gtk_widget_destroy(password->widget);
	if(password->source != 0)
		g_source_remove(password->source);
	object_delete(password);
}


/* password_get_widget */
static GtkWidget * _password_get_widget(Password * password)
{
	return password->widget;
}


/* password_action */
static int _password_action(Password * password, LockerAction action)
{
	LockerAuthHelper * helper = password->helper;
	GtkWidget * entry = password->password;
	char const * p;

	switch(action)
	{
		case LOCKER_ACTION_DEACTIVATE:
			gtk_widget_grab_focus(entry);
			gtk_widget_show(password->widget);
			break;
		case LOCKER_ACTION_LOCK:
			gtk_widget_hide(password->widget);
			if((p = helper->config_get(helper->locker, "password",
							"password")) == NULL)
			{
				gtk_entry_set_text(GTK_ENTRY(entry), "");
				return -helper->error(helper->locker,
						_("No password was set"), 1);
			}
			gtk_widget_set_sensitive(entry, TRUE);
			gtk_widget_set_sensitive(password->button, TRUE);
			gtk_entry_set_text(GTK_ENTRY(entry), "");
			if(password->source != 0)
				g_source_remove(password->source);
			password->source = g_timeout_add(30000,
					_password_on_timeout, password);
			break;
		case LOCKER_ACTION_ACTIVATE:
		case LOCKER_ACTION_CYCLE:
		case LOCKER_ACTION_START:
			gtk_widget_hide(password->widget);
			break;
		case LOCKER_ACTION_UNLOCK:
			gtk_widget_hide(password->widget);
			if(password->source != 0)
				g_source_remove(password->source);
			password->source = 0;
			break;
		default:
			break;
	}
	return 0;
}


/* callbacks */
/* password_on_password_activate */
static void _password_on_password_activate(gpointer data)
{
	Password * password = data;
	LockerAuthHelper * helper = password->helper;
	char const * text;
	char const * p;
	char const * q;

	if(password->source != 0)
		g_source_remove(password->source);
	password->source = 0;
	gtk_widget_set_sensitive(password->password, FALSE);
	gtk_widget_set_sensitive(password->button, FALSE);
	text = gtk_entry_get_text(GTK_ENTRY(password->password));
	if((p = helper->config_get(helper->locker, "password", "password"))
			== NULL)
		helper->error(NULL, _("No password was set"), 1);
	else
	{
		/* check if the password is hashed */
		if(p[0] == '$' && (q = crypt(text, p)) != NULL)
			text = q;
		if(strcmp(text, p) == 0)
		{
			gtk_entry_set_text(GTK_ENTRY(password->password), "");
			helper->action(helper->locker, LOCKER_ACTION_UNLOCK);
			return;
		}
	}
	gtk_entry_set_text(GTK_ENTRY(password->password), "");
	helper->error(NULL, _("Authentication failed"), 1);
	gtk_widget_grab_focus(password->password);
	gtk_label_set_text(GTK_LABEL(password->error), _("Wrong password!"));
	password->source = g_timeout_add(3000, _password_on_timeout_clear,
			password);
}


/* password_on_timeout */
static gboolean _password_on_timeout(gpointer data)
{
	Password * password = data;

	gtk_label_set_text(GTK_LABEL(password->error), _("Timed out"));
	gtk_widget_set_sensitive(password->password, FALSE);
	gtk_widget_set_sensitive(password->button, FALSE);
	password->source = g_timeout_add(3000, _password_on_timeout_clear,
			password);
	return FALSE;
}


/* password_on_timeout_clear */
static gboolean _password_on_timeout_clear(gpointer data)
{
	Password * password = data;

	password->source = 0;
	gtk_label_set_text(GTK_LABEL(password->error), "");
	gtk_widget_set_sensitive(password->password, TRUE);
	gtk_widget_set_sensitive(password->button, TRUE);
	gtk_entry_set_text(GTK_ENTRY(password->password), "");
	gtk_widget_hide(password->widget);
	password->helper->action(password->helper->locker, LOCKER_ACTION_START);
	return FALSE;
}
