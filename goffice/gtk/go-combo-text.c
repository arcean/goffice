/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-combo-text: A combo box for selecting from a list.
 */

#include <goffice/goffice-config.h>
#include "go-combo-text.h"
#include "go-combo-box.h"
#include <goffice/utils/go-marshalers.h>
#include <goffice/gtk/go-gtk-compat.h>

#include <gsf/gsf-impl-utils.h>

struct _GOComboText {
	GOComboBox parent;

	GCompareFunc cmp_func;

	GtkWidget *entry;
	GtkWidget *list;
	GtkWidget *scroll;
	int rows;
};

typedef struct {
	GOComboBoxClass	base;

	gboolean (* selection_changed)	(GOComboText *ct, GtkTreeSelection *selection);
	gboolean (* entry_changed)	(GOComboText *ct, char const *new_str);
} GOComboTextClass;

#define GO_COMBO_TEXT_CLASS(klass) G_TYPE_CHECK_CLASS_CAST (klass, go_combo_text_get_type (), GOComboTextClass)

enum {
	SELECTION_CHANGED,
	ENTRY_CHANGED,
	LAST_SIGNAL
};
static guint combo_text_signals [LAST_SIGNAL] = { 0 };

/**
 * go_signal_emit :
 *
 * A utility wrapper around g_signal_emitv because it does not initialize the
 * result to FALSE if there is no handler.
 */
static gboolean
go_signal_emit (GOComboText *ct, int signal,
		gconstpointer arg, int default_ret)
{
	gboolean result;
	GValue ret = { 0, };
	GValue instance_and_parm [2] = { { 0, }, { 0, } };

	g_value_init (instance_and_parm + 0, GO_TYPE_COMBO_TEXT);
	g_value_set_instance (instance_and_parm + 0, G_OBJECT (ct));

	g_value_init (instance_and_parm + 1, G_TYPE_POINTER);
	g_value_set_pointer (instance_and_parm + 1, (gpointer)arg);

	g_value_init (&ret, G_TYPE_BOOLEAN);
	g_value_set_boolean  (&ret, default_ret);

	g_signal_emitv (instance_and_parm, combo_text_signals [signal], 0, &ret);
	result = g_value_get_boolean (&ret);

	g_value_unset (instance_and_parm + 0);
	g_value_unset (instance_and_parm + 1);

	return result;
}

static void
cb_entry_activate (GtkWidget *entry, gpointer ct)
{
	char const *text = gtk_entry_get_text (GTK_ENTRY (entry));

	if (go_signal_emit (GO_COMBO_TEXT (ct), ENTRY_CHANGED, text, TRUE))
		go_combo_text_set_text (GO_COMBO_TEXT (ct), text,
			GO_COMBO_TEXT_CURRENT);
}

static void
cb_list_changed (GtkTreeView *list, gpointer data)
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection (list);
	GOComboText *ct = GO_COMBO_TEXT (data);
	GtkEntry *entry = GTK_ENTRY (ct->entry);
	gboolean accept_change;
	GtkTreeModel *store;
	GtkTreeIter iter;
	char *text;

	if (gtk_tree_selection_get_selected (selection, &store, &iter))
		gtk_tree_model_get (store, &iter, 0, &text, -1);
	else
		text = g_strdup ("");

	accept_change = TRUE;
	if (go_signal_emit (ct, SELECTION_CHANGED, selection, TRUE))
		accept_change = go_signal_emit (ct, ENTRY_CHANGED, text, TRUE);
	if (accept_change)
		gtk_entry_set_text (entry, text);

	g_free (text);

	go_combo_box_popup_hide (GO_COMBO_BOX (ct));
}

static void
cb_scroll_size_request (GtkWidget *widget, GtkRequisition *requisition,
			GOComboText *ct)
{
	GtkRequisition list_req, w_req;
	GtkAllocation allocation;
	int mon_width, mon_height, border_width;
	GdkRectangle rect;
	GdkScreen    *screen;

	/* In a Xinerama setup, use geometry of the actual display unit.  */
	screen = gtk_widget_get_screen (widget);
	if (screen == NULL)
		/* Looks like this will happen when
		 * embedded as a bonobo component */
		screen = gdk_screen_get_default ();

	gdk_screen_get_monitor_geometry (screen, 0, &rect);
	mon_width  = rect.width;
	mon_height = rect.height;
	border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));

	gtk_widget_size_request	(ct->list, &list_req);
	if (requisition->height < list_req.height) {
		int height       = list_req.height;
		GtkWidget const *w = ct->list;

		if (w != NULL) {
			/* Make room for a whole number of items which don't
			 * overflow the screen, but no more than 20. */
			int avail_height, nitems;

			gtk_widget_get_child_requisition (GTK_WIDGET (w), &w_req);
			avail_height = mon_height - 20
				- border_width * 2 + 4;
			nitems = MIN (20, avail_height * ct->rows / w_req.height);
			height = nitems *  w_req.height / ct->rows;
			if (height > list_req.height)
				height = list_req.height;
		}

		/* FIXME : Why do we need 4 ??
		 * without it things end up scrolling.
		 */
		requisition->height = height +
			border_width * 2 + 4;
	}

	gtk_widget_get_allocation (ct->entry, &allocation);
	requisition->width  = MAX (requisition->width,
				   allocation.width + border_width * 2);
	requisition->width  = MIN (requisition->width, mon_width - 20);
	requisition->height = MIN (requisition->height, mon_height - 20);
}

static void
cb_screen_changed (GOComboText *ct, GdkScreen *previous_screen)
{
	GtkWidget *w = GTK_WIDGET (ct);
	GdkScreen *screen = gtk_widget_has_screen (w)
		? gtk_widget_get_screen (w)
		: NULL;

	if (screen) {
		GtkWidget *toplevel = gtk_widget_get_toplevel (ct->scroll);
		gtk_window_set_screen (GTK_WINDOW (toplevel), screen);
	}
}

static void
go_combo_text_init (GOComboText *ct)
{
	GtkCellRenderer *renderer;
	GtkListStore *store;
	GtkTreeViewColumn *column;

	ct->rows = 0;
	ct->entry = gtk_entry_new ();
	ct->list = gtk_tree_view_new ();
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (ct->list), FALSE);
	store = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (ct->list), GTK_TREE_MODEL (store));
	g_object_unref (store);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (
								NULL,
								renderer, "text", 0, NULL);
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_append_column (GTK_TREE_VIEW (ct->list), column);
	g_signal_connect (G_OBJECT (ct->list), "cursor_changed",
			  G_CALLBACK (cb_list_changed), ct);

	ct->scroll = gtk_scrolled_window_new (NULL, NULL);

	gtk_scrolled_window_set_policy (
		GTK_SCROLLED_WINDOW (ct->scroll),
		GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport (
		GTK_SCROLLED_WINDOW (ct->scroll), ct->list);
	gtk_container_set_focus_hadjustment (
		GTK_CONTAINER (ct->list),
		gtk_scrolled_window_get_hadjustment (
			GTK_SCROLLED_WINDOW (ct->scroll)));
	gtk_container_set_focus_vadjustment (
		GTK_CONTAINER (ct->list),
		gtk_scrolled_window_get_vadjustment (
			GTK_SCROLLED_WINDOW (ct->scroll)));

	g_signal_connect (G_OBJECT (ct->entry),
		"activate",
		G_CALLBACK (cb_entry_activate), (gpointer) ct);
	g_signal_connect (G_OBJECT (ct->scroll),
		"size_request",
		G_CALLBACK (cb_scroll_size_request), (gpointer) ct);

	gtk_widget_show (ct->entry);
	go_combo_box_construct (GO_COMBO_BOX (ct),
		ct->entry, ct->scroll, ct->list);

	g_signal_connect (G_OBJECT (ct),
		"screen-changed", G_CALLBACK (cb_screen_changed),
		NULL);
}

static void
go_combo_text_destroy (GtkObject *object)
{
	GtkObjectClass *parent;
	GOComboText *ct = GO_COMBO_TEXT (object);

	if (ct->list != NULL) {
		g_signal_handlers_disconnect_by_func (G_OBJECT (ct),
			G_CALLBACK (cb_screen_changed), NULL);
		ct->list = NULL;
	}

	parent = g_type_class_peek (GO_TYPE_COMBO_BOX);
	if (parent && parent->destroy)
		(*parent->destroy) (object);
}

static void
go_combo_text_class_init (GtkObjectClass *klass)
{
	klass->destroy = &go_combo_text_destroy;

	combo_text_signals [SELECTION_CHANGED] = g_signal_new ("selection_changed",
		GO_TYPE_COMBO_TEXT,
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GOComboTextClass, selection_changed),
		NULL, NULL,
		go__BOOLEAN__POINTER,
		G_TYPE_BOOLEAN, 1, G_TYPE_POINTER);
	combo_text_signals [ENTRY_CHANGED] = g_signal_new ("entry_changed",
		GO_TYPE_COMBO_TEXT,
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (GOComboTextClass, entry_changed),
		NULL, NULL,
		go__BOOLEAN__POINTER,
		G_TYPE_BOOLEAN, 1, G_TYPE_POINTER);
}

/**
 * go_combo_text_new :
 * @cmp_func : an optional comparison routine.
 */
GtkWidget*
go_combo_text_new (GCompareFunc cmp_func)
{
	GOComboText *ct;

	if (cmp_func == NULL)
		cmp_func = &g_str_equal;

	ct = g_object_new (GO_TYPE_COMBO_TEXT, NULL);
	ct->cmp_func = cmp_func;
	return GTK_WIDGET (ct);
}

GtkWidget *
go_combo_text_new_default (void)
{
	return go_combo_text_new (NULL);
}

GSF_CLASS (GOComboText, go_combo_text,
	   go_combo_text_class_init, go_combo_text_init,
	   GO_TYPE_COMBO_BOX)

GtkWidget *
go_combo_text_get_entry (GOComboText *ct)
{
	g_return_val_if_fail (GO_IS_COMBO_TEXT (ct), NULL);

	return ct->entry;
}

/**
 * go_combo_text_set_text :
 * @ct : #GOComboText
 * @text : the label for the new item
 * @start : where to begin the search in the list.
 *
 * Returns: %TRUE if the item is found in the list.
 */
gboolean
go_combo_text_set_text (GOComboText *ct, gchar const *text,
			GOComboTextSearch start)
{
	gboolean found = FALSE, result;
	GtkTreeView *list;
	GtkTreeIter iter;
	GtkTreeModel *store;
	GtkTreeSelection *selection;
	char *label;

	g_return_val_if_fail (GO_IS_COMBO_TEXT (ct), FALSE);
	g_return_val_if_fail (text != NULL, FALSE);

	list = GTK_TREE_VIEW (ct->list);
	selection = gtk_tree_view_get_selection (list);

	/* Be careful */
	result = start != GO_COMBO_TEXT_FROM_TOP &&
				gtk_tree_selection_get_selected (selection, &store, &iter);

	if (result) {
		if (start == GO_COMBO_TEXT_NEXT)
			result = gtk_tree_model_iter_next (store, &iter);
		for (; result ; result = gtk_tree_model_iter_next (store, &iter)) {
			gtk_tree_model_get (store, &iter, 0, &label, -1);
			if (ct->cmp_func (label, text))
				break;
			g_free (label);
		}
	} else
		store = gtk_tree_view_get_model (list);

	if (!result)
		for (result = gtk_tree_model_get_iter_first (store, &iter);
				result;
				result = gtk_tree_model_iter_next (store, &iter)) {
			gtk_tree_model_get (store, &iter, 0, &label, -1);
			if (ct->cmp_func (label, text))
				break;
			g_free (label);
		}

	g_signal_handlers_block_by_func (G_OBJECT (list),
					  G_CALLBACK (cb_list_changed),
					  (gpointer) ct);
	gtk_tree_selection_unselect_all (selection);

	/* Use visible label rather than supplied text just in case */
	if (result) {
		GtkTreePath *path = gtk_tree_model_get_path (store, &iter);
		gtk_tree_selection_select_iter (selection, &iter);
		gtk_tree_view_set_cursor (GTK_TREE_VIEW (ct->list), path, NULL, FALSE);
		gtk_tree_path_free (path);
		gtk_entry_set_text (GTK_ENTRY (ct->entry), label);
		g_free (label);
		found = TRUE;
	} else
		gtk_entry_set_text (GTK_ENTRY (ct->entry), text);

	g_signal_handlers_unblock_by_func (G_OBJECT (list),
					  G_CALLBACK (cb_list_changed),
					  (gpointer) ct);
	return found;
}

/**
 * go_combo_text_add_item :
 * @ct : The text combo that will get the new element.
 * @label : the user visible label for the new item
 **/

void
go_combo_text_add_item (GOComboText *ct, char const *label)
{
	GtkListStore *store;
	GtkTreeIter iter;

	g_return_if_fail (GO_IS_COMBO_TEXT (ct));
	g_return_if_fail (label != NULL);

	store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (ct->list)));
	gtk_list_store_append (store, &iter);
	gtk_list_store_set (store, &iter, 0, label, -1);
	ct->rows++;
}
