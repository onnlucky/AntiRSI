/*
 * author: Onne Gorter <o.gorter@gmail.com>
 * package: antirsi-gnome
 * license: GPL
 */

#include "break-window.h"

gboolean handle_expose(GtkWidget *widget, GdkEventExpose *event, gpointer userdata);

static void
handle_postpone_clicked(GtkWidget * widget, gpointer data)
{
  app_context * app = (app_context *)data;

  ai_work_break_postpone(app->core);
}

void
handle_mini_break_start(gpointer data)
{
  app_context * app = (app_context *)data;

  gtk_label_set_markup(GTK_LABEL(app->break_type_label), "<span font_desc='Sans Bold 25'>Mini Break</span>");
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), 0);

  gtk_window_set_position(GTK_WINDOW(app->break_window), GTK_WIN_POS_CENTER);

  gtk_widget_show_all(app->break_window);
  gtk_widget_hide(app->postpone_button);

  gtk_window_stick(GTK_WINDOW(app->break_window));
  gtk_window_set_keep_above(GTK_WINDOW(app->break_window), TRUE);
}

void
handle_work_break_start(gpointer data)
{
  app_context * app = (app_context *)data;

  gtk_label_set_markup(GTK_LABEL(app->break_type_label), "<span font_desc='Sans Bold 25'>Work Break</span>");
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), 0);

  gtk_window_set_position(GTK_WINDOW(app->break_window), GTK_WIN_POS_CENTER);

  gtk_widget_show_all(app->break_window);
  gtk_widget_show(app->postpone_button);

  gtk_window_stick(GTK_WINDOW(app->break_window));
  gtk_window_set_keep_above(GTK_WINDOW(app->break_window), TRUE);
}

void
handle_break_end(gpointer data)
{
  app_context * app = (app_context *)data;

  gtk_widget_hide(app->break_window);
  gtk_widget_hide(app->postpone_button);
}

void
handle_break_update(gpointer data)
{
  app_context * app = (app_context *)data;
  char * tmp;

  int seconds = ai_break_time_left(app->core);
  int minutes = (int)round(ai_seconds_until_next_work_break(app->core) / 60.0);

  tmp = g_strdup_printf("%d:%02d", seconds / 60, seconds % 60);
  gtk_label_set_text(GTK_LABEL(app->time_left_label), tmp);
  g_free(tmp);

  if (minutes > 60) {
    tmp = g_strdup_printf("next break in %d:%02d hours", minutes / 60, minutes % 60);
  } else if (seconds > 60) {
    tmp = g_strdup_printf("next break in %d minutes", minutes);
  } else if (minutes == 1) {
    tmp = g_strdup_printf("next break in 1 minute");
  } else {
    tmp = g_strdup_printf("next break in %d minutes", minutes);
  }

  gtk_label_set_text(GTK_LABEL(app->time_until_label), tmp);
  g_free(tmp);

  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), ai_break_progress(app->core));
}

static gboolean
handle_clicked(GtkWidget * window, GdkEventButton * event, gpointer data)
{
  //app_context * app = (app_context *)data;

  if (event->button == 1) {
    gtk_window_begin_move_drag (GTK_WINDOW(window), event->button, event->x_root, event->y_root, event->time);

    return FALSE;
  }

  return FALSE;
}

/* Only some X servers support alpha channels. Always have a fallback */
gboolean supports_alpha = FALSE;

static void
handle_screen_changed(GtkWidget *widget, GdkScreen *old_screen, gpointer userdata)
{
    /* To check if the display supports alpha channels, get the colormap */
    GdkScreen *screen = gtk_widget_get_screen(widget);
    GdkColormap *colormap = gdk_screen_get_rgba_colormap(screen);

    if (!colormap)
    {
        g_debug("Your screen does NOT support alpha channels!\n");
        colormap = gdk_screen_get_rgb_colormap(screen);
        supports_alpha = FALSE;
    }
    else
    {
        g_debug("Your screen supports alpha channels!\n");
        supports_alpha = TRUE;
    }

    /* Now we have a colormap appropriate for the screen, use it */
    gtk_widget_set_colormap(widget, colormap);
}

void
break_window_create(app_context * app)
{
  GtkWidget * fixed;

  app->break_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  // GTK_WINDOW_POPUP doesn't work, as it can not be made movable ...
  // gdk_window_set_override_redirect(GDK_WINDOW(app->break_window->window), FALSE);
  // gdk_window_set_type_hint(GDK_WINDOW(app->break_window->window), GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);
  // gtk_window_set_type_hint(GTK_WINDOW(app->break_window), GDK_WINDOW_TYPE_HINT_MENU);
  gtk_window_set_accept_focus(GTK_WINDOW(app->break_window), FALSE);
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(app->break_window), TRUE);

  gtk_widget_set_size_request(app->break_window, 300, 300);
  gtk_window_set_resizable(GTK_WINDOW(app->break_window), FALSE);
  gtk_window_set_title(GTK_WINDOW(app->break_window), "Break");
  // gtk_window_set_deletable(GTK_WINDOW(app->break_window), FALSE); /* gtk 2.10 */

  gtk_window_set_position(GTK_WINDOW(app->break_window), GTK_WIN_POS_CENTER);
  gtk_window_stick(GTK_WINDOW(app->break_window));
  gtk_window_set_decorated(GTK_WINDOW(app->break_window), FALSE);
  gtk_window_set_keep_above(GTK_WINDOW(app->break_window), TRUE);

  // magic to be transparent and movable
  gtk_widget_set_app_paintable(app->break_window, TRUE);

  g_signal_connect(G_OBJECT(app->break_window), "expose-event", G_CALLBACK(handle_expose), NULL);
  g_signal_connect(G_OBJECT(app->break_window), "screen-changed", G_CALLBACK(handle_screen_changed), NULL);

  gtk_widget_add_events(app->break_window, GDK_BUTTON_PRESS_MASK);
  g_signal_connect(G_OBJECT(app->break_window), "button-press-event", G_CALLBACK(handle_clicked), app);


  // add widgets
  fixed = gtk_fixed_new();

  app->break_type_label = gtk_label_new("mini break");
  gtk_label_set_markup(GTK_LABEL(app->break_type_label), "<span font_desc='Sans Bold 25'>Mini Break</span>");
  gtk_widget_set_size_request(app->break_type_label, 298, -1);

  app->time_left_label = gtk_label_new("0:00");

  app->progress_bar = gtk_progress_bar_new();
  gtk_widget_set_size_request(app->progress_bar, 290, -1);

  app->postpone_button = gtk_button_new_with_label(" Postpone ");
  g_signal_connect(G_OBJECT(app->postpone_button), "clicked", G_CALLBACK(handle_postpone_clicked), app);

  app->time_until_label = gtk_label_new("next break in 0 minutes");

  gtk_fixed_put(GTK_FIXED(fixed), app->break_type_label, 1, 100);
  gtk_fixed_put(GTK_FIXED(fixed), app->time_left_label, 200, 190);
  gtk_fixed_put(GTK_FIXED(fixed), app->progress_bar, 5, 210);
  gtk_fixed_put(GTK_FIXED(fixed), app->postpone_button, 200, 260);
  gtk_fixed_put(GTK_FIXED(fixed), app->time_until_label, 60, 235);

  gtk_container_add(GTK_CONTAINER(app->break_window), fixed);

  handle_screen_changed(app->break_window, NULL, NULL);
}

// DRAWING

void
cairo_rounded_rectangle(cairo_t * cr, double x, double y, double width, double height, double radius)
{
  double x0, y0, x1,y1;

  x0 = x;
  y0 = y;
  x1 = x + width;
  y1 = y + height;

  cairo_move_to(cr, x0, y0 + radius);

  cairo_curve_to(cr, x0 , y0, x0, y0, x0 + radius, y0);

  cairo_line_to(cr, x1 - radius, y0);

  cairo_curve_to(cr, x1 , y0, x1, y0, x1, y0 + radius);

  cairo_line_to(cr, x1, y1 - radius);

  cairo_curve_to(cr, x1 , y1, x1, y1, x1 - radius, y1);

  cairo_line_to(cr, x0 + radius, y1);

  cairo_curve_to(cr, x0 , y1, x0, y1, x0, y1 - radius);

  cairo_close_path (cr);
}

/* This is called when we need to draw the windows contents */
gboolean
handle_expose(GtkWidget *widget, GdkEventExpose *event, gpointer userdata)
{
    cairo_t *cr = gdk_cairo_create(widget->window);

    int width, height;

    GdkColor bc = widget->style->bg[GTK_STATE_NORMAL];
    GdkColor dc = widget->style->dark[GTK_STATE_NORMAL];

    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_width(cr, 2.0);

    gtk_window_get_size(GTK_WINDOW(widget), &width, &height);

    if (supports_alpha) {
      cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0); /* transparent */
    } else {
      cairo_set_source_rgb (cr, bc.red / 65536.0, bc.green / 65536.0, bc.blue / 65536.0 );
    }

    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
    cairo_paint (cr);

    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

    cairo_rounded_rectangle(cr, 2, 2, width - 4, height - 4, 40.0);

    if (supports_alpha) {
      cairo_set_source_rgba (cr, bc.red / 65536.0, bc.green / 65536.0, bc.blue / 65536.0, 0.85 );
    } else {
      cairo_set_source_rgb (cr, bc.red / 65536.0, bc.green / 65536.0, bc.blue / 65536.0 );
    }

    cairo_fill_preserve(cr);

    if (supports_alpha) {
      cairo_set_source_rgba (cr, dc.red / 65536.0, dc.green / 65536.0, dc.blue / 65536.0, 0.85 );
    } else {
      cairo_set_source_rgb (cr, dc.red / 65536.0, dc.green / 65536.0, dc.blue / 65536.0 );
    }

    cairo_stroke(cr);

    cairo_destroy(cr);
    return FALSE;
}

