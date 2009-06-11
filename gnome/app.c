/*
 * author: Onne Gorter <o.gorter@gmail.com>
 * package: antirsi-gnome
 * license: GPL
 *
 *
 * TODO
 * status bar plugin
 *
 * settings window / gconf configuration
 *
 * make window always on top of everything, currently it doesn't move anymore then
 */

#include "app.h"

#include <X11/extensions/scrnsaver.h>
#include <gdk/gdkx.h>

#include "break-window.h"
#include "status-display.h"

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

static gboolean
delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  return FALSE;
}

static void
destroy(GtkWidget *widget, gpointer data)
{
  gtk_main_quit();
}

gint
app_tick(gpointer data)
{
  char * tmp;
  app_context * app = (app_context *)data;

  ai_tick(app->core, get_idle_time(app));

  tmp = g_strdup_printf("mini: %2.2f; %2.2f", app->core->mini_t, app->core->mini_taking_t);
  gtk_label_set_text(GTK_LABEL(app->mini_label), tmp);
  g_free(tmp);

  tmp = g_strdup_printf("work: %2.2f; %2.2f", app->core->work_t, app->core->work_taking_t);
  gtk_label_set_text(GTK_LABEL(app->work_label), tmp);
  g_free(tmp);

  tmp = g_strdup_printf("state: %s", (app->core->state == S_NORMAL)?"normal":(app->core->state == S_IN_MINI)?"mini":"work");
  gtk_label_set_text(GTK_LABEL(app->status_label), tmp);
  g_free(tmp);

  tmp = g_strdup_printf("history: %1.1f, %1.1f, %1.1f, %1.1f", app->core->ith[0], app->core->ith[1], app->core->ith[2], app->core->ith[3]);
  gtk_label_set_text(GTK_LABEL(app->history_label), tmp);
  g_free(tmp);

  status_display_update(app->status_display);

  return TRUE;
}

int
main(int argc, char * argv[])
{
  app_context * app;
  app = g_new0(app_context, 1);

  GtkWidget *vbox;

  gtk_init(&argc, &argv);
  app->core = antirsi_init(app);

  //app->core->emit_status_update = handle_status_update;

  app->core->emit_mini_break_start = handle_mini_break_start;
  app->core->emit_work_break_start = handle_work_break_start;
  app->core->emit_break_end = handle_break_end;
  app->core->emit_break_update = handle_break_update;

#ifdef DEBUG
  app->core->mini_duration = 4; // 14;
  app->core->mini_interval = 10; // 4*60;
  app->core->work_duration = 20; // 8*50
  app->core->work_interval = 60 * 300; // 50*60;
#endif

  app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(app->window), "AntiRSI");

  app->mini_label = gtk_label_new("mini:  0;  0");
  app->work_label = gtk_label_new("work:  0;  0");
  app->status_label = gtk_label_new("status: normal");
  app->history_label = gtk_label_new("history: 0, 0, 0, 0");
  app->status_display = status_display_new(app->core);

  g_signal_connect(G_OBJECT(app->window), "delete_event", G_CALLBACK(delete_event), app);
  g_signal_connect(G_OBJECT(app->window), "destroy", G_CALLBACK(destroy), app);

  vbox = gtk_vbox_new(FALSE, 4);

  gtk_container_add(GTK_CONTAINER(vbox), app->mini_label);
  gtk_container_add(GTK_CONTAINER(vbox), app->work_label);
  gtk_container_add(GTK_CONTAINER(vbox), app->status_label);
  gtk_container_add(GTK_CONTAINER(vbox), app->history_label);
  gtk_container_add(GTK_CONTAINER(vbox), app->status_display);

  gtk_container_add(GTK_CONTAINER(app->window), vbox);

  app->timer = g_timeout_add(100, app_tick, app);

  break_window_create(app);

  gtk_widget_show_all(app->window);

#ifdef DEBUG
  gtk_widget_show_all(app->break_window);
#endif

  gtk_main();

  return 0;
}

