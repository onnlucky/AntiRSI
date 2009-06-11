#ifndef _status_display_h_
#define _status_display_h_

/*
 * author: Onne Gorter <o.gorter@gmail.com>
 * package: antirsi-gnome
 * license: GPL
 */

#include <glib-object.h>
#include <gtk/gtk.h>

#include "../antirsi-core/antirsi-core.h"

G_BEGIN_DECLS

#define TYPE_STATUS_DISPLAY (status_display_get_type ())
#define STATUS_DISPLAY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_STATUS_DISPLAY, StatusDisplay))
#define STATUS_DISPLAY_CLASS(obj) (G_TYPE_CHECK_CLASS_CAST ((obj), STATUS_DISPLAY,  StatusDisplayClass))
#define IS_STATUS_DISPLAY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_STATUS_DISPLAY))
#define IS_STATUS_DISPLAY_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE ((obj), TYPE_STATUS_DISPLAY))
#define STATUS_DISPLAY_GET_CLASS (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_STATUS_DISPLAY, StatusDisplayClass))

typedef struct {
    GtkDrawingArea parent;
} StatusDisplay;

typedef struct {
    GtkDrawingAreaClass parent_class;
} StatusDisplayClass;

GType status_display_get_type (void);

GtkWidget * status_display_new(ai_core *core);
void status_display_update(GtkWidget *status_display);

G_END_DECLS

#endif
