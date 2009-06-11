#ifndef _antirsi_applet_h_
#define _antirsi_applet_h_

/*
 * author: Onne Gorter <o.gorter@gmail.com>
 * package: antirsi-gnome
 * license: GPL
 */

#include <glib-object.h>
#include <panel-applet.h>

G_BEGIN_DECLS

#define TYPE_ANTIRSI_APPLET antirsi_applet_get_type()
#define ANTIRSI_APPLET(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_ANTIRSI_APPLET, AntiRSIApplet))
#define ANTIRSI_APPLET_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_ANTIRSI_APPLET, AntiRSIAppletClass))
#define IS_ANTIRSI_APPLET(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_ANTIRSI_APPLET))
#define IS_ANTIRSI_APPLET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_ANTIRSI_APPLET))
#define ANTIRSI_APPLET_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_ANTIRSI_APPLET, AntiRSIAppletClass))

typedef struct {
        PanelApplet parent;
} AntiRSIApplet;

typedef struct {
        PanelAppletClass parent_class;
} AntiRSIAppletClass;

GType antirsi_applet_get_type (void);

AntiRSIApplet * antirsi_applet_new (void);

G_END_DECLS

#endif
