#ifndef _app_h_
#define _app_h_

// #define DEBUG

/*
 * author: Onne Gorter <o.gorter@gmail.com>
 * package: antirsi-gnome
 * license: GPL
 */

#include <gtk/gtk.h>
#include <math.h>
#include <X11/extensions/scrnsaver.h>

#include "../antirsi-core/antirsi-core.h"

typedef struct _app_context {
  // debug window
  GtkWidget *window;
  GtkWidget *mini_label;
  GtkWidget *work_label;
  GtkWidget *status_label;
  GtkWidget *history_label;

  // mini or work break window and widgets
  GtkWidget *break_window;
  GtkWidget *break_type_label;
  GtkWidget *time_left_label;
  GtkWidget *time_until_label;
  GtkWidget *progress_bar;
  GtkWidget *postpone_button;

  // status display
  GtkWidget *status_display;

  // timer driving the ticks
  gint timer;

  // the low level antirsi core
  ai_core * core;

} app_context;

#endif
