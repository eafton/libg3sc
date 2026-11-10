#include <stdio.h>
#include <fcntl.h>
#include <glib-object.h>
#include <g3sc.h>

struct _G3SCNodePrivate {
	gchar *name;
	gchar *id;

	G3SCNode *parent;
	
	GList *children;
	GList *classes;
};

typedef struct {
	G3SCNode *parent;
	gpointer udata;
	
	gboolean str;
	union _G3SCNodeUData {
		void (*node)(G3SCNode *p, G3SCNode *c, gpointer u);
		void (*string)(G3SCNode *p, const gchar *c, gpointer u);
	} callback;
} G3SCNodeUData;

enum {
	PROP_NAME = 1,
	PROP_ID
};

G_DEFINE_TYPE(G3SCNode, g3sc_node, G_TYPE_OBJECT)

void _unparent(gpointer data, gpointer udata) {
	g3sc_node_set_parent(G3SC_NODE(data), NULL);
}

static void _finalize(GObject *object) {
	G3SCNode *node;
	
	node = G3SC_NODE(object);

	g_free(node->priv->name);
	g_free(node->priv->id);
	
	if (node->priv->parent) {
		g_object_unref(node->priv->parent);
	}

	g_list_foreach(node->priv->children, _unparent, NULL);
	g_list_free_full(node->priv->children, g_object_unref);
	g_list_free_full(node->priv->classes, g_free);

	G_OBJECT_CLASS(g3sc_node_parent_class)->finalize(object);
}

static void _set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
	G3SCNode *node;
	
	node = G3SC_NODE(object);
	switch (prop_id) {
		case PROP_NAME:
			node->priv->name = g_value_dup_string(value);
			break;
		case PROP_ID:
			node->priv->id = g_value_dup_string(value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void _get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
	G3SCNode *node;
	
	node = G3SC_NODE(object);
	switch (prop_id) {
		case PROP_NAME:
			g_value_set_string(value, node->priv->name);
			break;
		case PROP_ID:
			g_value_set_string(value, node->priv->id);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
			break;
	}
}

static void g3sc_node_class_init(G3SCNodeClass *klass) {
	GObjectClass *g_class;

	g_class = G_OBJECT_CLASS(klass);
	
	g_class->finalize = _finalize;
	g_class->get_property = _get_property;
	g_class->set_property = _set_property;

    g_object_class_install_property(g_class, PROP_NAME, g_param_spec_string("name", "nick", "blurb", NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
    g_object_class_install_property(g_class, PROP_ID, g_param_spec_string("id", "nick", "blurb", NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private(klass, sizeof(G3SCNodePrivate));
}	

static void g3sc_node_init(G3SCNode *node) {
	node->priv = G_TYPE_INSTANCE_GET_PRIVATE(node, G3SC_TYPE_NODE, G3SCNodePrivate);
	node->priv->name = node->priv->id = NULL;
	node->priv->parent = NULL;
	node->priv->children = NULL;
	node->priv->classes = NULL;
}

G3SCNode *g3sc_node_new(const gchar *name) {
	return g_object_new(G3SC_TYPE_NODE, "name", name, NULL);
}

void g3sc_node_set_id(G3SCNode *node, const gchar *id) {
	node->priv->id = g_strdup(id);
	g_object_notify(G_OBJECT(node), "id");
}

const gchar *g3sc_node_get_id(G3SCNode *node) {
	return node->priv->id;
}

void g3sc_node_set_name(G3SCNode *node, const gchar *name) {
	node->priv->name = g_strdup(name);
	g_object_notify(G_OBJECT(node), "name");
}

const gchar *g3sc_node_get_name(G3SCNode *node) {
	return node->priv->name;
}

void g3sc_node_set_parent(G3SCNode *node, G3SCNode *parent) {
	if (node->priv->parent) {
		g_object_unref(node->priv->parent);
	}
	node->priv->parent = g_object_ref(parent);
}

G3SCNode *g3sc_node_get_parent(G3SCNode *node) {
	return node->priv->parent;
}

void g3sc_node_add_child(G3SCNode *node, G3SCNode *child) {
	if (!g_list_find(node->priv->children, child)) {
		node->priv->children = g_list_append(node->priv->children, g_object_ref(child));
		g3sc_node_set_parent(child, node);
	}
}

void g3sc_node_remove_child(G3SCNode *node, G3SCNode *child) {
	GList *found;
	
	found = g_list_find(node->priv->children, child);
	if (found) {
		node->priv->children = g_list_remove(node->priv->children, child);
		g3sc_node_set_parent(child, NULL);
		g_object_unref(child);
	}
}

void _foreach(gpointer data, gpointer udata) {
	G3SCNodeUData *ud;
	
	ud = udata;
	if (ud->str) {
		ud->callback.node(ud->parent, data, ud->udata);
	} else {
		ud->callback.string(ud->parent, data, ud->udata);
	}
}

void g3sc_node_foreach_child(G3SCNode *node, void (*callback)(G3SCNode *p, G3SCNode *c, gpointer u), gpointer udata) {
	G3SCNodeUData ud;
	
	ud.parent = node;
	ud.str = FALSE;
	ud.callback.node = callback;
	ud.udata = udata;
	g_list_foreach(node->priv->children, _foreach, &ud);
}

void g3sc_node_add_class(G3SCNode *node, const gchar *klass) {
	if (!g_list_find_custom(node->priv->classes, klass, (GCompareFunc)strcmp)) {
		node->priv->classes = g_list_append(node->priv->classes, g_strdup(klass));
	}
}

void g3sc_node_remove_class(G3SCNode *node, const gchar *klass) {
	GList *found;
	
	found = g_list_find_custom(node->priv->classes, klass, (GCompareFunc)strcmp);
	if (found) {
		gpointer d;
		
		d = found->data;
		node->priv->classes = g_list_remove(node->priv->classes, d);
		g_free(d);
	}
}

void g3sc_node_foreach_class(G3SCNode *node, void (*callback)(G3SCNode *p, const gchar *c, gpointer u), gpointer udata) {
	G3SCNodeUData ud;
	
	ud.parent = node;
	ud.str = TRUE;
	ud.callback.string = callback;
	ud.udata = udata;
	g_list_foreach(node->priv->classes, _foreach, &ud);
}



