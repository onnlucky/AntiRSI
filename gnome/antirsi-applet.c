/*
 * author: Onne Gorter <o.gorter@gmail.com>
 * package: antirsi-gnome
 * license: GPL
 *
 * TODO
 * fix unexpected quit ...
 * add about/help/configuration
 * add break now / postpone break and natural break menu
 * remove app.h and put antirsi-applet.h with struct
 */

#include <X11/extensions/scrnsaver.h>
#include <gdk/gdkx.h>

#include "status-display.h"
#include "break-window.h"
#include "app.h"

#include "antirsi-applet.h"

#include <glib-object.h>
#include <panel-applet.h>
#include <panel-applet-gconf.h>

static double
get_idle_time(app_context *app)
{
  XScreenSaverInfo * sc_info;
  double idle_time;

  sc_info = XScreenSaverAllocInfo();
  XScreenSaverQueryInfo(GDK_DISPLAY(), GDK_ROOT_WINDOW(), sc_info);
  idle_time = sc_info->idle / 1000.0;
  free(sc_info);

  return idle_time;
}

gint
app_tick(gpointer data)
{
  app_context * app = (app_context *)data;

  ai_tick(app->core, get_idle_time(app));
  status_display_update(app->status_display);

  return TRUE;
}

G_DEFINE_TYPE (AntiRSIApplet, antirsi_applet, PANEL_TYPE_APPLET);

static void antirsi_applet_change_background(PanelApplet *applet, PanelAppletBackgroundType type,
    GdkColor  *colour, GdkPixmap *pixmap);

static void
antirsi_applet_class_init(AntiRSIAppletClass *klass)
{
}

static void
antirsi_applet_init(AntiRSIApplet *self)
{
  g_signal_connect(self, "change-background", G_CALLBACK (antirsi_applet_change_background), NULL);
}

AntiRSIApplet*
antirsi_applet_new(void)
{
  return g_object_new (TYPE_ANTIRSI_APPLET, NULL);
}

static void
antirsi_applet_change_background(PanelApplet *applet, PanelAppletBackgroundType type,
    GdkColor  *colour, GdkPixmap *pixmap)
{
  GtkRcStyle *rc_style;
  GtkStyle *style;

  /* reset style */
  gtk_widget_set_style (GTK_WIDGET (applet), NULL);
  rc_style = gtk_rc_style_new ();
  gtk_widget_modify_style (GTK_WIDGET (applet), rc_style);
  gtk_rc_style_unref (rc_style);

  switch (type){
    case PANEL_NO_BACKGROUND:
      break;
    case PANEL_COLOR_BACKGROUND:
      gtk_widget_modify_bg (GTK_WIDGET (applet),
                            GTK_STATE_NORMAL, colour);
      break;
    case PANEL_PIXMAP_BACKGROUND:
      style = gtk_style_copy (GTK_WIDGET (applet)->style);

      if (style->bg_pixmap[GTK_STATE_NORMAL])
        g_object_unref (style->bg_pixmap[GTK_STATE_NORMAL]);

      style->bg_pixmap[GTK_STATE_NORMAL] = g_object_ref (pixmap);
      gtk_widget_set_style (GTK_WIDGET (applet), style);
      g_object_unref (style);
      break;
  }
}

static const BonoboUIVerb antirsi_applet_menu_verbs [] = {
//        BONOBO_UI_UNSAFE_VERB ("Prefs", preferences_cb),
//        BONOBO_UI_UNSAFE_VERB ("Help", help_cb),
//        BONOBO_UI_UNSAFE_VERB ("About", about_cb),

        BONOBO_UI_VERB_END
};

static const char context_menu_xml [] =
   "<popup name=\"button1\">\n"
   "   <menuitem name=\"Item0\" "
   "             verb=\"TakeBreakNow\" "
   "           _label=\"_Take break now...\"\n"
   "          pixtype=\"stock\" "
   "          pixname=\"gtk-properties\"/>\n"
   "   <menuitem name=\"Item1\" "
   "             verb=\"Prefs\" "
   "           _label=\"_Preferences...\"\n"
   "          pixtype=\"stock\" "
   "          pixname=\"gtk-properties\"/>\n"
   "   <menuitem name=\"Item2\" "
   "             verb=\"Help\" "
   "           _label=\"_Help\"\n"
   "          pixtype=\"stock\" "
   "          pixname=\"gnome-help\"/>\n"
   "   <menuitem name=\"Item3\" "
   "             verb=\"About\" "
   "           _label=\"_About\"\n"
   "          pixtype=\"stock\" "
   "          pixname=\"gnome-stock-about\"/>\n"
   "</popup>\n";

static gboolean
antirsi_applet_fill(PanelApplet *applet)
{
    g_debug("starting antirsi-applet");

    AntiRSIApplet *antirsi_applet;

    g_return_val_if_fail(PANEL_IS_APPLET (applet), FALSE);

    app_context * app;
    app = g_new0(app_context, 1);

    app->core = antirsi_init(app);

    app->core->emit_mini_break_start = handle_mini_break_start;
    app->core->emit_work_break_start = handle_work_break_start;
    app->core->emit_break_end = handle_break_end;
    app->core->emit_break_update = handle_break_update;

    app->timer = g_timeout_add(100, app_tick, app);

    app->status_display = status_display_new(app->core);

    gtk_container_add(GTK_CONTAINER(applet), app->status_display);

    break_window_create(app);

    gtk_widget_show_all(GTK_WIDGET (applet));


    panel_applet_setup_menu(applet, context_menu_xml, antirsi_applet_menu_verbs, antirsi_applet);

        if (panel_applet_get_locked_down (applet)) {
                BonoboUIComponent *popup_component;

                popup_component = panel_applet_get_popup_component (applet);

                bonobo_ui_component_set_prop (popup_component,
                                              "/commands/Props",
                                              "hidden", "1",
                                              NULL);
        }

        return TRUE;
}

static gboolean
antirsi_applet_factory (PanelApplet *applet,
                        const gchar *iid,
                        gpointer     data)
{
        g_debug("antirsi applet factory");
        gboolean retval = FALSE;

        if (!strcmp (iid, "OAFIID:AntiRSIApplet"))
                retval = antirsi_applet_fill (applet);

        if (retval == FALSE) {
                exit (-1);
        }

        return retval;
}

PANEL_APPLET_BONOBO_FACTORY ("OAFIID:AntiRSIApplet_Factory",
                             TYPE_ANTIRSI_APPLET,
                             "antirsi-applet",
                             "0",
                             antirsi_applet_factory,
                             NULL)


