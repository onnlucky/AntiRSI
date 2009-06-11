/*
 * author: Onne Gorter <o.gorter@gmail.com>
 * package: antirsi-gnome
 * license: GPL
 */

#include <gtk/gtk.h>

#include "status-display.h"
#include "../antirsi-core/antirsi-core.h"

#define STATUS_DISPLAY_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), TYPE_STATUS_DISPLAY, StatusDisplayPrivate))

G_DEFINE_TYPE(StatusDisplay, status_display, GTK_TYPE_DRAWING_AREA);

typedef struct _StatusDisplayPrivate StatusDisplayPrivate;
struct _StatusDisplayPrivate {
    ai_core * core;
};

#define DRAW_TRESHOLD 0.0001

static void
draw(GtkWidget *status_display, cairo_t *cr)
{
  StatusDisplayPrivate *priv;
  int width = status_display->allocation.width;
  int height = status_display->allocation.height;

  priv = STATUS_DISPLAY_GET_PRIVATE(status_display);
  double tmp;

  // whole widget
  GdkColor bc = status_display->style->bg[GTK_STATE_NORMAL];
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_set_source_rgb(cr, bc.red / 65536.0, bc.green / 65536.0, bc.blue / 65536.0);
  cairo_fill_preserve(cr);
  cairo_set_source_rgb(cr, bc.red / 62536.0, bc.green / 62536.0, bc.blue / 62536.0);
  cairo_stroke(cr);

  // mini break
  // elapsed
  tmp = priv->core->mini_t / priv->core->mini_interval;
  if (tmp > DRAW_TRESHOLD) {
    cairo_rectangle(cr, 1, (height / 40.0) * 4.0, (width - 1) * tmp, (height / 40.0) * 13.0);
    cairo_set_source_rgba(cr, 0.5, 0.5, 1, 0.8);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.4, 0.4, 1, 0.8);
    cairo_stroke(cr);
  }

  // natural
  tmp = priv->core->mini_taking_t / priv->core->mini_duration;
  if (tmp > DRAW_TRESHOLD) {
    cairo_rectangle(cr, 1, (height / 40.0) * 3.0, (width - 1) * tmp, (height / 40.0) * 16.0);
    cairo_set_source_rgba(cr, 0.5, 1, 0.5, 0.8);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.4, 1, 0.4, 0.8);
    cairo_stroke(cr);
  }

  // work break
  // elapsed
  tmp = priv->core->work_t / priv->core->work_interval;
  if (tmp > DRAW_TRESHOLD) {
    cairo_rectangle(cr, 1, (height / 40.0) * 23.0, (width - 1) * tmp, (height / 40.0) * 13.0);
    cairo_set_source_rgba(cr, 0.5, 0.5, 1, 0.8);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.4, 0.4, 1, 0.8);
    cairo_stroke(cr);
  }

  // natural
  tmp = priv->core->work_taking_t / priv->core->work_duration;
  if (tmp > DRAW_TRESHOLD) {
    cairo_rectangle(cr, 1, (height / 40.0) * 21.0, (width - 1) * tmp, (height / 40.0) * 16.0);
    cairo_set_source_rgba(cr, 0.5, 1, 0.5, 0.8);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.4, 1, 0.4, 0.8);
    cairo_stroke(cr);
  }
}

static gboolean
status_display_expose(GtkWidget *status_display, GdkEventExpose *event)
{
  cairo_t *cr;

  cr = gdk_cairo_create(status_display->window);

  draw(status_display, cr);

  cairo_destroy(cr);

  return FALSE;
}

void
status_display_update(GtkWidget *status_display)
{
  status_display_expose(status_display, NULL);
}

static void
status_display_class_init(StatusDisplayClass *class)
{
  GObjectClass *obj_class;
  GtkWidgetClass *widget_class;

  obj_class = G_OBJECT_CLASS(class);
  widget_class = GTK_WIDGET_CLASS(class);

  widget_class->expose_event = status_display_expose;

  g_type_class_add_private(obj_class, sizeof(StatusDisplayPrivate));
}

static void
status_display_init(StatusDisplay *status_display)
{
  gtk_widget_set_size_request(GTK_WIDGET(status_display), 16, 24);
}

GtkWidget *
status_display_new(ai_core *core)
{
  StatusDisplay *status_display = g_object_new(TYPE_STATUS_DISPLAY, NULL);
  StatusDisplayPrivate *priv = STATUS_DISPLAY_GET_PRIVATE(status_display);

  priv->core = core;

  return GTK_WIDGET(status_display);
}

