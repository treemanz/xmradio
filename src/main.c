/** 
 * main.c
 * This file is part of xmradio
 *
 * Copyright (C) 2012  Weitian Leung (weitianleung@gmail.com)

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib/gi18n.h>
#include <locale.h>
#include <glib.h>
#include <curl/curl.h>
#include <gst/gst.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <glib/gstdio.h>

#include "xmrapp.h"
#include "config.h"
#include "xmrdebug.h"
#include "xmrutil.h"

static gboolean debug = FALSE;
static gboolean action_play = FALSE;
static gboolean action_pause = FALSE;
static gboolean action_next = FALSE;
static gboolean action_love = FALSE;
static gboolean action_hate = FALSE;

static gboolean
print_version_and_exit (const gchar *option_name,
			 const gchar *value,
			 gpointer data,
			 GError **error)
{
	g_print("%s %s\n", PACKAGE, VERSION);
	exit(EXIT_SUCCESS);
	return TRUE;
}

const GOptionEntry options []  =
{
	{"debug",	'd', 0, G_OPTION_ARG_NONE,	&debug, N_("Enable debug output"), NULL},
	{"play",	'p', 0, G_OPTION_ARG_NONE,	&action_play, N_("Play song"), NULL},
	{"pause",	's', 0, G_OPTION_ARG_NONE,	&action_pause, N_("Pause current song if playing"), NULL},
	{"next",	'n', 0, G_OPTION_ARG_NONE,	&action_next, N_("Play next song"), NULL},
	{"love",	'l', 0, G_OPTION_ARG_NONE,	&action_love, N_("Love current song"), NULL},
	{"hate",	't', 0, G_OPTION_ARG_NONE,	&action_hate, N_("Hate current song"), NULL},
	{"version", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, print_version_and_exit, N_("Show the application's version"), NULL},
	{ NULL }
};

typedef enum
{
	ActionNone,
	ActionPlay,
	ActionPause,
	ActionNext,
	ActionLove,
	ActionHate
}
PlayerAction;

static void
send_action(DBusConnection *bus, PlayerAction action);

static void
remove_file(const gchar *file, gpointer data);

int main(int argc, char **argv)
{
	XmrApp *app;
	GOptionContext *context;
	GError *error = NULL;
	PlayerAction player_action = ActionNone;
	gchar *tmp_dir = NULL;

#if !GLIB_CHECK_VERSION(2, 32, 0)
	g_thread_init(NULL);
#endif

	gdk_threads_init();

	g_type_init();

	setlocale(LC_ALL, NULL);

#ifdef ENABLE_NLS
	/* initialize i18n */
	bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	textdomain(GETTEXT_PACKAGE);
#endif
	
	context = g_option_context_new(NULL);

	g_option_context_add_main_entries(context, options, GETTEXT_PACKAGE);

	g_option_context_add_group(context, gtk_get_option_group(TRUE));
	g_option_context_add_group(context, gst_init_get_option_group());

	if (g_option_context_parse(context, &argc, &argv, &error) == FALSE)
	{
		g_print(_("%s\nRun '%s --help' to see a full list of available command line options.\n"),
			 error->message, argv[0]);
		g_error_free(error);
		g_option_context_free(context);
		exit(1);
	}

	g_option_context_free(context);

	if (action_play){
		player_action = ActionPlay;
	}else if (action_pause){
		player_action = ActionPause;
	}else if(action_next){
		player_action = ActionNext;
	}else if(action_love){
		player_action = ActionLove;
	}else if(action_hate){
		player_action = ActionHate;
	}

	if (player_action != ActionNone)
	{
		DBusConnection *bus;
		DBusError dbus_error;
		dbus_error_init(&dbus_error);
		bus = dbus_bus_get(DBUS_BUS_SESSION, &dbus_error);
		if (!bus)
		{
			g_warning ("Failed to connect to the D-BUS daemon: %s", dbus_error.message);
			dbus_error_free(&dbus_error);
			exit(1);
		}
		
		dbus_connection_setup_with_g_main(bus, NULL);

		send_action(bus, player_action);

		// exit directly
		return 0;
	}

	xmr_debug_enable(debug);

	gst_init(&argc, &argv);

	gdk_threads_init();
	curl_global_init(CURL_GLOBAL_ALL);

	// this make our XmrRadio always works
	g_object_set(gtk_settings_get_default(),
				"gtk-button-images", TRUE,
				NULL);

	// ensure folder exists
	tmp_dir = g_strdup_printf("%s/%s", g_get_tmp_dir(), PACKAGE);
	g_mkdir_with_parents(tmp_dir, 0755);

	app = xmr_app_new();

	g_application_run(G_APPLICATION(app), argc, argv);

	// remove ...
	list_file(tmp_dir, FALSE, remove_file, NULL);

	g_free(tmp_dir);
	g_object_unref(app);

	curl_global_cleanup();

	return 0;
}

static void
send_action(DBusConnection *bus, PlayerAction action)
{
	DBusMessage *message;

	dbus_int32_t d_action = action;

	message = dbus_message_new_signal("/com/xmradio/dbus/action",
                                     "com.xmradio.dbus.Signal",
									 "Action");
	dbus_message_append_args(message,
                            DBUS_TYPE_INT32, &d_action,
                            DBUS_TYPE_INVALID);

	dbus_connection_send(bus, message, NULL);
	
	dbus_message_unref(message);
}

static void
remove_file(const gchar *file, gpointer data)
{
	g_remove(file);
}
