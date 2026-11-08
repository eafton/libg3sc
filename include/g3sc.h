#include <glib-object.h>
#include <g3sc-export.h>

#ifndef G3SC_H
#define G3SC_H

G_BEGIN_DECLS

#define G3SC_VERSION_MAJOR 0
#define G3SC_VERSION_MINOR 0

#define G3SC_TYPE_STYLE (g3sc_style_get_type())
#define G3SC_STYLE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), G3SC_TYPE_STYLE, G3SCStyle))
#define G3SC_STYLE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), G3SC_TYPE_STYLE, G3SCStyleClass))
#define G3SC_IS_STYLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), G3SC_TYPE_STYLE))
#define G3SC_IS_STYLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), G3SC_TYPE_STYLE))
#define G3SC_STYLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), G3SC_TYPE_STYLE, G3SCStyleClass))

typedef struct _G3SCStyle G3SCStyle;
typedef struct _G3SCStyleClass G3SCStyleClass;
typedef struct _G3SCStylePrivate G3SCStylePrivate;

struct _G3SCStyle {
	GObject parent_instance;
	G3SCStylePrivate *priv;
};

struct _G3SCStyleClass {
	GObjectClass parent_class;
};

G3SC_EXPORT GType g3sc_style_get_type(void) G_GNUC_CONST;
G3SC_EXPORT G3SCStyle *g3sc_style_new_from_file(const gchar *file);

G_END_DECLS

#endif
