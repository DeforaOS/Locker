/* $Id$ */
static char const _copyright[] =
"Copyright (c) 2010-2012 Pierre Pronchery <khorben@defora.org>";
/* This file is part of DeforaOS Desktop Locker */
static char const _license[] =
"This program is free software: you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation, version 3 of the License.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program.  If not, see <http://www.gnu.org/licenses/>.";



#if defined(__NetBSD__)
# include <sys/param.h>
# include <sys/sysctl.h>
#elif defined(__linux__)
# include <fcntl.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libintl.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#include <X11/extensions/scrnsaver.h>
#include <System.h>
#include <Desktop.h>
#include "locker.h"
#include "../config.h"
#define _(string) gettext(string)

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))


/* Locker */
/* private */
/* types */
typedef struct _LockerPlugins
{
	Plugin * pplugin;
	LockerPluginDefinition * definition;
	LockerPlugin * plugin;
} LockerPlugins;

struct _Locker
{
	/* settings */
	int suspend;
	Config * config;

	/* internal */
	int enabled;
	int locked;

	GdkDisplay * display;
	int screen;
	int event;
	GtkWidget ** windows;
	size_t windows_cnt;

	/* auth */
	Plugin * aplugin;
	LockerAuthDefinition * adefinition;
	LockerAuth * auth;
	LockerAuthHelper ahelper;

	/* demo */
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
	GtkListStore * pr_plstore;
	GtkWidget * pr_plview;

	/* about */
	GtkWidget * ab_window;
};

typedef enum _LockerPluginsColumn
{
	LOCKER_PLUGINS_COLUMN_PLUGIN = 0,
	LOCKER_PLUGINS_COLUMN_ENABLED,
	LOCKER_PLUGINS_COLUMN_FILENAME,
	LOCKER_PLUGINS_COLUMN_ICON,
	LOCKER_PLUGINS_COLUMN_NAME
} LockerPluginsColumn;
#define LOCKER_PLUGINS_COLUMN_LAST LOCKER_PLUGINS_COLUMN_NAME
#define LOCKER_PLUGINS_COLUMN_COUNT (LOCKER_PLUGINS_COLUMN_LAST + 1)


/* constants */
#define LOCKER_CONFIG_FILE	".locker"

static char const * _authors[] =
{
	"Pierre Pronchery <khorben@defora.org>",
	NULL
};


/* prototypes */
/* accessors */
static int _locker_is_locked(Locker * locker);

/* useful */
static void _locker_about(Locker * locker);

/* actions */
static int _locker_action(Locker * locker, LockerAction action);
static int _locker_action_activate(Locker * locker, int force);
static int _locker_action_deactivate(Locker * locker, int reset);
static int _locker_action_disable(Locker * locker);
static int _locker_action_enable(Locker * locker);
static int _locker_action_lock(Locker * locker, int force);
static int _locker_action_start(Locker * locker);
static int _locker_action_stop(Locker * locker);
static int _locker_action_suspend(Locker * locker);
static int _locker_action_unlock(Locker * locker);

/* auth */
static char const * _locker_auth_config_get(Locker * locker,
		char const * section, char const * variable);
static int _locker_auth_config_set(Locker * locker, char const * section,
		char const * variable, char const * value);

/* demo */
static char const * _locker_demo_config_get(Locker * locker,
		char const * section, char const * variable);
static int _locker_demo_config_set(Locker * locker, char const * section,
		char const * variable, char const * value);

static int _locker_error(Locker * locker, char const * message, int ret);
static int _locker_event(Locker * locker, LockerEvent event);

/* plug-ins */
static char const * _locker_plugin_config_get(Locker * locker,
		char const * section, char const * variable);
static int _locker_plugin_config_set(Locker * locker, char const * section,
		char const * variable, char const * value);

/* callbacks */
static gboolean _lock_on_closex(void);
static GdkFilterReturn _locker_on_filter(GdkXEvent * xevent, GdkEvent * event,
		gpointer data);
static gboolean _locker_on_map_event(gpointer data);
static int _locker_on_message(void * data, uint32_t value1, uint32_t value2,
		uint32_t value3);
static void _locker_on_realize(GtkWidget * widget, gpointer data);


/* public */
/* functions */
/* locker_new */
static int _new_config(Locker * locker);
static int _new_demo(Locker * locker, char const * demo);
static void _new_helpers(Locker * locker);
static GtkWidget * _new_auth(Locker * locker, char const * plugin);
static int _new_plugins(Locker * locker);
static int _new_plugins_load(Locker * locker, char const * plugin);
static int _new_xss(Locker * locker, size_t cnt);

Locker * locker_new(int suspend, char const * demo, char const * auth)
{
	Locker * locker;
	GdkScreen * screen;
	GtkWidget * widget = NULL;
	size_t cnt;
	size_t i;
	GdkColor black;
	GdkRectangle rect;
	GdkWindow * root;

	if((locker = object_new(sizeof(*locker))) == NULL)
	{
		_locker_error(NULL, error_get(), 1);
		return NULL;
	}
	_new_helpers(locker);
	locker->suspend = (suspend != 0) ? 1 : 0;
	locker->enabled = 1;
	locker->locked = 0;
	screen = gdk_screen_get_default();
	locker->display = gdk_screen_get_display(screen);
	locker->screen = gdk_x11_get_default_screen();
	if((cnt = gdk_screen_get_n_monitors(screen)) < 1)
		cnt = 1;
	locker->windows = NULL;
	locker->windows_cnt = cnt;
	locker->dplugin = NULL;
	locker->demo = NULL;
	locker->plugins = NULL;
	locker->plugins_cnt = 0;
	locker->pr_window = NULL;
	locker->ab_window = NULL;
	/* check for errors */
	if(_new_config(locker) != 0
			|| _new_demo(locker, demo) != 0
			|| (widget = _new_auth(locker, auth)) == NULL
			|| _new_xss(locker, cnt) != 0)
	{
		_locker_error(NULL, error_get(), 1);
		locker_delete(locker);
		return NULL;
	}
	_new_plugins(locker);
	/* create windows */
	memset(&black, 0, sizeof(black));
	for(i = 0; i < locker->windows_cnt; i++)
	{
		locker->windows[i] = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		gdk_screen_get_monitor_geometry(screen, i, &rect);
		gtk_window_move(GTK_WINDOW(locker->windows[i]), rect.x, rect.y);
		gtk_window_resize(GTK_WINDOW(locker->windows[i]), rect.width,
				rect.height);
		gtk_window_set_keep_above(GTK_WINDOW(locker->windows[i]), TRUE);
		gtk_window_stick(GTK_WINDOW(locker->windows[i]));
		gtk_widget_modify_bg(locker->windows[i], GTK_STATE_NORMAL,
				&black);
		g_signal_connect_swapped(G_OBJECT(locker->windows[i]),
				"delete-event", G_CALLBACK(_lock_on_closex),
				NULL);
		g_signal_connect(locker->windows[i], "realize", G_CALLBACK(
					_locker_on_realize), locker);
	}
	/* automatically grab keyboard and mouse */
	g_signal_connect_swapped(G_OBJECT(locker->windows[0]), "map-event",
			G_CALLBACK(_locker_on_map_event), locker);
	/* pack the authentication widget */
	gtk_container_add(GTK_CONTAINER(locker->windows[0]), widget);
	root = gdk_get_default_root_window();
	XScreenSaverSelectInput(GDK_DISPLAY_XDISPLAY(locker->display),
			GDK_WINDOW_XWINDOW(root), ScreenSaverNotifyMask);
	gdk_x11_register_standard_event_type(locker->display, locker->event, 1);
	gdk_window_add_filter(root, _locker_on_filter, locker);
	desktop_message_register(LOCKER_CLIENT_MESSAGE, _locker_on_message,
			locker);
	return locker;
}

static int _new_config(Locker * locker)
{
	char const * homedir;
	char * filename;

	if((locker->config = config_new()) == NULL)
		return -1;
	if((homedir = getenv("HOME")) == NULL)
		homedir = g_get_home_dir();
	if((filename = malloc(strlen(homedir) + sizeof(LOCKER_CONFIG_FILE) + 1))
			== NULL)
		return -1;
	sprintf(filename, "%s/%s", homedir, LOCKER_CONFIG_FILE);
	if(config_load(locker->config, filename) != 0)
		_locker_error(NULL, error_get(), 1);
	free(filename);
	return 0;
}

static GtkWidget * _new_auth(Locker * locker, char const * plugin)
{
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
			== NULL)
		return NULL;
	if(locker->adefinition->init == NULL
			|| locker->adefinition->destroy == NULL
			|| locker->adefinition->get_widget == NULL
			|| locker->adefinition->action == NULL
			|| (locker->auth = locker->adefinition->init(
					&locker->ahelper)) == NULL)
		return NULL;
	return locker->adefinition->get_widget(locker->auth);
}

static int _new_demo(Locker * locker, char const * demo)
{
	if(demo == NULL && (demo = config_get(locker->config, NULL, "demo"))
			== NULL)
		return 0;
	if((locker->dplugin = plugin_new(LIBDIR, PACKAGE, "demos", demo))
			== NULL)
		return -1;
	if((locker->ddefinition = plugin_lookup(locker->dplugin, "demo"))
			== NULL)
		return -1;
	if(locker->ddefinition->init == NULL
			|| locker->ddefinition->destroy == NULL
			|| (locker->demo = locker->ddefinition->init(
					&locker->dhelper)) == NULL)
		return -1;
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
	/* demo helper */
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
		return 0;
	if((plugins = string_new(p)) == NULL)
		return -1;
	for(i = 0;; i++)
	{
		if(plugins[i] != ',' && plugins[i] != '\0')
			continue;
		c = plugins[i];
		plugins[i] = '\0';
		ret |= _new_plugins_load(locker, plugins);
		if(c == '\0')
			break;
		plugins += i + 1;
		i = 0;
	}
	string_delete(plugins);
	return ret;
}

static int _new_plugins_load(Locker * locker, char const * plugin)
{
	LockerPlugins * p;

	if((p = realloc(locker->plugins, sizeof(*p) * (locker->plugins_cnt
						+ 1))) == NULL)
		return _locker_error(NULL, strerror(errno), 1);
	locker->plugins = p;
	p = &locker->plugins[locker->plugins_cnt];
	if((p->pplugin = plugin_new(LIBDIR, PACKAGE, "plugins", plugin))
			== NULL)
		return _locker_error(NULL, error_get(), 1);
	if((p->definition = plugin_lookup(p->pplugin, "plugin")) == NULL)
	{
		plugin_delete(p->pplugin);
		return _locker_error(NULL, error_get(), 1);
	}
	if(p->definition->init == NULL || p->definition->destroy == NULL
			|| (p->plugin = p->definition->init(&locker->phelper))
			== NULL)
	{
		plugin_delete(p->pplugin);
		return _locker_error(NULL, error_get(), 1);
	}
	locker->plugins_cnt++;
	return 0;
}

static int _new_xss(Locker * locker, size_t cnt)
{
	int error;

	/* register as screensaver */
	if(XScreenSaverQueryExtension(GDK_DISPLAY_XDISPLAY(locker->display),
				&locker->event, &error) == 0
			|| XScreenSaverRegister(GDK_DISPLAY_XDISPLAY(
					locker->display), locker->screen,
				getpid(), XA_INTEGER) == 0
			|| (locker->windows = malloc(sizeof(*locker->windows)
					* cnt)) == NULL)
		return -error_set_code(1, "%s", _(
					"Could not register as screensaver"));
	return 0;
}


/* locker_delete */
void locker_delete(Locker * locker)
{
	size_t i;

	/* FIXME also destroy plug-ins */
	if(locker->adefinition != NULL)
		locker->adefinition->destroy(locker->auth);
	if(locker->aplugin != NULL)
		plugin_delete(locker->aplugin);
	if(locker->demo != NULL)
	{
		if(locker->ddefinition->remove != NULL)
			for(i = 0; i < locker->windows_cnt; i++)
				locker->ddefinition->remove(locker->demo,
						locker->windows[i]);
		locker->ddefinition->destroy(locker->demo);
	}
	if(locker->dplugin != NULL)
		plugin_delete(locker->dplugin);
	if(locker->ab_window != NULL)
		gtk_widget_destroy(locker->ab_window);
	free(locker->windows);
	XScreenSaverUnregister(GDK_DISPLAY_XDISPLAY(locker->display),
			locker->screen);
	if(locker->config != NULL)
		config_delete(locker->config);
	object_delete(locker);
}


/* useful */
/* locker_show_preferences */
static void _preferences_window(Locker * locker);
static GtkWidget * _preferences_window_plugins(Locker * locker);
/* callbacks */
static void _preferences_on_cancel(gpointer data);
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
	if(locker->pr_window != NULL)
	{
		if(show)
			gtk_window_present(GTK_WINDOW(locker->pr_window));
		else
			gtk_widget_hide(locker->pr_window);
		return;
	}
}

static void _preferences_window(Locker * locker)
{
	GtkWidget * vbox;
	GtkWidget * notebook;
	GtkWidget * widget;

	locker->pr_window = gtk_dialog_new_with_buttons(
			_("Screensaver preferences"), NULL, 0,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_window_set_default_size(GTK_WINDOW(locker->pr_window), 400, 300);
	gtk_window_set_position(GTK_WINDOW(locker->pr_window),
			GTK_WIN_POS_CENTER_ALWAYS);
	g_signal_connect_swapped(locker->pr_window, "delete-event", G_CALLBACK(
				_preferences_on_closex), locker);
	g_signal_connect(locker->pr_window, "response", G_CALLBACK(
				_preferences_on_response), locker);
	notebook = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
	/* authentication */
	/* FIXME implement */
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), gtk_vbox_new(FALSE, 0),
			gtk_label_new(_("Authentication")));
	/* demos */
	/* FIXME implement */
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), gtk_vbox_new(FALSE, 0),
			gtk_label_new(_("Demos")));
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

static GtkWidget * _preferences_window_plugins(Locker * locker)
{
	GtkWidget * vbox;
	GtkWidget * widget;
	GtkCellRenderer * renderer;
	GtkTreeViewColumn * column;

	vbox = gtk_vbox_new(FALSE, 0);
	widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(widget),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(widget),
			GTK_SHADOW_ETCHED_IN);
	locker->pr_plstore = gtk_list_store_new(LOCKER_PLUGINS_COLUMN_COUNT,
			G_TYPE_POINTER, G_TYPE_BOOLEAN, G_TYPE_STRING,
			GDK_TYPE_PIXBUF, G_TYPE_STRING);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(
				locker->pr_plstore),
			LOCKER_PLUGINS_COLUMN_NAME, GTK_SORT_ASCENDING);
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
			renderer, "active", LOCKER_PLUGINS_COLUMN_ENABLED,
			NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(locker->pr_plview), column);
	/* icon */
	renderer = gtk_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer,
			"pixbuf", LOCKER_PLUGINS_COLUMN_ICON, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(locker->pr_plview), column);
	/* name */
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer,
			"text", LOCKER_PLUGINS_COLUMN_NAME, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(locker->pr_plview), column);
	gtk_container_add(GTK_CONTAINER(widget), locker->pr_plview);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
	return vbox;
}

static void _preferences_on_cancel(gpointer data)
{
	Locker * locker = data;
	GtkIconTheme * theme;
	DIR * dir;
	struct dirent * de;
	char const ext[] = ".so";
	size_t len;
	Plugin * p;
	LockerPluginDefinition * lpd;
	GtkTreeIter iter;
	GdkPixbuf * icon;

	gtk_widget_hide(locker->pr_window);
	theme = gtk_icon_theme_get_default();
	/* plug-ins */
	gtk_list_store_clear(locker->pr_plstore);
	if((dir = opendir(LIBDIR "/" PACKAGE "/plugins")) != NULL)
	{
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
			if((p = plugin_new(LIBDIR, PACKAGE, "plugins",
							de->d_name)) == NULL)
				continue;
			if((lpd = plugin_lookup(p, "plugin")) == NULL)
			{
				plugin_delete(p);
				continue;
			}
			gtk_list_store_append(locker->pr_plstore, &iter);
			gtk_list_store_set(locker->pr_plstore, &iter,
					LOCKER_PLUGINS_COLUMN_FILENAME,
					de->d_name, LOCKER_PLUGINS_COLUMN_NAME,
					lpd->name, -1);
			/* FIXME check if it is already enabled */
			icon = NULL;
			if(lpd->icon != NULL)
				icon = gtk_icon_theme_load_icon(theme,
						lpd->icon, 24, 0, NULL);
			if(icon == NULL)
				icon = gtk_icon_theme_load_icon(theme,
						"gnome-settings", 24, 0, NULL);
			gtk_list_store_set(locker->pr_plstore, &iter,
					LOCKER_PLUGINS_COLUMN_ICON, icon, -1);
			plugin_delete(p);
		}
		closedir(dir);
	}
}

static gboolean _preferences_on_closex(gpointer data)
{
	Locker * locker = data;

	gtk_widget_hide(locker->pr_window);
	return TRUE;
}

static void _preferences_on_ok(gpointer data)
{
	/* FIXME implement */
}

static void _preferences_on_plugins_toggled(GtkCellRendererToggle * renderer,
		char * path, gpointer data)
{
	Locker * locker = data;
	GtkTreeIter iter;

	gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(locker->pr_plstore),
			&iter, path);
	gtk_list_store_set(locker->pr_plstore, &iter,
			LOCKER_PLUGINS_COLUMN_ENABLED,
			!gtk_cell_renderer_toggle_get_active(renderer), -1);
}

static void _preferences_on_response(GtkWidget * widget, gint response,
		gpointer data)
{
	gtk_widget_hide(widget);
	if(response == GTK_RESPONSE_OK)
		_preferences_on_ok(data);
	else if(response == GTK_RESPONSE_CANCEL)
		_preferences_on_cancel(data);
}


/* private */
/* functions */
/* accessors */
/* locker_is_locked */
static int _locker_is_locked(Locker * locker)
{
	return locker->locked;
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
static int _locker_action(Locker * locker, LockerAction action)
{
	int ret = -1;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s(%u)\n", __func__, action);
#endif
	switch(action)
	{
		case LOCKER_ACTION_ACTIVATE:
			ret = _locker_action_activate(locker, 1);
			break;
		case LOCKER_ACTION_DEACTIVATE:
			ret = _locker_action_deactivate(locker, 1);
			break;
		case LOCKER_ACTION_DISABLE:
			ret = _locker_action_disable(locker);
			break;
		case LOCKER_ACTION_ENABLE:
			ret = _locker_action_enable(locker);
			break;
		case LOCKER_ACTION_LOCK:
			ret = _locker_action_lock(locker, 1);
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


/* locker_action_activate */
static int _locker_action_activate(Locker * locker, int force)
{
	size_t i;

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
		gtk_window_fullscreen(GTK_WINDOW(locker->windows[i]));
	}
	_locker_action_start(locker);
	return 0;
}


/* locker_action_deactivate */
static int _locker_action_deactivate(Locker * locker, int reset)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(_locker_event(locker, LOCKER_EVENT_DEACTIVATING) != 0)
		return -1;
	if(locker->adefinition->action(locker->auth, LOCKER_ACTION_DEACTIVATE)
			!= 0)
		return -1;
	_locker_action_stop(locker);
	if(reset != 0)
		XResetScreenSaver(GDK_DISPLAY_XDISPLAY(locker->display));
	return 0;
}


/* locker_action_disable */
static int _locker_action_disable(Locker * locker)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(locker->locked)
		return 0;
	locker->enabled = 0;
	return 0;
}


/* locker_action_enable */
static int _locker_action_enable(Locker * locker)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	locker->enabled = 1;
	return 0;
}


/* locker_action_lock */
static int _locker_action_lock(Locker * locker, int force)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(force == 0 && _locker_event(locker, LOCKER_EVENT_LOCKING) != 0)
		return -1;
	locker->locked = 1;
	_locker_action_activate(locker, 1);
	return locker->adefinition->action(locker->auth, LOCKER_ACTION_LOCK);
}


/* locker_action_start */
static int _locker_action_start(Locker * locker)
{
#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	XActivateScreenSaver(GDK_DISPLAY_XDISPLAY(locker->display));
	if(locker->ddefinition->start != NULL)
		locker->ddefinition->start(locker->demo);
	return 0;
}


/* locker_action_stop */
static int _locker_action_stop(Locker * locker)
{
	GdkWindow * window;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(locker->ddefinition->stop != NULL)
		locker->ddefinition->stop(locker->demo);
#if GTK_CHECK_VERSION(2, 14, 0)
	if(locker->windows[0] != NULL)
	{
		window = gtk_widget_get_window(locker->windows[0]);
		gdk_window_clear(window);
	}
#endif
	return 0;
}


/* locker_action_suspend */
static int _locker_action_suspend(Locker * locker)
{
#if defined(__NetBSD__)
	int sleep_state = 3;
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
	if(_locker_action_lock(locker, 1) != 0)
		return -1;
	/* suspend immediately */
#if defined(__NetBSD__)
	if(sysctlbyname("machdep.sleep_state", NULL, NULL, &sleep_state,
				sizeof(sleep_state)) != 0)
	{
		_locker_error(locker, strerror(errno), 1);
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


/* locker_action_unlock */
static int _locker_action_unlock(Locker * locker)
{
	size_t i;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s()\n", __func__);
#endif
	if(_locker_event(locker, LOCKER_EVENT_UNLOCKING) != 0)
		return -1;
	if(locker->adefinition->action(locker->auth, LOCKER_ACTION_UNLOCK) != 0)
		return -1;
	locker->locked = 0;
	_locker_action_deactivate(locker, 1);
	/* ungrab keyboard and mouse */
	if(locker->windows == NULL)
		return 0;
	gdk_keyboard_ungrab(GDK_CURRENT_TIME);
	gdk_pointer_ungrab(GDK_CURRENT_TIME);
	for(i = 0; i < locker->windows_cnt; i++)
		gtk_widget_hide(locker->windows[i]);
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
	return ret;
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
	return ret;
}


/* locker_error */
static int _locker_error(Locker * locker, char const * message, int ret)
{
	GtkWidget * dialog;

	if(locker == NULL || _locker_is_locked(locker))
	{
		fprintf(stderr, "%s: %s\n", "locker", message);
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
	return ret;
}


/* callbacks */
/* locker_on_closex */
static gboolean _lock_on_closex(void)
{
	return TRUE;
}


/* locker_on_filter */
static GdkFilterReturn _filter_xscreensaver_notify(Locker * locker,
		XScreenSaverNotifyEvent * xssne);

static GdkFilterReturn _locker_on_filter(GdkXEvent * xevent, GdkEvent * event,
		gpointer data)
{
	Locker * locker = data;
	XEvent * xev = xevent;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s() 0x%x 0x%x\n", __func__, xev->type,
			event->type);
#endif
	if(xev->type == locker->event)
		return _filter_xscreensaver_notify(locker, xevent);
	else
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
			_locker_action_enable(locker);
			_locker_action_deactivate(locker, 0);
			break;
		case ScreenSaverOn:
			if(locker->enabled && locker->locked == 0)
				_locker_action_lock(locker, 0);
			break;
		case ScreenSaverDisabled:
			_locker_action_disable(locker);
			break;
	}
	return GDK_FILTER_CONTINUE;
}


/* locker_on_map_event */
static gboolean _locker_on_map_event(gpointer data)
{
	Locker * locker = data;
	GdkWindow * window;
	GdkGrabStatus status;

	/* FIXME detect if this is the first window */
	/* FIXME the mouse may already be grabbed (Panel's lock button...) */
	/* grab keyboard and mouse */
#if GTK_CHECK_VERSION(2, 14, 0)
	window = gtk_widget_get_window(locker->windows[0]);
#else
	window = locker->windows[0]->window;
#endif
	if(window == NULL)
		_locker_error(NULL, "Failed to grab input", 1);
	else
	{
		if((status = gdk_keyboard_grab(window, TRUE, GDK_CURRENT_TIME))
				!= GDK_GRAB_SUCCESS)
			_locker_error(NULL, "Failed to grab keyboard", 1);
#ifdef DEBUG
		fprintf(stderr, "DEBUG: keyboard grab status=%u\n", status);
#endif
		if((status = gdk_pointer_grab(window, TRUE, 0, window, NULL,
						GDK_CURRENT_TIME))
				!= GDK_GRAB_SUCCESS)
			_locker_error(NULL, "Failed to grab mouse", 1);
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


/* locker_on_realize */
static void _locker_on_realize(GtkWidget * widget, gpointer data)
{
	Locker * locker = data;

	if(locker->ddefinition != NULL && locker->ddefinition->add != NULL)
		locker->ddefinition->add(locker->demo, widget);
}
