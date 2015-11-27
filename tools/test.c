/* $Id$ */
/* Copyright (c) 2012-2015 Pierre Pronchery <khorben@defora.org> */
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



#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <System.h>
#include "../include/Locker/demo.h"
#include "../src/locker.h"
#include "../config.h"

#ifndef PROGNAME
# define PROGNAME	"locker-test"
#endif
#ifndef PREFIX
# define PREFIX		"/usr/local"
#endif
#ifndef LIBDIR
# define LIBDIR		PREFIX "/lib"
#endif


/* private */
/* types */
struct _Locker
{
	char * name;
	Config * config;

	/* demo */
	LockerDemoDefinition * dplugin;
	LockerDemo * demo;

	/* auth */
	LockerAuthDefinition * aplugin;
	LockerAuth * auth;

	/* widgets */
	GtkWidget * window;
	GtkWidget * variable;
	GtkWidget * value;
};


/* prototypes */
static int _test(int desktop, int root, int width, int height,
		char const * demo, char const * auth);
static int _usage(void);

/* helpers */
static int _test_helper_action(Locker * locker, LockerAction action);
static char const * _test_helper_config_get_auth(Locker * locker,
		char const * section, char const * variable);
static char const * _test_helper_config_get_demo(Locker * locker,
		char const * section, char const * variable);
static int _test_helper_config_set(Locker * locker, char const * section,
		char const * variable, char const * value);
static int _test_helper_error(Locker * locker, char const * message, int ret);

/* callbacks */
static gboolean _test_on_closex(void);
static void _test_on_apply(gpointer data);
static void _test_on_cycle(gpointer data);
static void _test_on_lock(gpointer data);
static void _test_on_start(gpointer data);
static void _test_on_stop(gpointer data);
static void _test_on_unlock(gpointer data);


/* functions */
/* test */
static Config * _test_config(void);

static int _test(int desktop, int root, int width, int height,
		char const * demo, char const * auth)
{
	int ret = 0;
	Locker * locker;
	LockerDemoHelper dhelper;
	Plugin * dplugin;
	LockerAuthHelper ahelper;
	Plugin * aplugin;
#if GTK_CHECK_VERSION(3, 0, 0)
	GdkRGBA black;
#else
	GdkColor black;
#endif
	GtkWidget * window;
	GdkWindow * wwindow;
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * button;
	GtkWidget * widget;
	GdkScreen * screen;

	if((locker = object_new(sizeof(*locker))) == NULL)
		return error_print(PROGNAME);
	if((locker->name = strdup(demo)) == NULL)
	{
		object_delete(locker);
		return error_set_print(PROGNAME, 1, "%s", strerror(errno));
	}
	locker->config = _test_config();
	/* demo plug-in */
	dhelper.locker = locker;
	dhelper.error = _test_helper_error;
	dhelper.config_get = _test_helper_config_get_demo;
	dhelper.config_set = _test_helper_config_set;
	if((dplugin = plugin_new(LIBDIR, PACKAGE, "demos", demo)) == NULL)
	{
		if(locker->config != NULL)
			config_delete(locker->config);
		free(locker->name);
		object_delete(locker);
		return error_set_print(PROGNAME, 1, "%s: %s", demo,
				"Could not load demo plug-in");
	}
	if((locker->dplugin = plugin_lookup(dplugin, "plugin")) == NULL
			|| locker->dplugin->init == NULL
			|| (locker->demo = locker->dplugin->init(&dhelper))
			== NULL)
	{
		plugin_delete(dplugin);
		if(locker->config != NULL)
			config_delete(locker->config);
		free(locker->name);
		object_delete(locker);
		return error_set_print(PROGNAME, 1, "%s: %s", demo,
				"Could not initialize demo plug-in");
	}
	/* auth plug-in */
	ahelper.locker = locker;
	ahelper.error = _test_helper_error;
	ahelper.action = _test_helper_action;
	ahelper.config_get = _test_helper_config_get_auth;
	ahelper.config_set = _test_helper_config_set;
	if(auth == NULL)
	{
		aplugin = NULL;
		locker->aplugin = NULL;
	}
	else if((aplugin = plugin_new(LIBDIR, PACKAGE, "auth", auth)) == NULL)
		error_set_print(PROGNAME, 1, "%s: %s", auth,
				"Could not load auth plug-in");
	else if((locker->aplugin = plugin_lookup(aplugin, "plugin")) == NULL
			|| locker->aplugin->init == NULL
			|| locker->aplugin->destroy == NULL
			|| locker->aplugin->get_widget == NULL
			|| locker->aplugin->action == NULL
			|| (locker->auth = locker->aplugin->init(&ahelper))
			== NULL)
	{
		locker->aplugin = NULL;
		plugin_delete(aplugin);
		aplugin = NULL;
		error_set_print(PROGNAME, 1, "%s: %s", auth,
				"Could not initialize auth plug-in");
	}
	/* widgets */
	/* toolbar */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_container_set_border_width(GTK_CONTAINER(window), 4);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	g_signal_connect(window, "delete-event", G_CALLBACK(_test_on_closex),
			NULL);
#if GTK_CHECK_VERSION(3, 0, 0)
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
	vbox = gtk_vbox_new(FALSE, 4);
#endif
	/* controls */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
#else
	hbox = gtk_hbutton_box_new();
#endif
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_START);
	gtk_box_set_spacing(GTK_BOX(hbox), 4);
	button = gtk_button_new_with_label("Start");
	g_signal_connect_swapped(button, "clicked", G_CALLBACK(_test_on_start),
			locker);
	gtk_container_add(GTK_CONTAINER(hbox), button);
	button = gtk_button_new_with_label("Stop");
	g_signal_connect_swapped(button, "clicked", G_CALLBACK(_test_on_stop),
			locker);
	gtk_container_add(GTK_CONTAINER(hbox), button);
	button = gtk_button_new_with_label("Cycle");
	g_signal_connect_swapped(button, "clicked", G_CALLBACK(_test_on_cycle),
			locker);
	gtk_container_add(GTK_CONTAINER(hbox), button);
	button = gtk_button_new_with_label("Lock");
	g_signal_connect_swapped(button, "clicked", G_CALLBACK(_test_on_lock),
			locker);
	gtk_container_add(GTK_CONTAINER(hbox), button);
	button = gtk_button_new_with_label("Unlock");
	g_signal_connect_swapped(button, "clicked", G_CALLBACK(_test_on_unlock),
			locker);
	gtk_container_add(GTK_CONTAINER(hbox), button);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* configuration */
#if GTK_CHECK_VERSION(3, 0, 0)
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	widget = gtk_label_new("Configuration: ");
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	locker->variable = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), locker->variable, FALSE, TRUE, 0);
	locker->value = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), locker->value, FALSE, TRUE, 0);
	button = gtk_button_new_from_stock(GTK_STOCK_APPLY);
	g_signal_connect_swapped(button, "clicked", G_CALLBACK(_test_on_apply),
			locker);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	gtk_widget_show_all(window);
	/* demo window */
	if(root)
	{
		screen = gdk_screen_get_default();
		wwindow = gdk_screen_get_root_window(screen);
		locker->window = NULL;
	}
	else
	{
		locker->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		memset(&black, 0, sizeof(black));
#if GTK_CHECK_VERSION(3, 0, 0)
		gtk_widget_override_background_color(locker->window,
				GTK_STATE_NORMAL, &black);
#else
		gtk_widget_modify_bg(locker->window, GTK_STATE_NORMAL, &black);
#endif
		gtk_window_set_default_size(GTK_WINDOW(locker->window), width,
				height);
		if(desktop)
			gtk_window_set_type_hint(GTK_WINDOW(locker->window),
					GDK_WINDOW_TYPE_HINT_DESKTOP);
		g_signal_connect(locker->window, "delete-event", G_CALLBACK(
					_test_on_closex), NULL);
		/* load the authentication plug-in (if specified) */
		if(locker->auth != NULL
				&& (widget = locker->aplugin->get_widget(
						locker->auth)) != NULL)
			gtk_container_add(GTK_CONTAINER(locker->window),
					widget);
		gtk_widget_show_all(locker->window);
#if GTK_CHECK_VERSION(2, 14, 0)
		wwindow = gtk_widget_get_window(locker->window);
#else
		wwindow = locker->window->window;
#endif
	}
	if(locker->dplugin->add(locker->demo, wwindow) != 0)
		ret = error_set_print(PROGNAME, 1, "%s: %s", demo,
				"Could not add window");
	else
	{
		locker->dplugin->start(locker->demo);
		gtk_main();
		if(locker->aplugin != NULL && locker->aplugin->destroy != NULL)
			locker->aplugin->destroy(locker->auth);
		if(locker->window != NULL)
			gtk_widget_destroy(locker->window);
		gtk_widget_destroy(window);
	}
	if(aplugin != NULL)
		plugin_delete(aplugin);
	locker->dplugin->destroy(locker->demo);
	plugin_delete(dplugin);
	if(locker->config != NULL)
		config_delete(locker->config);
	free(locker->name);
	object_delete(locker);
	return ret;
}

static Config * _test_config(void)
{
	Config * config;
	char const * homedir;
	String * filename;

	if((config = config_new()) == NULL)
		return NULL;
	if((homedir = getenv("HOME")) == NULL)
		homedir = g_get_home_dir();
	if((filename = string_new_append(homedir, "/", LOCKER_CONFIG_FILE,
					NULL)) == NULL)
	{
		error_print(PROGNAME);
		return config;
	}
	if(config_load(config, filename) != 0)
		/* we can ignore errors */
		error_print(PROGNAME);
	string_delete(filename);
	return config;
}


/* usage */
static int _usage(void)
{
	fputs("Usage: " PROGNAME " [-a authentication][-d][-r][-w width]"
			"[-h height] demo\n"
"  -a	Authentication plug-in to load\n"
"  -d	Display the demo as a desktop window\n"
"  -r	Display the demo on the root window\n"
"  -w	Set the width of the test window\n"
"  -h	Set the height of the test window\n", stderr);
	return 1;
}


/* helpers */
/* test_helper_action */
static int _test_helper_action(Locker * locker, LockerAction action)
{
	GtkWidget * widget;
	guint flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;

	switch(action)
	{
		case LOCKER_ACTION_CYCLE:
			if(locker->dplugin->cycle != NULL)
				locker->dplugin->cycle(locker->demo);
			if(locker->aplugin->action != NULL)
				locker->aplugin->action(locker->auth,
						LOCKER_ACTION_CYCLE);
			break;
		case LOCKER_ACTION_LOCK:
			if(locker->aplugin->action != NULL)
				locker->aplugin->action(locker->auth,
						LOCKER_ACTION_LOCK);
			break;
		case LOCKER_ACTION_START:
			if(locker->dplugin->start != NULL)
				locker->dplugin->start(locker->demo);
			if(locker->aplugin->action != NULL)
				locker->aplugin->action(locker->auth,
						LOCKER_ACTION_START);
			break;
		case LOCKER_ACTION_STOP:
			if(locker->dplugin->stop != NULL)
				locker->dplugin->stop(locker->demo);
			if(locker->aplugin->action != NULL)
				locker->aplugin->action(locker->auth,
						LOCKER_ACTION_DEACTIVATE);
			break;
		case LOCKER_ACTION_UNLOCK:
			if(locker->aplugin->action != NULL)
				locker->aplugin->action(locker->auth,
						LOCKER_ACTION_UNLOCK);
			widget = gtk_message_dialog_new_with_markup(
					GTK_WINDOW(locker->window), flags,
					GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
					"%s", "Unlocked");
			gtk_window_set_title(GTK_WINDOW(widget), "Information");
			gtk_dialog_run(GTK_DIALOG(widget));
			gtk_widget_destroy(widget);
			break;
		default:
			/* FIXME really implement */
			return -1;
	}
	return 0;
}


/* test_helper_config_get_auth */
static char const * _test_helper_config_get_auth(Locker * locker,
		char const * section, char const * variable)
{
	char const * ret;
	String * s = NULL;

	if(locker->config == NULL)
	{
		error_set_code(1, "%s", "Configuration not available");
		return NULL;
	}
	if(section != NULL
			&& (s = string_new_append("auth::", section, NULL))
			== NULL)
		return NULL;
	ret = config_get(locker->config, s, variable);
	string_delete(s);
	return ret;
}


/* test_helper_config_get_demo */
static char const * _test_helper_config_get_demo(Locker * locker,
		char const * section, char const * variable)
{
	char const * ret;
	String * s = NULL;

	if(locker->config == NULL)
	{
		error_set_code(1, "%s", "Configuration not available");
		return NULL;
	}
	if(section != NULL
			&& (s = string_new_append("demo::", section, NULL))
			== NULL)
		return NULL;
	ret = config_get(locker->config, s, variable);
	string_delete(s);
	return ret;
}


/* test_helper_config_set */
static int _test_helper_config_set(Locker * locker, char const * section,
		char const * variable, char const * value)
{
	if(locker->config == NULL)
		return -error_set_code(1, "%s", "Configuration not available");
	return config_set(locker->config, section, variable, value);
}


/* test_helper_error */
static int _test_helper_error(Locker * locker, char const * message, int ret)
{
	return error_set_print(PROGNAME, ret, "%s", message);
}


/* callbacks */
/* test_on_apply */
static void _test_on_apply(gpointer data)
{
	Locker * locker = data;
	char const section[] = "demo::";
	char * p;
	char const * q;
	char const * r;

	if((p = malloc(sizeof(section) + strlen(locker->name))) == NULL)
	{
		error_set_print(PROGNAME, 1, "%s", strerror(errno));
		return;
	}
	sprintf(p, "%s%s", section, locker->name);
	q = gtk_entry_get_text(GTK_ENTRY(locker->variable));
	r = gtk_entry_get_text(GTK_ENTRY(locker->value));
	if(_test_helper_config_set(locker, "demo::logo", q, r) != 0)
		error_print(PROGNAME);
	else
	{
		/* XXX really force a configuration reload */
		_test_on_stop(locker);
		_test_on_start(locker);
	}
	free(p);
}


/* test_on_closex */
static gboolean _test_on_closex(void)
{
	gtk_main_quit();
	return TRUE;
}


/* test_on_cycle */
static void _test_on_cycle(gpointer data)
{
	Locker * locker = data;

	_test_helper_action(locker, LOCKER_ACTION_CYCLE);
}


/* test_on_lock */
static void _test_on_lock(gpointer data)
{
	Locker * locker = data;

	_test_helper_action(locker, LOCKER_ACTION_LOCK);
}


/* test_on_start */
static void _test_on_start(gpointer data)
{
	Locker * locker = data;

	_test_helper_action(locker, LOCKER_ACTION_START);
}


/* test_on_stop */
static void _test_on_stop(gpointer data)
{
	Locker * locker = data;

	_test_helper_action(locker, LOCKER_ACTION_STOP);
}


/* test_on_unlock */
static void _test_on_unlock(gpointer data)
{
	Locker * locker = data;

	_test_helper_action(locker, LOCKER_ACTION_UNLOCK);
}


/* public */
/* functions */
/* main */
int main(int argc, char * argv[])
{
	int o;
	char const * auth = NULL;
	int desktop = 0;
	int root = 0;
	int width = 640;
	int height = 480;
	char const * demo = NULL;

	gtk_init(&argc, &argv);
	while((o = getopt(argc, argv, "a:drw:h:")) != -1)
		switch(o)
		{
			case 'a':
				auth = optarg;
				break;
			case 'd':
				desktop = 1;
				root = 0;
				break;
			case 'r':
				desktop = 0;
				root = 1;
				break;
			case 'w':
				width = strtoul(optarg, NULL, 0);
				break;
			case 'h':
				height = strtoul(optarg, NULL, 0);
				break;
			default:
				return _usage();
		}
	if(width == 0 || height == 0 || optind + 1 != argc)
		return _usage();
	demo = argv[optind];
	return (_test(desktop, root, width, height, demo, auth) == 0) ? 2 : 0;
}
