/* $Id$ */
static char const _copyright[] =
"Copyright © 2010-2022 Pierre Pronchery <khorben@defora.org>";
/* This file is part of DeforaOS Desktop Locker */
static char const _license[] =
"Redistribution and use in source and binary forms, with or without\n"
"modification, are permitted provided that the following conditions are\n"
"met:\n"
"\n"
"1. Redistributions of source code must retain the above copyright notice,\n"
"   this list of conditions and the following disclaimer.\n"
"\n"
"2. Redistributions in binary form must reproduce the above copyright notice,\n"
"   this list of conditions and the following disclaimer in the documentation\n"
"   and/or other materials provided with the distribution.\n"
"\n"
"THIS SOFTWARE IS PROVIDED BY ITS AUTHORS AND CONTRIBUTORS \"AS IS\" AND ANY\n"
"EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED\n"
"WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n"
"DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY\n"
"DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES\n"
"(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;\n"
"LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND\n"
"ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT\n"
"(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF\n"
"THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.";



#if defined(__NetBSD__)
# include <sys/param.h>
# include <sys/sysctl.h>
#elif !defined(__FreeBSD__)
# include <fcntl.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <libintl.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#include <X11/extensions/dpms.h>
#include <X11/extensions/scrnsaver.h>
#include <System.h>
#include <Desktop.h>
#include "locker.h"
#include "../config.h"
#define _(string) gettext(string)
#define N_(string) (string)

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

#ifndef PROGNAME_LOCKER
# define PROGNAME_LOCKER "locker"
#endif


/* Locker */
/* private */
/* types */
typedef enum _LockerGeneralColumn
{
	LGC_VALUE = 0,
	LGC_DISPLAY
} LockerGeneralColumn;
#define LGC_LAST LGC_DISPLAY
#define LGC_COUNT (LGC_LAST + 1)

typedef struct _LockerPlugins
{
	char * name;
	Plugin * pplugin;
	LockerPluginDefinition * definition;
	LockerPlugin * plugin;
} LockerPlugins;

struct _Locker
{
	/* settings */
	Config * config;

	/* internal */
	guint source;
	gboolean enabled;
	gboolean locked;

	GdkDisplay * display;
	int screen;
	int event;
	GtkWidget ** windows;
	size_t windows_cnt;

	/* authentication */
	Plugin * aplugin;
	LockerAuthDefinition * adefinition;
	LockerAuth * auth;
	LockerAuthHelper ahelper;

	/* demos */
	Plugin * dplugin;
	LockerDemoDefinition * ddefinition;
	LockerDemo * demo;
	LockerDemoHelper dhelper;

	/* plug-ins */
	LockerPlugins * plugins;
	size_t plugins_cnt;
	LockerPluginHelper phelper;

	/* preferences */
	GtkWidget * pr_window;
	GtkWidget * pr_alock;
	GtkWidget * pr_adelay;
	GtkListStore * pr_astore;
	GtkWidget * pr_acombo;
	GtkListStore * pr_dstore;
	GtkWidget * pr_dcombo;
	GtkWidget * pr_genabled;
	GtkWidget * pr_gtimeout;
	GtkListStore * pr_gstore;
	GtkWidget * pr_gblanking;
	GtkWidget * pr_gdenabled;
	GtkWidget * pr_gdpms1;
	GtkWidget * pr_gdpms2;
	GtkWidget * pr_gdpms3;
	GtkListStore * pr_plstore;
	GtkWidget * pr_plview;

	/* about */
	GtkWidget * ab_window;
};

typedef enum _LockerPluginsColumn
{
	LPC_PLUGIN = 0,
	LPC_ENABLED,
	LPC_FILENAME,
	LPC_ICON,
	LPC_NAME
} LockerPluginsColumn;
#define LPC_LAST LPC_NAME
#define LPC_COUNT (LPC_LAST + 1)


/* constants */
static char const * _authors[] =
{
	"Pierre Pronchery <khorben@defora.org>",
	NULL
};


/* prototypes */
/* accessors */
static size_t _locker_get_primary_monitor(Locker * locker);
static gboolean _locker_is_locked(Locker * locker);
static gboolean _locker_plugin_is_enabled(Locker * locker, char const * plugin);

/* useful */
static void _locker_about(Locker * locker);

static int _locker_activate(Locker * locker, int force);

static int _locker_action(Locker * locker, LockerAction action);

/* authentication */
static char const * _locker_auth_config_get(Locker * locker,
		char const * section, char const * variable);
static int _locker_auth_config_set(Locker * locker, char const * section,
		char const * variable, char const * value);
static GtkWidget * _locker_auth_load(Locker * locker, char const * plugin);
static void _locker_auth_unload(Locker * locker);

/* configuration */
static int _locker_config_load(Locker * locker);
static int _locker_config_save(Locker * locker);

static int _locker_cycle(Locker * locker, int force);

static int _locker_deactivate(Locker * locker, int force);

/* demos */
static char const * _locker_demo_config_get(Locker * locker,
		char const * section, char const * variable);
static int _locker_demo_config_set(Locker * locker, char const * section,
		char const * variable, char const * value);
static int _locker_demo_load(Locker * locker, char const * demo);
static void _locker_demo_reload(Locker * locker);
static void _locker_demo_start(Locker * locker);
static void _locker_demo_stop(Locker * locker);
static void _locker_demo_unload(Locker * locker);

static void _locker_disable(Locker * locker);
static void _locker_enable(Locker * locker);

static int _locker_error(Locker * locker, char const * message, int ret);

static int _locker_event(Locker * locker, LockerEvent event);

static int _locker_lock(Locker * locker, int force);

/* plug-ins */
static char const * _locker_plugin_config_get(Locker * locker,
		char const * section, char const * variable);
static int _locker_plugin_config_set(Locker * locker, char const * section,
		char const * variable, char const * value);
static int _locker_plugin_load(Locker * locker, char const * plugin);
static int _locker_plugin_unload(Locker * locker, char const * plugin);

static int _locker_unlock(Locker * locker, int force);

/* windows */
static void _locker_window_register(Locker * locker, size_t i);

/* callbacks */
static gboolean _locker_on_closex(void);
static gboolean _locker_on_configure(GtkWidget * widget, GdkEvent * event,
		gpointer data);
static GdkFilterReturn _locker_on_filter(GdkXEvent * xevent, GdkEvent * event,
		gpointer data);
static gboolean _locker_on_lock(gpointer data);
static gboolean _locker_on_map_event(GtkWidget * widget, GdkEvent * event,
		gpointer data);
static int _locker_on_message(void * data, uint32_t value1, uint32_t value2,
		uint32_t value3);
static void _locker_on_preferences_general_dpms_toggled(gpointer data);
static void _locker_on_preferences_general_toggled(gpointer data);
static void _locker_on_preferences_lock_toggled(gpointer data);
static void _locker_on_realize(GtkWidget * widget, gpointer data);


/* public */
/* functions */
/* locker_new */
static int _new_config(Locker * locker);
static void _new_helpers(Locker * locker);
static int _new_plugins(Locker * locker);
static int _new_xss(Locker * locker);

Locker * locker_new(char const * demo, char const * auth)
{
	Locker * locker;
	GdkScreen * screen;
	GtkWidget * widget = NULL;
	size_t cnt;
	size_t i;
	GdkWindow * root;

	if((locker = object_new(sizeof(*locker))) == NULL)
	{
		_locker_error(NULL, error_get(NULL), 1);
		return NULL;
	}
	_new_helpers(locker);
	locker->source = 0;
	locker->enabled = TRUE;
	locker->locked = FALSE;
	locker->display = gdk_display_get_default();
	screen = gdk_display_get_default_screen(locker->display);
	locker->screen = gdk_x11_screen_get_screen_number(screen);
	/* XXX assume at least one monitor */
#if GTK_CHECK_VERSION(3, 22, 0)
	if((cnt = gdk_display_get_n_monitors(locker->display)) < 1)
		cnt = 1;
#else
	if((cnt = gdk_screen_get_n_monitors(screen)) < 1)
		cnt = 1;
#endif
	if((locker->windows = malloc(sizeof(*locker->windows) * cnt)) != NULL)
		for(i = 0; i < cnt; i++)
			locker->windows[i] = NULL;
	locker->windows_cnt = cnt;
	locker->aplugin = NULL;
	locker->adefinition = NULL;
	locker->auth = NULL;
	locker->dplugin = NULL;
	locker->ddefinition = NULL;
	locker->demo = NULL;
	locker->plugins = NULL;
	locker->plugins_cnt = 0;
	locker->pr_window = NULL;
	locker->ab_window = NULL;
	/* check for errors */
	if(locker->windows == NULL
			|| _new_config(locker) != 0
			|| _locker_demo_load(locker, demo) != 0
			|| (widget = _locker_auth_load(locker, auth)) == NULL
			|| _new_xss(locker) != 0)
	{
		_locker_error(NULL, error_get(NULL), 1);
		locker_delete(locker);
		return NULL;
	}
	_new_plugins(locker);
	/* create windows */
	for(i = 0; i < locker->windows_cnt; i++)
		_locker_window_register(locker, i);
	/* pack the authentication widget */
	i = _locker_get_primary_monitor(locker);
	gtk_container_add(GTK_CONTAINER(locker->windows[i]), widget);
	root = gdk_get_default_root_window();
	XScreenSaverSelectInput(GDK_DISPLAY_XDISPLAY(locker->display),
			GDK_WINDOW_XID(root),
			ScreenSaverNotifyMask | ScreenSaverCycleMask);
	gdk_x11_register_standard_event_type(locker->display, locker->event, 1);
	gdk_window_add_filter(root, _locker_on_filter, locker);
	/* listen to desktop messages */
	gtk_widget_realize(locker->windows[0]);
	desktop_message_register(locker->windows[0], LOCKER_CLIENT_MESSAGE,
			_locker_on_message, locker);
	return locker;
}

static int _new_config(Locker * locker)
{
	if((locker->config = config_new()) == NULL)
		return -1;
	/* ignore errors */
	_locker_config_load(locker);
	return 0;
}

static void _new_helpers(Locker * locker)
{
	/* authentication helper */
	locker->ahelper.locker = locker;
	locker->ahelper.error = _locker_error;
	locker->ahelper.action = _locker_action;
	locker->ahelper.config_get = _locker_auth_config_get;
	locker->ahelper.config_set = _locker_auth_config_set;
	/* demos helper */
	locker->dhelper.locker = locker;
	locker->dhelper.error = _locker_error;
	locker->dhelper.config_get = _locker_demo_config_get;
	locker->dhelper.config_set = _locker_demo_config_set;
	/* plug-ins helper */
	locker->phelper.locker = locker;
	locker->phelper.error = _locker_error;
	locker->phelper.about_dialog = _locker_about;
	locker->phelper.action = _locker_action;
	locker->phelper.config_get = _locker_plugin_config_get;
	locker->phelper.config_set = _locker_plugin_config_set;
}

static int _new_plugins(Locker * locker)
{
	int ret = 0;
	char const * p;
	String * plugins;
	size_t i;
	int c;

	if((p = config_get(locker->config, NULL, "plugins")) == NULL)
		p = "systray";
	else if(strlen(p) == 0)
		return 0;
	if((plugins = string_new(p)) == NULL)
		return -1;
	for(i = 0;; i++)
	{
		if(plugins[i] != ',' && plugins[i] != '\0')
			continue;
		c = plugins[i];
		plugins[i] = '\0';
		ret |= _locker_plugin_load(locker, plugins);
		if(c == '\0')
			break;
		plugins += i + 1;
		i = 0;
	}
	string_delete(plugins);
	return ret;
}

static int _new_xss(Locker * locker)
{
	Display * display = GDK_DISPLAY_XDISPLAY(locker->display);
	int error;

	/* register as screensaver */
	if(XScreenSaverQueryExtension(display, &locker->event, &error) == 0
			|| XScreenSaverRegister(display, locker->screen,
				getpid(), XA_INTEGER) == 0)
		return -error_set_code(1, "%s",
				_("Could not register as screensaver"));
	return 0;
}


/* locker_delete */
void locker_delete(Locker * locker)
{
	size_t i;
	LockerPlugins * p;

	if(locker->source != 0)
		g_source_remove(locker->source);
	/* destroy the generic plug-ins */
	for(i = 0; i < locker->plugins_cnt; i++)
	{
		p = &locker->plugins[i];
		p->definition->destroy(p->plugin);
		plugin_delete(p->pplugin);
		free(p->name);
	}
	free(locker->plugins);
	/* destroy the authentication plug-in */
	_locker_auth_unload(locker);
	/* destroy the demo plug-in */
	_locker_demo_unload(locker);
	/* destroy the windows */
	for(i = 0; i < locker->windows_cnt; i++)
		if(locker->windows[i] != NULL)
			gtk_widget_destroy(locker->windows[i]);
	free(locker->windows);
	if(locker->ab_window != NULL)
		gtk_widget_destroy(locker->ab_window);
	XScreenSaverUnregister(GDK_DISPLAY_XDISPLAY(locker->display),
			locker->screen);
	if(locker->config != NULL)
		config_delete(locker->config);
	object_delete(locker);
}


/* useful */
/* locker_show_preferences */
static void _preferences_window(Locker * locker);
static GtkWidget * _preferences_window_auth(Locker * locker);
static GtkWidget * _preferences_window_demo(Locker * locker);
static GtkWidget * _preferences_window_general(Locker * locker);
static GtkWidget * _preferences_window_plugins(Locker * locker);
/* callbacks */
static void _preferences_on_cancel(gpointer data);
static void _cancel_auth(Locker * locker, GtkListStore * store);
static void _cancel_demo(Locker * locker, GtkListStore * store);
static void _cancel_general(Locker * locker);
static void _cancel_general_dpms(Locker * locker);
static void _cancel_plugins(Locker * locker, GtkListStore * store);
static void _preferences_on_apply(gpointer data);
static gboolean _preferences_on_closex(gpointer data);
static void _preferences_on_ok(gpointer data);
static void _preferences_on_plugins_toggled(GtkCellRendererToggle * renderer,
		char * path, gpointer data);
static void _preferences_on_response(GtkWidget * widget, gint response,
		gpointer data);

void locker_show_preferences(Locker * locker, gboolean show)
{
	if(locker->pr_window == NULL)
		_preferences_window(locker);
	if(show)
		gtk_window_present(GTK_WINDOW(locker->pr_window));
	else
		gtk_widget_hide(locker->pr_window);
}

static void _preferences_window(Locker * locker)
{
	GtkWidget * vbox;
	GtkWidget * notebook;
	GtkWidget * widget;

	locker->pr_window = gtk_dialog_new_with_buttons(
			_("Screensaver preferences"), NULL, 0,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
			GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_window_set_position(GTK_WINDOW(locker->pr_window),
			GTK_WIN_POS_CENTER);
	gtk_window_set_resizable(GTK_WINDOW(locker->pr_window), FALSE);
	g_signal_connect_swapped(locker->pr_window, "delete-event", G_CALLBACK(
				_preferences_on_closex), locker);
	g_signal_connect(locker->pr_window, "response", G_CALLBACK(
				_preferences_on_response), locker);
	notebook = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
	/* general */
	widget = _preferences_window_general(locker);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, gtk_label_new(
				_("General")));
	/* authentication */
	widget = _preferences_window_auth(locker);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, gtk_label_new(
				_("Authentication")));
	/* demos */
	widget = _preferences_window_demo(locker);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, gtk_label_new(
				_("Demos")));
	/* plug-ins */
	widget = _preferences_window_plugins(locker);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget,
			gtk_label_new(_("Plug-ins")));
#if GTK_CHECK_VERSION(2, 14, 0)
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(locker->pr_window));
#else
	vbox = GTK_DIALOG(locker->pr_window)->vbox;
#endif
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
	_preferences_on_cancel(locker);
	gtk_widget_show_all(vbox);
}

static GtkWidget * _preferences_window_auth(Locker * locker)
{
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * widget;
	GtkCellRenderer * renderer;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	/* checkbox */
	locker->pr_alock = gtk_check_button_new_with_label(_(
				"Lock the screen automatically"));
	g_signal_connect_swapped(locker->pr_alock, "toggled", G_CALLBACK(
				_locker_on_preferences_lock_toggled), locker);
	gtk_box_pack_start(GTK_BOX(vbox), locker->pr_alock, FALSE, TRUE, 0);
	/* spinner */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(_("Lock after "));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	locker->pr_adelay = gtk_spin_button_new_with_range(0.0, 3600.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(locker->pr_adelay), 0);
	gtk_box_pack_start(GTK_BOX(hbox), locker->pr_adelay, FALSE, TRUE, 0);
	widget = gtk_label_new(_(" second(s)"));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* selector */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(_("Method: "));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	locker->pr_astore = gtk_list_store_new(LPC_COUNT, G_TYPE_POINTER,
			G_TYPE_BOOLEAN, G_TYPE_STRING, GDK_TYPE_PIXBUF,
			G_TYPE_STRING);
	locker->pr_acombo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(
				locker->pr_astore));
	widget = locker->pr_acombo;
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(widget), renderer,
			"pixbuf", LPC_ICON, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(widget), renderer,
			"text", LPC_NAME, NULL);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	return vbox;
}

static GtkWidget * _preferences_window_demo(Locker * locker)
{
	GtkWidget * vbox;
	GtkWidget * hbox;
	GtkWidget * widget;
	GtkCellRenderer * renderer;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	/* selector */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(_("Plug-in: "));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	locker->pr_dstore = gtk_list_store_new(LPC_COUNT, G_TYPE_POINTER,
			G_TYPE_BOOLEAN, G_TYPE_STRING, GDK_TYPE_PIXBUF,
			G_TYPE_STRING);
	locker->pr_dcombo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(
				locker->pr_dstore));
	widget = locker->pr_dcombo;
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, FALSE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(widget), renderer,
			"pixbuf", LPC_ICON, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(widget), renderer,
			"text", LPC_NAME, NULL);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	return vbox;
}

static GtkWidget * _preferences_window_general(Locker * locker)
{
	GtkWidget * vbox;
	GtkWidget * vbox2;
	GtkWidget * hbox;
	GtkWidget * frame;
	GtkWidget * widget;
	GtkSizeGroup * group;
	GtkCellRenderer * renderer;
	struct {
		int value;
		char const * name;
	} blanking[] =
	{
		{ DefaultBlanking, N_("Default") },
		{ DontPreferBlanking, N_("Do not blank") },
		{ PreferBlanking, N_("Blank screen") }
	};
	size_t i;
	GtkTreeIter iter;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	/* enable */
	locker->pr_genabled = gtk_check_button_new_with_label(
			_("Enable screen saving"));
	g_signal_connect_swapped(locker->pr_genabled, "toggled", G_CALLBACK(
				_locker_on_preferences_general_toggled),
			locker);
	gtk_box_pack_start(GTK_BOX(vbox), locker->pr_genabled, FALSE, TRUE, 0);
	/* timeout */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(_("After: "));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	locker->pr_gtimeout = gtk_spin_button_new_with_range(1.0, 3600.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(locker->pr_gtimeout), 0);
	gtk_box_pack_start(GTK_BOX(hbox), locker->pr_gtimeout, FALSE, TRUE, 0);
	widget = gtk_label_new(_(" second(s)"));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* blanking */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(_("Blanking: "));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	locker->pr_gstore = gtk_list_store_new(LGC_COUNT,
			G_TYPE_INT, G_TYPE_STRING);
	for(i = 0; i < sizeof(blanking) / sizeof(*blanking); i++)
	{
#if GTK_CHECK_VERSION(2, 6, 0)
		gtk_list_store_insert_with_values(locker->pr_gstore, &iter, -1,
#else
		gtk_list_store_append(locker->pr_gstore, &iter);
		gtk_list_store_set(locker->pr_gstore, &iter,
#endif
			LGC_VALUE, blanking[i].value,
			LGC_DISPLAY, _(blanking[i].name), -1);
	}
	locker->pr_gblanking = gtk_combo_box_new_with_model(GTK_TREE_MODEL(
				locker->pr_gstore));
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(locker->pr_gblanking),
			renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(locker->pr_gblanking),
			renderer, "text", LGC_DISPLAY, NULL);
	gtk_box_pack_start(GTK_BOX(hbox), locker->pr_gblanking, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	/* DPMS */
	frame = gtk_frame_new(_("Energy saving"));
	gtk_container_set_border_width(GTK_CONTAINER(frame), 4);
	vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox2), 4);
	/* DPMS: enabled */
	locker->pr_gdenabled = gtk_check_button_new_with_label(
			_("Enable DPMS"));
	g_signal_connect_swapped(locker->pr_gdenabled, "toggled", G_CALLBACK(
				_locker_on_preferences_general_dpms_toggled),
			locker);
#if GTK_CHECK_VERSION(2, 12, 0)
	gtk_widget_set_tooltip_text(locker->pr_gdenabled,
			_("DPMS manages the power supply of video monitors"
				" for computers through the graphics card"));
#endif
	gtk_box_pack_start(GTK_BOX(vbox2), locker->pr_gdenabled, FALSE, TRUE,
			0);
	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);
	/* DPMS: standby */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(_("Standby: "));
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	locker->pr_gdpms1 = gtk_spin_button_new_with_range(0.0, 3600.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(locker->pr_gdpms1), 0);
	gtk_box_pack_start(GTK_BOX(hbox), locker->pr_gdpms1, FALSE, TRUE, 0);
	widget = gtk_label_new(_(" second(s)"));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, TRUE, 0);
	/* DPMS: suspend */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(_("Suspend: "));
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	locker->pr_gdpms2 = gtk_spin_button_new_with_range(0.0, 3600.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(locker->pr_gdpms2), 0);
	gtk_box_pack_start(GTK_BOX(hbox), locker->pr_gdpms2, FALSE, TRUE, 0);
	widget = gtk_label_new(_(" second(s)"));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, TRUE, 0);
	/* DPMS: off */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	widget = gtk_label_new(_("Off: "));
#if GTK_CHECK_VERSION(3, 0, 0)
	g_object_set(widget, "halign", GTK_ALIGN_START, NULL);
#else
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
#endif
	gtk_size_group_add_widget(group, widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	locker->pr_gdpms3 = gtk_spin_button_new_with_range(0.0, 3600.0, 1.0);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(locker->pr_gdpms3), 0);
	gtk_box_pack_start(GTK_BOX(hbox), locker->pr_gdpms3, FALSE, TRUE, 0);
	widget = gtk_label_new(_(" second(s)"));
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);
	return vbox;
}

static GtkWidget * _preferences_window_plugins(Locker * locker)
{
	GtkWidget * vbox;
	GtkWidget * widget;
	GtkCellRenderer * renderer;
	GtkTreeViewColumn * column;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(widget),
			GTK_SHADOW_ETCHED_IN);
	locker->pr_plstore = gtk_list_store_new(LPC_COUNT, G_TYPE_POINTER,
			G_TYPE_BOOLEAN, G_TYPE_STRING, GDK_TYPE_PIXBUF,
			G_TYPE_STRING);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(
				locker->pr_plstore), LPC_NAME,
			GTK_SORT_ASCENDING);
	locker->pr_plview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(
				locker->pr_plstore));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(locker->pr_plview),
			FALSE);
	gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(locker->pr_plview), TRUE);
	/* enabled */
	renderer = gtk_cell_renderer_toggle_new();
	g_signal_connect(renderer, "toggled", G_CALLBACK(
				_preferences_on_plugins_toggled), locker);
	column = gtk_tree_view_column_new_with_attributes(_("Enabled"),
			renderer, "active", LPC_ENABLED, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(locker->pr_plview), column);
	/* icon */
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
			"pixbuf", LPC_ICON, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(locker->pr_plview), column);
	/* name */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer,
			"text", LPC_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(locker->pr_plview), column);
	gtk_container_add(GTK_CONTAINER(widget), locker->pr_plview);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
	return vbox;
}

static void _preferences_on_apply(gpointer data)
{
	Locker * locker = data;
	Display * display = GDK_DISPLAY_XDISPLAY(locker->display);
	int timeout = 0;
	CARD16 dpms1 = 0;
	CARD16 dpms2 = 0;
	CARD16 dpms3 = 0;
	int interval = 0;
	int prefer_blanking = 0;
	int allow_exposures = 0;
	char buf[16] = "";
	GtkTreeModel * model = GTK_TREE_MODEL(locker->pr_plstore);
	GtkTreeIter iter;
	gchar * p;
	GtkWidget * widget;
	gboolean valid;
	gboolean enabled;
	int i;
	String * value = string_new("");
	String * sep = "";

	/* general */
	XGetScreenSaver(display, &timeout, &interval, &prefer_blanking,
			&allow_exposures);
	timeout = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
				locker->pr_genabled))
		? gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(
					locker->pr_gtimeout)) : 0;
	if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(locker->pr_gblanking),
				&iter))
		gtk_tree_model_get(GTK_TREE_MODEL(locker->pr_gstore), &iter,
				LGC_VALUE, &prefer_blanking, -1);
	XSetScreenSaver(display, timeout, interval, prefer_blanking,
			allow_exposures);
	if(DPMSCapable(display))
	{
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
					locker->pr_gdenabled))
			? DPMSEnable(display) : DPMSDisable(display);
		dpms1 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(
					locker->pr_gdpms1));
		dpms2 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(
					locker->pr_gdpms2));
		dpms3 = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(
					locker->pr_gdpms3));
		DPMSSetTimeouts(display, dpms1, dpms2, dpms3);
	}
	/* authentication */
	if((enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
						locker->pr_alock))) == TRUE)
	{
		if((i = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(
							locker->pr_adelay)))
				>= 0)
			snprintf(buf, sizeof(buf), "%d", i);
		config_set(locker->config, NULL, "lock", buf);
	}
	else
		config_set(locker->config, NULL, "lock", NULL);
	p = NULL;
	if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(locker->pr_acombo),
				&iter))
		gtk_tree_model_get(GTK_TREE_MODEL(locker->pr_astore), &iter,
				LPC_FILENAME, &p, -1);
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() auth=\"%s\"\n", __func__, p);
#endif
	config_set(locker->config, NULL, "auth", p);
	/* XXX report errors */
	i = _locker_get_primary_monitor(locker);
	if((widget = _locker_auth_load(locker, p)) != NULL)
		gtk_container_add(GTK_CONTAINER(locker->windows[i]), widget);
	g_free(p);
	/* demos */
	p = NULL;
	if(gtk_combo_box_get_active_iter(GTK_COMBO_BOX(locker->pr_dcombo),
				&iter))
		gtk_tree_model_get(GTK_TREE_MODEL(locker->pr_dstore), &iter,
				LPC_FILENAME, &p, -1);
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() demo=\"%s\"\n", __func__, p);
#endif
	config_set(locker->config, NULL, "demo", p);
	/* XXX check errors */
	_locker_demo_load(locker, p);
	g_free(p);
	/* plug-ins */
	i = 0;
	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid == TRUE;
			valid = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(model, &iter, LPC_FILENAME, &p,
				LPC_ENABLED, &enabled, -1);
		/* FIXME also save the configuration */
		if(enabled)
		{
			_locker_plugin_load(locker, p);
			i |= string_append(&value, sep);
			i |= string_append(&value, p);
			sep = ",";
		}
		else
			_locker_plugin_unload(locker, p);
		g_free(p);
	}
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() value=\"%s\"\n", __func__, value);
#endif
	if(i == 0)
		config_set(locker->config, NULL, "plugins", value);
	string_delete(value);
	_cancel_plugins(locker, locker->pr_plstore); /* XXX */
}

static void _preferences_on_cancel(gpointer data)
{
	Locker * locker = data;

	gtk_widget_hide(locker->pr_window);
	/* general */
	_cancel_general(locker);
	/* authentication */
	_cancel_auth(locker, locker->pr_astore);
	/* demos */
	_cancel_demo(locker, locker->pr_dstore);
	/* plug-ins */
	_cancel_plugins(locker, locker->pr_plstore);
}

static void _cancel_auth(Locker * locker, GtkListStore * store)
{
	char const * q;
	gboolean active;
	GtkIconTheme * theme;
	GtkTreeIter iter;
	GdkPixbuf * icon;
	DIR * dir;
	struct dirent * de;
	size_t len;
#ifdef __APPLE__
	char const ext[] = ".dylib";
#else
	char const ext[] = ".so";
#endif
	Plugin * p;
	LockerAuthDefinition * lad;
	gint size = 24;

	if((q = config_get(locker->config, NULL, "lock")) != NULL)
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(locker->pr_adelay),
				strtol(q, NULL, 10));
	else
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(locker->pr_adelay),
				0.0);
	active = (q != NULL) ? TRUE : FALSE;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(locker->pr_alock),
			active);
	gtk_widget_set_sensitive(locker->pr_adelay, active);
	gtk_widget_set_sensitive(locker->pr_acombo, active);
	theme = gtk_icon_theme_get_default();
	gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &size, &size);
	gtk_list_store_clear(store);
	if((dir = opendir(LIBDIR "/" PACKAGE "/auth")) == NULL)
		return;
	while((de = readdir(dir)) != NULL)
	{
		if((len = strlen(de->d_name)) < sizeof(ext))
			continue;
		if(strcmp(&de->d_name[len - sizeof(ext) + 1], ext) != 0)
			continue;
		de->d_name[len - sizeof(ext) + 1] = '\0';
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() \"%s\"\n", __func__, de->d_name);
#endif
		if((p = plugin_new(LIBDIR, PACKAGE, "auth", de->d_name))
				== NULL)
			continue;
		if((lad = plugin_lookup(p, "plugin")) == NULL)
		{
			plugin_delete(p);
			continue;
		}
#if GTK_CHECK_VERSION(2, 6, 0)
		gtk_list_store_insert_with_values(store, &iter, -1,
#else
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
#endif
				LPC_FILENAME, de->d_name, LPC_NAME, lad->name,
				-1);
		/* select if currently active */
		if(locker->adefinition != NULL
				/* XXX check on de->d_name instead */
				&& strcmp(locker->adefinition->name, lad->name)
				== 0)
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(
						locker->pr_acombo), &iter);
		icon = NULL;
		if(lad->icon != NULL)
			icon = gtk_icon_theme_load_icon(theme, lad->icon, size,
					0, NULL);
		if(icon == NULL)
			icon = gtk_icon_theme_load_icon(theme, "gnome-settings",
					size, 0, NULL);
		gtk_list_store_set(store, &iter, LPC_ICON, icon, -1);
		plugin_delete(p);
	}
	closedir(dir);
}

static void _cancel_demo(Locker * locker, GtkListStore * store)
{
	GtkIconTheme * theme;
	GtkTreeIter iter;
	GdkPixbuf * icon;
	DIR * dir;
	struct dirent * de;
	size_t len;
#ifdef __APPLE__
	char const ext[] = ".dylib";
#else
	char const ext[] = ".so";
#endif
	Plugin * p;
	LockerDemoDefinition * ldd;
	gint size = 24;

	theme = gtk_icon_theme_get_default();
	gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &size, &size);
	gtk_list_store_clear(store);
	/* "Disabled", selected by default */
	icon = gtk_icon_theme_load_icon(theme, "gtk-cancel", size, 0, NULL);
#if GTK_CHECK_VERSION(2, 6, 0)
	gtk_list_store_insert_with_values(store, &iter, -1,
#else
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter,
#endif
			LPC_FILENAME, NULL, LPC_NAME, _("Disabled"),
			LPC_ICON, icon, -1);
	gtk_combo_box_set_active_iter(GTK_COMBO_BOX(locker->pr_dcombo), &iter);
	if((dir = opendir(LIBDIR "/" PACKAGE "/demos")) == NULL)
		return;
	while((de = readdir(dir)) != NULL)
	{
		if((len = strlen(de->d_name)) < sizeof(ext))
			continue;
		if(strcmp(&de->d_name[len - sizeof(ext) + 1], ext) != 0)
			continue;
		de->d_name[len - sizeof(ext) + 1] = '\0';
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() \"%s\"\n", __func__,
				de->d_name);
#endif
		if((p = plugin_new(LIBDIR, PACKAGE, "demos", de->d_name))
				== NULL)
			continue;
		if((ldd = plugin_lookup(p, "plugin")) == NULL)
		{
			plugin_delete(p);
			continue;
		}
#if GTK_CHECK_VERSION(2, 6, 0)
		gtk_list_store_insert_with_values(store, &iter, -1,
#else
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
#endif
				LPC_FILENAME, de->d_name, LPC_NAME, ldd->name,
				-1);
		/* select if currently active */
		if(locker->ddefinition != NULL
				/* XXX check on de->d_name instead */
				&& strcmp(locker->ddefinition->name, ldd->name)
				== 0)
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(
						locker->pr_dcombo), &iter);
		icon = NULL;
		if(ldd->icon != NULL)
			icon = gtk_icon_theme_load_icon(theme, ldd->icon, size,
					0, NULL);
		if(icon == NULL)
			icon = gtk_icon_theme_load_icon(theme, "gnome-settings",
					size, 0, NULL);
		if(icon != NULL)
			gtk_list_store_set(store, &iter, LPC_ICON, icon, -1);
		plugin_delete(p);
	}
	closedir(dir);
}

static void _cancel_general(Locker * locker)
{
	GtkTreeModel * model = GTK_TREE_MODEL(locker->pr_gstore);
	GtkTreeIter iter;
	int timeout = 0;
	int interval = 0;
	int prefer_blanking = 0;
	int allow_exposures = 0;
	gboolean valid;
	int i;

	XGetScreenSaver(GDK_DISPLAY_XDISPLAY(locker->display), &timeout,
			&interval, &prefer_blanking, &allow_exposures);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(locker->pr_genabled),
			(timeout != 0) ? TRUE : FALSE);
	_locker_on_preferences_general_dpms_toggled(locker);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(locker->pr_gtimeout),
			timeout);
	for(valid = gtk_tree_model_get_iter_first(model, &iter); valid == TRUE;
			valid = gtk_tree_model_iter_next(model, &iter))
	{
		gtk_tree_model_get(model, &iter, LGC_VALUE, &i, -1);
		if(i == prefer_blanking)
		{
			gtk_combo_box_set_active_iter(GTK_COMBO_BOX(
						locker->pr_gblanking), &iter);
			break;
		}
	}
	/* DPMS */
	if(DPMSCapable(GDK_DISPLAY_XDISPLAY(locker->display)))
		_cancel_general_dpms(locker);
	else
		gtk_widget_set_sensitive(locker->pr_gdenabled, FALSE);
}

static void _cancel_general_dpms(Locker * locker)
{
	Display * display = GDK_DISPLAY_XDISPLAY(locker->display);
	CARD16 power_level;
	BOOL state;
	CARD16 dpms1 = 0;
	CARD16 dpms2 = 0;
	CARD16 dpms3 = 0;

	if(DPMSInfo(display, &power_level, &state) != TRUE)
		state = FALSE;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(locker->pr_gdenabled),
			state);
	if(state == FALSE)
		return;
	if(DPMSGetTimeouts(display, &dpms1, &dpms2, &dpms3) != TRUE)
		return;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(locker->pr_gdpms1), dpms1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(locker->pr_gdpms2), dpms2);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(locker->pr_gdpms3), dpms3);
}

static void _cancel_plugins(Locker * locker, GtkListStore * store)
{
	GtkIconTheme * theme;
	GtkTreeIter iter;
	gboolean enabled;
	GdkPixbuf * icon;
	DIR * dir;
	struct dirent * de;
	size_t len;
#ifdef __APPLE__
	char const ext[] = ".dylib";
#else
	char const ext[] = ".so";
#endif
	Plugin * p;
	LockerPluginDefinition * lpd;

	gtk_list_store_clear(store);
	if((dir = opendir(LIBDIR "/" PACKAGE "/plugins")) == NULL)
		return;
	theme = gtk_icon_theme_get_default();
	while((de = readdir(dir)) != NULL)
	{
		if((len = strlen(de->d_name)) < sizeof(ext))
			continue;
		if(strcmp(&de->d_name[len - sizeof(ext) + 1], ext) != 0)
			continue;
		de->d_name[len - sizeof(ext) + 1] = '\0';
#ifdef DEBUG
		fprintf(stderr, "DEBUG: %s() \"%s\"\n", __func__, de->d_name);
#endif
		if((p = plugin_new(LIBDIR, PACKAGE, "plugins", de->d_name))
				== NULL)
			continue;
		if((lpd = plugin_lookup(p, "plugin")) == NULL)
		{
			plugin_delete(p);
			continue;
		}
		enabled = _locker_plugin_is_enabled(locker, de->d_name);
		icon = NULL;
		if(lpd->icon != NULL)
			icon = gtk_icon_theme_load_icon(theme, lpd->icon, 24, 0,
					NULL);
		if(icon == NULL)
			icon = gtk_icon_theme_load_icon(theme, "gnome-settings",
					24, 0, NULL);
#if GTK_CHECK_VERSION(2, 6, 0)
		gtk_list_store_insert_with_values(store, &iter, -1,
#else
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
#endif
				LPC_FILENAME, de->d_name, LPC_NAME, lpd->name,
				LPC_ENABLED, enabled, LPC_ICON, icon, -1);
		plugin_delete(p);
	}
	closedir(dir);
}

static gboolean _preferences_on_closex(gpointer data)
{
	Locker * locker = data;

	gtk_widget_hide(locker->pr_window);
	return TRUE;
}

static void _preferences_on_ok(gpointer data)
{
	Locker * locker = data;

	gtk_widget_hide(locker->pr_window);
	_preferences_on_apply(locker);
	_locker_config_save(locker);
}

static void _preferences_on_plugins_toggled(GtkCellRendererToggle * renderer,
		char * path, gpointer data)
{
	Locker * locker = data;
	GtkTreeIter iter;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(locker->pr_plstore),
			&iter, path);
	gtk_list_store_set(locker->pr_plstore, &iter, LPC_ENABLED,
			!gtk_cell_renderer_toggle_get_active(renderer), -1);
}

static void _preferences_on_response(GtkWidget * widget, gint response,
		gpointer data)
{
	Locker * locker = data;
	(void) widget;

	if(response == GTK_RESPONSE_OK)
		_preferences_on_ok(locker);
	else if(response == GTK_RESPONSE_APPLY)
		_preferences_on_apply(locker);
	else if(response == GTK_RESPONSE_CANCEL)
		_preferences_on_cancel(locker);
}


/* private */
/* functions */
/* accessors */
/* locker_get_primary */
static size_t _locker_get_primary_monitor(Locker * locker)
{
	int primary;
#if GTK_CHECK_VERSION(3, 22, 0)
	GdkMonitor * monitor;
	int i;

	for(i = 0; i < gdk_display_get_n_monitors(locker->display); i++)
		if((monitor = gdk_display_get_monitor(locker->display, i))
				!= NULL
				&& gdk_monitor_is_primary(monitor))
			return i;
	primary = 0;
#elif GTK_CHECK_VERSION(2, 20, 0)
	GdkScreen * screen;

	screen = gdk_display_get_default_screen(locker->display);
	primary = gdk_screen_get_primary_monitor(screen);
#else
	primary = 0;
#endif
	if(primary < 0 || (size_t)primary > locker->windows_cnt
			|| locker->windows[primary] == NULL)
		primary = 0;
	return (size_t)primary;
}


/* locker_is_locked */
static gboolean _locker_is_locked(Locker * locker)
{
	return locker->locked;
}


/* locker_plugin_is_enabled */
static gboolean _locker_plugin_is_enabled(Locker * locker, char const * plugin)
{
	size_t i;

	for(i = 0; i < locker->plugins_cnt; i++)
		if(strcmp(locker->plugins[i].name, plugin) == 0)
			return TRUE;
	return FALSE;
}


/* useful */
/* locker_about */
static gboolean _about_on_closex(gpointer data);

static void _locker_about(Locker * locker)
{
	GtkWidget * dialog;

	if(locker->ab_window != NULL)
	{
		gtk_window_present(GTK_WINDOW(locker->ab_window));
		return;
	}
	dialog = desktop_about_dialog_new();
	locker->ab_window = dialog;
	g_signal_connect_swapped(G_OBJECT(dialog), "delete-event", G_CALLBACK(
				_about_on_closex), locker);
	desktop_about_dialog_set_name(dialog, PACKAGE);
	desktop_about_dialog_set_version(dialog, VERSION);
	desktop_about_dialog_set_authors(dialog, _authors);
	desktop_about_dialog_set_comments(dialog,
			_("Screensaver for the DeforaOS desktop"));
	desktop_about_dialog_set_copyright(dialog, _copyright);
	desktop_about_dialog_set_logo_icon_name(dialog, "gnome-lockscreen");
	desktop_about_dialog_set_license(dialog, _license);
	desktop_about_dialog_set_translator_credits(dialog,
			_("translator-credits"));
	desktop_about_dialog_set_website(dialog, "http://www.defora.org/");
	gtk_widget_show(dialog);
}

static gboolean _about_on_closex(gpointer data)
{
	Locker * locker = data;

	gtk_widget_hide(locker->ab_window);
	return TRUE;
}


/* actions */
/* locker_action */
static int _locker_action_activate(Locker * locker);
static int _locker_action_cycle(Locker * locker);
static int _locker_action_deactivate(Locker * locker);
static int _locker_action_disable(Locker * locker);
static int _locker_action_enable(Locker * locker);
static int _locker_action_lock(Locker * locker);
static int _locker_action_reload(Locker * locker);
static int _locker_action_start(Locker * locker);
static int _locker_action_stop(Locker * locker);
static int _locker_action_suspend(Locker * locker);
static int _locker_action_unlock(Locker * locker);

static int _locker_action(Locker * locker, LockerAction action)
{
	int ret = -1;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__, action);
#endif
	switch(action)
	{
		case LOCKER_ACTION_ACTIVATE:
			ret = _locker_action_activate(locker);
			break;
		case LOCKER_ACTION_CYCLE:
			ret = _locker_action_cycle(locker);
			break;
		case LOCKER_ACTION_DEACTIVATE:
			ret = _locker_action_deactivate(locker);
			break;
		case LOCKER_ACTION_DISABLE:
			ret = _locker_action_disable(locker);
			break;
		case LOCKER_ACTION_ENABLE:
			ret = _locker_action_enable(locker);
			break;
		case LOCKER_ACTION_LOCK:
			ret = _locker_action_lock(locker);
			break;
		case LOCKER_ACTION_RELOAD:
			ret = _locker_action_reload(locker);
			break;
		case LOCKER_ACTION_SHOW_PREFERENCES:
			locker_show_preferences(locker, TRUE);
			ret = 0;
			break;
		case LOCKER_ACTION_START:
			ret = _locker_action_start(locker);
			break;
		case LOCKER_ACTION_STOP:
			ret = _locker_action_stop(locker);
			break;
		case LOCKER_ACTION_SUSPEND:
			ret = _locker_action_suspend(locker);
			break;
		case LOCKER_ACTION_UNLOCK:
			ret = _locker_action_unlock(locker);
			break;
	}
	return ret;
}

static int _locker_action_activate(Locker * locker)
{
	XForceScreenSaver(GDK_DISPLAY_XDISPLAY(locker->display),
			ScreenSaverActive);
	return 0;
}

static int _locker_action_cycle(Locker * locker)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	_locker_action_activate(locker);
	return _locker_cycle(locker, 1);
}

static int _locker_action_deactivate(Locker * locker)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	XForceScreenSaver(GDK_DISPLAY_XDISPLAY(locker->display),
			ScreenSaverReset);
	return 0;
}

static int _locker_action_disable(Locker * locker)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	_locker_disable(locker);
	return 0;
}

static int _locker_action_enable(Locker * locker)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	_locker_enable(locker);
	return 0;
}

static int _locker_action_lock(Locker * locker)
{
	_locker_action_activate(locker);
	return _locker_lock(locker, 1);
}

static int _locker_action_reload(Locker * locker)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	_locker_demo_reload(locker);
	return 0;
}

static int _locker_action_start(Locker * locker)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	_locker_action_activate(locker);
	_locker_demo_start(locker);
	return 0;
}

static int _locker_action_stop(Locker * locker)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	_locker_demo_stop(locker);
	return 0;
}

static int _locker_action_suspend(Locker * locker)
{
#if defined(__NetBSD__)
	int sleep_state = 3;
#elif defined(__FreeBSD__)
	char * suspend[] = { PREFIX "/bin/sudo", "sudo", "/usr/sbin/zzz",
		NULL };
	GError * error = NULL;
#else
	int fd;
	char * suspend[] = { "/usr/bin/sudo", "sudo", "/usr/bin/apm", "-s",
		NULL };
	GError * error = NULL;
#endif

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(_locker_event(locker, LOCKER_EVENT_SUSPENDING) != 0)
		return -1;
	/* automatically lock the screen when suspending */
	if(_locker_action_lock(locker) != 0)
		return -1;
	/* suspend immediately */
#if defined(__NetBSD__)
	if(sysctlbyname("machdep.sleep_state", NULL, NULL, &sleep_state,
				sizeof(sleep_state)) != 0)
	{
		_locker_error(locker, strerror(errno), 1);
		return -1;
	}
#elif defined(__FreeBSD__)
	if(g_spawn_async(NULL, suspend, NULL, G_SPAWN_FILE_AND_ARGV_ZERO, NULL,
				NULL, NULL, &error) != TRUE)
	{
		_locker_error(locker, error->message, 1);
		g_error_free(error);
		return -1;
	}
#else /* XXX this assumes Linux with sysfs or APM configured */
	if((fd = open("/sys/power/state", O_WRONLY)) >= 0)
	{
		write(fd, "mem\n", 4);
		close(fd);
		return 0;
	}
	if(g_spawn_async(NULL, suspend, NULL, G_SPAWN_FILE_AND_ARGV_ZERO, NULL,
				NULL, NULL, &error) != TRUE)
	{
		_locker_error(locker, error->message, 1);
		g_error_free(error);
		return -1;
	}
#endif
	return 0;
}

static int _locker_action_unlock(Locker * locker)
{
	return _locker_unlock(locker, 1);
}


/* locker_activate */
static int _locker_activate(Locker * locker, int force)
{
	size_t i;
	GdkWindow * window;
	int primary;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(force == 0 && _locker_event(locker, LOCKER_EVENT_ACTIVATING) != 0)
		return -1;
	if(locker->adefinition->action(locker->auth, LOCKER_ACTION_ACTIVATE)
			!= 0)
		return -1;
	for(i = 0; i < locker->windows_cnt; i++)
	{
		gtk_widget_show(locker->windows[i]);
		/* XXX is this really necessary? */
		gtk_window_fullscreen(GTK_WINDOW(locker->windows[i]));
	}
	/* force focus on the primary monitor */
	primary = _locker_get_primary_monitor(locker);
#if GTK_CHECK_VERSION(2, 14, 0)
	window = gtk_widget_get_window(locker->windows[primary]);
#else
	window = locker->windows[primary]->window;
#endif
	gdk_window_focus(window, GDK_CURRENT_TIME);
	_locker_demo_start(locker);
	_locker_event(locker, LOCKER_EVENT_ACTIVATED);
	return 0;
}


/* locker_auth_config_get */
static char const * _locker_auth_config_get(Locker * locker,
		char const * section, char const * variable)
{
	char const * ret;
	String * s;

	if((s = string_new_append("auth::", section, NULL)) == NULL)
		return NULL;
	ret = config_get(locker->config, s, variable);
	string_delete(s);
	return ret;
}


/* locker_auth_config_set */
static int _locker_auth_config_set(Locker * locker, char const * section,
		char const * variable, char const * value)
{
	int ret;
	String * s;

	if((s = string_new_append("auth::", section, NULL)) == NULL)
		return -1;
	ret = config_set(locker->config, s, variable, value);
	string_delete(s);
	if(ret == 0)
		ret = _locker_config_save(locker);
	return ret;
}


/* locker_auth_load */
static GtkWidget * _locker_auth_load(Locker * locker, char const * plugin)
{
	_locker_auth_unload(locker);
	if(plugin == NULL)
		plugin = config_get(locker->config, NULL, "auth");
	if(plugin == NULL)
#ifdef EMBEDDED
		plugin = "slider";
#else
		plugin = "password";
#endif
	if((locker->aplugin = plugin_new(LIBDIR, PACKAGE, "auth", plugin))
			== NULL)
		return NULL;
	if((locker->adefinition = plugin_lookup(locker->aplugin, "plugin"))
			== NULL
			|| locker->adefinition->init == NULL
			|| locker->adefinition->destroy == NULL
			|| locker->adefinition->get_widget == NULL
			|| locker->adefinition->action == NULL
			|| (locker->auth = locker->adefinition->init(
					&locker->ahelper)) == NULL)
	{
		plugin_delete(locker->aplugin);
		locker->adefinition = NULL;
		locker->aplugin = NULL;
		return NULL;
	}
	return locker->adefinition->get_widget(locker->auth);
}


/* locker_auth_unload */
static void _locker_auth_unload(Locker * locker)
{
	if(locker->adefinition != NULL)
		locker->adefinition->destroy(locker->auth);
	locker->auth = NULL;
	if(locker->aplugin != NULL)
		plugin_delete(locker->aplugin);
	locker->adefinition = NULL;
	locker->aplugin = NULL;
}


/* locker_config_load */
static int _locker_config_load(Locker * locker)
{
	int ret;

	if((ret = config_load_preferences(locker->config, LOCKER_CONFIG_VENDOR,
					PACKAGE, LOCKER_CONFIG_FILE)) != 0)
		_locker_error(NULL, error_get(NULL), ret);
	return ret;
}


/* locker_config_save */
static int _locker_config_save(Locker * locker)
{
	int ret;

	if((ret = config_save_preferences_user(locker->config,
					LOCKER_CONFIG_VENDOR, PACKAGE,
					LOCKER_CONFIG_FILE)) != 0)
		_locker_error(NULL, error_get(NULL), 1);
	return ret;
}


/* locker_cycle */
static int _locker_cycle(Locker * locker, int force)
{
	if(force == 0 && _locker_event(locker, LOCKER_EVENT_CYCLING) != 0)
		return -1;
	if(locker->ddefinition != NULL && locker->ddefinition->cycle != NULL)
		locker->ddefinition->cycle(locker->demo);
	_locker_event(locker, LOCKER_EVENT_CYCLED);
	return 0;
}


/* locker_deactivate */
static int _locker_deactivate(Locker * locker, int force)
{
	if(force == 0 && _locker_event(locker, LOCKER_EVENT_DEACTIVATING) != 0)
		return -1;
	if(locker->locked == FALSE)
		_locker_unlock(locker, force);
	else if(locker->adefinition->action(locker->auth,
				LOCKER_ACTION_DEACTIVATE) != 0)
		return -1;
	_locker_demo_stop(locker);
	_locker_event(locker, LOCKER_EVENT_DEACTIVATED);
	return 0;
}


/* locker_demo_config_get */
static char const * _locker_demo_config_get(Locker * locker,
		char const * section, char const * variable)
{
	char const * ret;
	String * s;

	if((s = string_new_append("demo::", section, NULL)) == NULL)
		return NULL;
	ret = config_get(locker->config, s, variable);
	string_delete(s);
	return ret;
}


/* locker_demo_config_set */
static int _locker_demo_config_set(Locker * locker, char const * section,
		char const * variable, char const * value)
{
	int ret;
	String * s;

	if((s = string_new_append("demo::", section, NULL)) == NULL)
		return -1;
	ret = config_set(locker->config, s, variable, value);
	string_delete(s);
	if(ret == 0)
		ret = _locker_config_save(locker);
	return ret;
}


/* locker_demo_load */
static int _locker_demo_load(Locker * locker, char const * demo)
{
	size_t i;
	GdkWindow * window;

	_locker_demo_unload(locker);
	if(demo == NULL && (demo = config_get(locker->config, NULL, "demo"))
			== NULL)
		return 0;
	if((locker->dplugin = plugin_new(LIBDIR, PACKAGE, "demos", demo))
			== NULL)
		return -1;
	if((locker->ddefinition = plugin_lookup(locker->dplugin, "plugin"))
			== NULL
			|| locker->ddefinition->init == NULL
			|| locker->ddefinition->destroy == NULL
			|| (locker->demo = locker->ddefinition->init(
					&locker->dhelper)) == NULL)
	{
		plugin_delete(locker->dplugin);
		locker->ddefinition = NULL;
		locker->dplugin = NULL;
		return -1;
	}
	/* register the existing windows */
	for(i = 0; i < locker->windows_cnt; i++)
	{
		if(locker->windows[i] == NULL)
			continue;
#if GTK_CHECK_VERSION(2, 14, 0)
		window = gtk_widget_get_window(locker->windows[i]);
#else
		window = locker->windows[i]->window;
#endif
		if(window != NULL && locker->ddefinition->add != NULL)
			locker->ddefinition->add(locker->demo, window);
	}
	return 0;
}


/* locker_demo_reload */
static void _locker_demo_reload(Locker * locker)
{
	if(locker->ddefinition != NULL && locker->ddefinition->reload != NULL)
		locker->ddefinition->reload(locker->demo);
}


/* locker_demo_start */
static void _locker_demo_start(Locker * locker)
{
	if(locker->ddefinition != NULL && locker->ddefinition->start != NULL)
		locker->ddefinition->start(locker->demo);
}


/* locker_demo_stop */
static void _locker_demo_stop(Locker * locker)
{
#if GTK_CHECK_VERSION(2, 14, 0) && !GTK_CHECK_VERSION(3, 0, 0)
	GdkWindow * window;
#endif

	if(locker->ddefinition != NULL && locker->ddefinition->stop != NULL)
		locker->ddefinition->stop(locker->demo);
#if GTK_CHECK_VERSION(2, 14, 0) && !GTK_CHECK_VERSION(3, 0, 0)
	if(locker->windows[0] != NULL
			&& (window = gtk_widget_get_window(locker->windows[0]))
			!= NULL)
	{
		gdk_window_clear(window);
		gdk_window_invalidate_rect(window, NULL, TRUE);
	}
#endif
	if(locker->source != 0)
		g_source_remove(locker->source);
	locker->source = 0;
}


/* locker_demo_unload */
static void _locker_demo_unload(Locker * locker)
{
	size_t i;
	GdkWindow * window;

	if(locker->demo == NULL)
		return;
	if(locker->ddefinition != NULL && locker->ddefinition->remove != NULL)
		for(i = 0; i < locker->windows_cnt; i++)
		{
#if GTK_CHECK_VERSION(2, 14, 0)
			window = gtk_widget_get_window(locker->windows[i]);
#else
			window = locker->windows[i]->window;
#endif
			locker->ddefinition->remove(locker->demo, window);
		}
	locker->ddefinition->destroy(locker->demo);
	locker->demo = NULL;
	if(locker->dplugin != NULL)
		plugin_delete(locker->dplugin);
	locker->ddefinition = NULL;
	locker->dplugin = NULL;
}


/* locker_disable */
static void _locker_disable(Locker * locker)
{
	if(!locker->locked)
		locker->enabled = FALSE;
}


/* locker_enable */
static void _locker_enable(Locker * locker)
{
	locker->enabled = TRUE;
}


/* locker_error */
static int _locker_error(Locker * locker, char const * message, int ret)
{
	GtkWidget * dialog;

	if(locker == NULL || _locker_is_locked(locker))
	{
		fprintf(stderr, "%s: %s\n", PROGNAME_LOCKER, message);
		return ret;
	}
	dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE,
#if GTK_CHECK_VERSION(2, 6, 0)
			"%s", _("Error"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
#endif
			"%s", message);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Error"));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	return ret;
}


/* locker_event */
static int _locker_event(Locker * locker, LockerEvent event)
{
	int ret = 0;
	size_t i;
	LockerPluginDefinition * lpd;
	LockerPlugin * lp;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__, event);
#endif
	for(i = 0; i < locker->plugins_cnt; i++)
	{
		lpd = locker->plugins[i].definition;
		lp = locker->plugins[i].plugin;
		if(lpd->event != NULL)
			ret |= lpd->event(lp, event);
	}
	return (ret == 0) ? 0 : -1;
}


/* locker_lock */
static int _locker_lock(Locker * locker, int force)
{
	int ret;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(force == 0 && _locker_event(locker, LOCKER_EVENT_LOCKING) != 0)
		return -1;
	locker->locked = TRUE;
	_locker_activate(locker, 1);
	if((ret = locker->adefinition->action(locker->auth, LOCKER_ACTION_LOCK))
			== 0)
		_locker_event(locker, LOCKER_EVENT_LOCKED);
	return ret;
}


/* plug-ins */
/* locker_plugin_config_get */
static char const * _locker_plugin_config_get(Locker * locker,
		char const * section, char const * variable)
{
	char const * ret;
	String * s;

	if((s = string_new_append("plugin::", section, NULL)) == NULL)
		return NULL;
	ret = config_get(locker->config, s, variable);
	string_delete(s);
	return ret;
}


/* locker_plugin_config_set */
static int _locker_plugin_config_set(Locker * locker, char const * section,
		char const * variable, char const * value)
{
	int ret;
	String * s;

	if((s = string_new_append("plugin::", section, NULL)) == NULL)
		return -1;
	ret = config_set(locker->config, s, variable, value);
	string_delete(s);
	if(ret == 0)
		ret = _locker_config_save(locker);
	return ret;
}


/* locker_plugin_load */
static int _locker_plugin_load(Locker * locker, char const * plugin)
{
	LockerPlugins * p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, plugin);
#endif
	if(_locker_plugin_is_enabled(locker, plugin))
		return 0;
	if((p = realloc(locker->plugins, sizeof(*p) * (locker->plugins_cnt
						+ 1))) == NULL)
		return _locker_error(NULL, strerror(errno), 1);
	locker->plugins = p;
	p = &locker->plugins[locker->plugins_cnt];
	if((p->pplugin = plugin_new(LIBDIR, PACKAGE, "plugins", plugin))
			== NULL)
		return _locker_error(NULL, error_get(NULL), 1);
	if((p->definition = plugin_lookup(p->pplugin, "plugin")) == NULL)
	{
		plugin_delete(p->pplugin);
		return _locker_error(NULL, error_get(NULL), 1);
	}
	p->name = strdup(plugin);
	if(p->definition->init == NULL || p->definition->destroy == NULL
			|| (p->plugin = p->definition->init(&locker->phelper))
			== NULL)
	{
		free(p->name);
		plugin_delete(p->pplugin);
		return _locker_error(NULL, error_get(NULL), 1);
	}
	locker->plugins_cnt++;
	return 0;
}


/* locker_plugin_unload */
static int _locker_plugin_unload(Locker * locker, char const * plugin)
{
	size_t i;
	LockerPlugins * lp;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(\"%s\")\n", __func__, plugin);
#endif
	for(i = 0; i < locker->plugins_cnt; i++)
		if(strcmp(locker->plugins[i].name, plugin) == 0)
			break;
	if(i == locker->plugins_cnt)
		/* this plug-in is not loaded */
		return 0;
	/* unload the plug-in */
	lp = &locker->plugins[i];
	if(lp->definition->destroy != NULL)
		lp->definition->destroy(lp->plugin);
	plugin_delete(lp->pplugin);
	free(lp->name);
	memmove(lp, &lp[1], sizeof(*lp) * (--locker->plugins_cnt - i));
	/* FIXME should call realloc() to gain some memory */
	return 0;
}


/* locker_unlock */
static int _locker_unlock(Locker * locker, int force)
{
	size_t i;
#if GTK_CHECK_VERSION(3, 0, 0)
	GdkWindow * window;
	GdkDisplay * display;
	GdkDeviceManager * manager;
	GdkDevice * pointer;
	GdkDevice * keyboard;
#endif

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(force == 0 && _locker_event(locker, LOCKER_EVENT_UNLOCKING) != 0)
		return -1;
	if(locker->adefinition->action(locker->auth, LOCKER_ACTION_UNLOCK) != 0)
		return -1;
	_locker_event(locker, LOCKER_EVENT_UNLOCKED);
	locker->locked = FALSE;
	/* ungrab keyboard and mouse */
	if(locker->windows == NULL)
		return 0;
#if GTK_CHECK_VERSION(3, 0, 0)
	/* FIXME untested */
	window = gtk_widget_get_window(locker->windows[0]);
	display = gdk_window_get_display(window);
	manager = gdk_display_get_device_manager(display);
	pointer = gdk_device_manager_get_client_pointer(manager);
	keyboard = gdk_device_get_associated_device(pointer);
	gdk_device_ungrab(keyboard, GDK_CURRENT_TIME);
	gdk_device_ungrab(pointer, GDK_CURRENT_TIME);
#else
	gdk_keyboard_ungrab(GDK_CURRENT_TIME);
	gdk_pointer_ungrab(GDK_CURRENT_TIME);
#endif
	for(i = 0; i < locker->windows_cnt; i++)
		gtk_widget_hide(locker->windows[i]);
	return 0;
}


/* locker_window_register */
static gboolean _window_register_contained(Locker * locker, size_t primary,
		size_t i);

static void _locker_window_register(Locker * locker, size_t i)
{
#if GTK_CHECK_VERSION(3, 0, 0)
	GdkRGBA black;
#else
	GdkColor black;
#endif
	size_t primary;

	primary = _locker_get_primary_monitor(locker);
	if(_window_register_contained(locker, primary, i))
	{
		/* a clone was detected */
		locker->windows[i] = NULL;
		return;
	}
	memset(&black, 0, sizeof(black));
	locker->windows[i] = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_focus_on_map(GTK_WINDOW(locker->windows[i]), (i == 0)
			? TRUE : FALSE);
	gtk_window_set_keep_above(GTK_WINDOW(locker->windows[i]), TRUE);
	gtk_window_stick(GTK_WINDOW(locker->windows[i]));
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_widget_override_background_color(locker->windows[i],
			GTK_STATE_NORMAL, &black);
#else
	gtk_widget_modify_bg(locker->windows[i], GTK_STATE_NORMAL, &black);
#endif
	g_signal_connect(locker->windows[i], "configure-event", G_CALLBACK(
				_locker_on_configure), locker);
	g_signal_connect_swapped(locker->windows[i], "delete-event",
			G_CALLBACK(_locker_on_closex), NULL);
	/* automatically grab keyboard and mouse */
	g_signal_connect(locker->windows[i], "map-event", G_CALLBACK(
				_locker_on_map_event), locker);
	g_signal_connect(locker->windows[i], "realize", G_CALLBACK(
				_locker_on_realize), locker);
	_locker_on_configure(locker->windows[i], NULL, locker);
}

static gboolean _window_register_contained(Locker * locker, size_t primary,
		size_t i)
{
	size_t j;
	GdkRectangle irect;
	GdkRectangle jrect;
#if GTK_CHECK_VERSION(3, 22, 0)
	GdkMonitor * monitor;
#else
	GdkScreen * screen;
	(void) locker;
#endif

	/* obtain the monitor geometry */
#if GTK_CHECK_VERSION(3, 22, 0)
	if((monitor = gdk_display_get_monitor(locker->display, i)) == NULL)
		return FALSE;
	gdk_monitor_get_geometry(monitor, &irect);
#else
	screen = gdk_screen_get_default();
	gdk_screen_get_monitor_geometry(screen, i, &irect);
#endif
	for(j = 0; j < i; j++)
	{
		if(j == primary)
			continue;
		/* compare with previous monitors */
#if GTK_CHECK_VERSION(3, 22, 0)
		if((monitor = gdk_display_get_monitor(locker->display, j))
				== NULL)
			continue;
		gdk_monitor_get_geometry(monitor, &jrect);
#else
		gdk_screen_get_monitor_geometry(screen, j, &jrect);
#endif
		if(irect.x >= jrect.x && irect.y >= jrect.y
				&& ((irect.x + irect.width)
					<= (jrect.x + jrect.width))
				&& ((irect.y + irect.height)
					<= (jrect.y + jrect.height)))
			/* a container was already registered */
			return TRUE;
	}
	return FALSE;
}


/* callbacks */
/* locker_on_closex */
static gboolean _locker_on_closex(void)
{
	return TRUE;
}


/* locker_on_configure */
static gboolean _locker_on_configure(GtkWidget * widget, GdkEvent * event,
		gpointer data)
{
	Locker * locker = data;
	size_t i;
#if GTK_CHECK_VERSION(3, 22, 0)
	GdkMonitor * monitor;
#else
	GdkScreen * screen;
#endif
	GdkRectangle rect;
	(void) event;

	/* detect the window affected */
	for(i = 0; i < locker->windows_cnt; i++)
		if(locker->windows[i] == widget)
			break;
	if(i == locker->windows_cnt)
		return FALSE;
	/* configure the window */
#if GTK_CHECK_VERSION(3, 22, 0)
	if((monitor = gdk_display_get_monitor(locker->display, i)) == NULL)
		/* XXX report the error */
		return FALSE;
	gdk_monitor_get_geometry(monitor, &rect);
#else
	screen = gdk_screen_get_default();
	gdk_screen_get_monitor_geometry(screen, i, &rect);
#endif
	gtk_window_move(GTK_WINDOW(locker->windows[i]), rect.x, rect.y);
	gtk_window_resize(GTK_WINDOW(locker->windows[i]), rect.width,
			rect.height);
	return FALSE;
}


/* locker_on_filter */
static GdkFilterReturn _filter_configure(Locker * locker);
static GdkFilterReturn _filter_xscreensaver_notify(Locker * locker,
		XScreenSaverNotifyEvent * xssne);
static void _filter_xscreensaver_notify_on(Locker * locker);

static GdkFilterReturn _locker_on_filter(GdkXEvent * xevent, GdkEvent * event,
		gpointer data)
{
	Locker * locker = data;
	XEvent * xev = xevent;
	(void) event;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() 0x%x 0x%x\n", __func__, xev->type,
			event->type);
#endif
	if(xev->type == locker->event)
		return _filter_xscreensaver_notify(locker, xevent);
	else if(xev->type == ConfigureNotify)
		return _filter_configure(locker);
	else
		return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn _filter_configure(Locker * locker)
{
#if !GTK_CHECK_VERSION(3, 22, 0)
	GdkScreen * screen;
#endif
	size_t cnt;
	size_t i;
	GdkWindow * window;
	GtkWidget ** p;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	/* XXX assume at least one monitor */
#if GTK_CHECK_VERSION(3, 22, 0)
	if((cnt = gdk_display_get_n_monitors(locker->display)) < 1)
		cnt = 1;
#else
	screen = gdk_screen_get_default();
	if((cnt = gdk_screen_get_n_monitors(screen)) < 1)
		cnt = 1;
#endif
	for(i = 0; i < locker->windows_cnt && i < cnt; i++)
		if(locker->windows[i] == NULL)
			_locker_window_register(locker, i);
	if(i == cnt)
		/* remove obsoleted windows */
		for(; i < locker->windows_cnt; i++)
		{
			if(locker->windows[i] == NULL)
				continue;
#if GTK_CHECK_VERSION(2, 14, 0)
			window = gtk_widget_get_window(locker->windows[i]);
#else
			window = locker->windows[i]->window;
#endif
			if(locker->ddefinition != NULL
					&& locker->ddefinition->remove != NULL)
				locker->ddefinition->remove(locker->demo,
						window);
			gtk_widget_destroy(locker->windows[i]);
			locker->windows[i] = NULL;
		}
	else if(i == locker->windows_cnt)
	{
		/* add new windows */
		if((p = realloc(locker->windows, sizeof(*p) * cnt)) == NULL)
			/* XXX report the error */
			return GDK_FILTER_CONTINUE;
		locker->windows = p;
		for(; i < cnt; i++)
			_locker_window_register(locker, i);
	}
	return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn _filter_xscreensaver_notify(Locker * locker,
		XScreenSaverNotifyEvent * xssne)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() %u\n", __func__, xssne->state);
#endif
	switch(xssne->state)
	{
		case ScreenSaverOff:
			_locker_enable(locker);
			if(_locker_deactivate(locker, 0) != 0)
				XActivateScreenSaver(xssne->display);
			break;
		case ScreenSaverOn:
			_filter_xscreensaver_notify_on(locker);
			break;
		case ScreenSaverCycle:
			_locker_cycle(locker, 0);
			break;
		case ScreenSaverDisabled:
			_locker_disable(locker);
			break;
	}
	return GDK_FILTER_CONTINUE;
}

static void _filter_xscreensaver_notify_on(Locker * locker)
{
	char const * p;
	long delay;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	_locker_enable(locker);
	if(locker->locked != FALSE)
		/* we are already locked */
		return;
	_locker_activate(locker, 0);
	if((p = config_get(locker->config, NULL, "lock")) == NULL)
		/* do not lock at all */
		return;
	if((delay = strtol(p, NULL, 10)) <= 0)
	{
		/* lock immediately */
		_locker_lock(locker, 0);
		return;
	}
	/* lock after a delay */
	if(locker->source != 0)
		g_source_remove(locker->source);
	locker->source = g_timeout_add(delay * 1000, _locker_on_lock, locker);
}


/* locker_on_lock */
static gboolean _locker_on_lock(gpointer data)
{
	Locker * locker = data;

	locker->source = 0;
	_locker_lock(locker, 0);
	return FALSE;
}


/* locker_on_map_event */
static gboolean _locker_on_map_event(GtkWidget * widget, GdkEvent * event,
		gpointer data)
{
	Locker * locker = data;
	size_t primary;
	GdkWindow * window;
	GdkGrabStatus status;
#if GTK_CHECK_VERSION(3, 0, 0)
	GdkDisplay * display;
	GdkDeviceManager * manager;
	GdkDevice * pointer;
	GdkDevice * keyboard;
#endif
	(void) event;

	/* detect if this is the primary window */
	primary = _locker_get_primary_monitor(locker);
	if(locker->windows[primary] != widget)
		return FALSE;
	/* FIXME the mouse may already be grabbed (Panel's lock button...) */
	/* grab keyboard and mouse */
#if GTK_CHECK_VERSION(2, 14, 0)
	window = gtk_widget_get_window(locker->windows[primary]);
#else
	window = locker->windows[primary]->window;
#endif
	if(window == NULL)
		_locker_error(NULL, "Failed to grab input", 1);
	else
	{
#if GTK_CHECK_VERSION(3, 0, 0)
		/* FIXME untested */
		display = gdk_window_get_display(window);
		manager = gdk_display_get_device_manager(display);
		pointer = gdk_device_manager_get_client_pointer(manager);
		keyboard = gdk_device_get_associated_device(pointer);
		if((status = gdk_device_grab(keyboard, window,
						GDK_OWNERSHIP_WINDOW, FALSE, 0,
						NULL, GDK_CURRENT_TIME))
				!= GDK_GRAB_SUCCESS)
			_locker_error(NULL, "Failed to grab keyboard", 1);
# ifdef DEBUG
		fprintf(stderr, "DEBUG: keyboard grab status=%u\n", status);
# endif
		if((status = gdk_device_grab(pointer, window,
						GDK_OWNERSHIP_WINDOW, FALSE, 0,
						NULL, GDK_CURRENT_TIME))
				!= GDK_GRAB_SUCCESS)
			_locker_error(NULL, "Failed to grab mouse", 1);
#else
		if((status = gdk_keyboard_grab(window, TRUE, GDK_CURRENT_TIME))
				!= GDK_GRAB_SUCCESS)
			_locker_error(NULL, "Failed to grab keyboard", 1);
# ifdef DEBUG
		fprintf(stderr, "DEBUG: keyboard grab status=%u\n", status);
# endif
		if((status = gdk_pointer_grab(window, TRUE, 0, window, NULL,
						GDK_CURRENT_TIME))
				!= GDK_GRAB_SUCCESS)
			_locker_error(NULL, "Failed to grab mouse", 1);
#endif
#ifdef DEBUG
		fprintf(stderr, "DEBUG: mouse grab status=%u\n", status);
#endif
	}
	return FALSE;
}


/* locker_on_message */
static int _locker_on_message(void * data, uint32_t value1, uint32_t value2,
		uint32_t value3)
{
	Locker * locker = data;
	LockerAction action;
	gboolean show;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u, %u, %u)\n", __func__, value1, value2,
			value3);
#endif
	if(value1 != LOCKER_MESSAGE_ACTION)
		return 0;
	switch((action = value2))
	{
		case LOCKER_ACTION_SHOW_PREFERENCES:
			show = value3 ? TRUE : FALSE;
			locker_show_preferences(locker, show);
			break;
		default:
			_locker_action(locker, action);
			break;
	}
	return 0;
}


/* locker_on_preferences_general_dpms_toggled */
static void _locker_on_preferences_general_dpms_toggled(gpointer data)
{
	Locker * locker = data;
	gboolean active;

	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
				locker->pr_gdenabled));
	gtk_widget_set_sensitive(locker->pr_gdpms1, active);
	gtk_widget_set_sensitive(locker->pr_gdpms2, active);
	gtk_widget_set_sensitive(locker->pr_gdpms3, active);
}


/* locker_on_preferences_general_toggled */
static void _locker_on_preferences_general_toggled(gpointer data)
{
	Locker * locker = data;
	gboolean active;

	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
				locker->pr_genabled));
	gtk_widget_set_sensitive(locker->pr_gtimeout, active);
}


/* locker_on_preferences_lock_toggled */
static void _locker_on_preferences_lock_toggled(gpointer data)
{
	Locker * locker = data;
	gboolean active;

	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
				locker->pr_alock));
	gtk_widget_set_sensitive(locker->pr_adelay, active);
	gtk_widget_set_sensitive(locker->pr_acombo, active);
}


/* locker_on_realize */
static void _locker_on_realize(GtkWidget * widget, gpointer data)
{
	Locker * locker = data;
	GdkWindow * window;
	size_t primary;
	int x;
	int y;
	int width;
	int height;
	int depth;
	GdkVisual * visual;
	int i;

#if GTK_CHECK_VERSION(2, 14, 0)
	window = gtk_widget_get_window(widget);
#else
	window = widget->window;
#endif
	if(locker->ddefinition != NULL && locker->ddefinition->add != NULL)
		locker->ddefinition->add(locker->demo, window);
	primary = _locker_get_primary_monitor(locker);
	if(widget == locker->windows[primary])
	{
		window = gtk_widget_get_window(widget);
		visual = gdk_window_get_visual(window);
#if GTK_CHECK_VERSION(3, 0, 0)
		gdk_window_get_geometry(window, &x, &y, &width, &height);
		depth = gdk_visual_get_depth(visual);
#else
		gdk_window_get_geometry(window, &x, &y, &width, &height,
				&depth);
#endif
		gdk_x11_display_error_trap_push(locker->display);
		XScreenSaverSetAttributes(GDK_DISPLAY_XDISPLAY(locker->display),
				GDK_WINDOW_XID(window), x, y, width, height, 0,
				depth, 0, GDK_VISUAL_XVISUAL(visual), 0, NULL);
		if((i = gdk_x11_display_error_trap_pop(locker->display)) != 0)
		{
			error_set_code(i, "%s",
					"Could not set screensaver attributes");
			_locker_error(NULL, error_get(NULL), i);
		}
	}
}
