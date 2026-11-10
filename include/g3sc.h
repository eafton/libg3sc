#include <glib-object.h>
#include <g3sc-export.h>

#ifndef G3SC_H
#define G3SC_H

G_BEGIN_DECLS

/* Version */
#define G3SC_VERSION_MAJOR 0
#define G3SC_VERSION_MINOR 0

/* Node */
#define G3SC_TYPE_NODE (g3sc_node_get_type())
#define G3SC_NODE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), G3SC_TYPE_NODE, G3SCNode))
#define G3SC_NODE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), G3SC_TYPE_NODE, G3SCNodeClass))
#define G3SC_IS_NODE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), G3SC_TYPE_NODE))
#define G3SC_IS_NODE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), G3SC_TYPE_NODE))
#define G3SC_NODE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), G3SC_TYPE_NODE, G3SCNodeClass))

typedef struct _G3SCNode G3SCNode;
typedef struct _G3SCNodeClass G3SCNodeClass;
typedef struct _G3SCNodePrivate G3SCNodePrivate;

struct _G3SCNode {
	GObject parent_instance;
	G3SCNodePrivate *priv;
};

struct _G3SCNodeClass {
	GObjectClass parent_class;
};

G3SC_EXPORT GType g3sc_node_get_type(void) G_GNUC_CONST;
G3SC_EXPORT G3SCNode *g3sc_node_new(const gchar *name);
G3SC_EXPORT void g3sc_node_set_parent(G3SCNode *node, G3SCNode *parent);
G3SC_EXPORT G3SCNode *g3sc_node_get_parent(G3SCNode *node);
G3SC_EXPORT void g3sc_node_set_name(G3SCNode *node, const gchar *name);
G3SC_EXPORT const gchar *g3sc_node_get_name(G3SCNode *node);
G3SC_EXPORT void g3sc_node_set_id(G3SCNode *node, const gchar *id);
G3SC_EXPORT const gchar *g3sc_node_get_id(G3SCNode *node);
G3SC_EXPORT void g3sc_node_add_child(G3SCNode *node, G3SCNode *child);
G3SC_EXPORT void g3sc_node_remove_child(G3SCNode *node, G3SCNode *child);
G3SC_EXPORT void g3sc_node_foreach_child(G3SCNode *node, void (*callback)(G3SCNode *p, G3SCNode *c, gpointer u), gpointer udata);
G3SC_EXPORT void g3sc_node_add_class(G3SCNode *node, const gchar *klass);
G3SC_EXPORT void g3sc_node_remove_class(G3SCNode *node, const gchar *klass);
G3SC_EXPORT void g3sc_node_foreach_class(G3SCNode *node, void (*callback)(G3SCNode *p, const gchar *c, gpointer u), gpointer udata);

/* Style */
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
