/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * A font selector widget.  This is a simplified version of the
 * GnomePrint font selector widget.
 *
 * Authors:
 *   Jody Goldberg (jody@gnome.org)
 *   Miguel de Icaza (miguel@gnu.org)
 *   Almer S. Tigelaar (almer@gnome.org)
 */
#include <goffice/goffice-config.h>
#include <goffice/goffice.h>
#include <goffice/gtk/go-gtk-compat.h>

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

struct _GOFontSel {
	GtkHBox		box;
	GtkBuilder	*gui;

	GtkWidget	*font_name_entry;
	GtkWidget	*font_style_entry;
	GtkWidget	*font_size_entry;
	GtkTreeView	*font_name_list;
	GtkTreeView	*font_style_list;
	GtkTreeView	*font_size_list;

	GocCanvas	*font_preview_canvas;
	GocItem		*font_preview_text;

	GOFont		*base, *current;
	PangoAttrList	*modifications;

	GSList          *family_names;
	GSList          *font_sizes;
};

typedef struct {
	GtkHBoxClass parent_class;

	void (* font_changed) (GOFontSel *gfs, PangoAttrList *modfications);
} GOFontSelClass;

enum {
	FONT_CHANGED,
	LAST_SIGNAL
};

static guint gfs_signals[LAST_SIGNAL] = { 0 };
static GtkObjectClass *gfs_parent_class;

static void
go_font_sel_add_attr (GOFontSel *gfs, PangoAttribute *attr0, PangoAttribute *attr1)
{
	attr0->start_index = 0;
	attr0->end_index = -1;
	pango_attr_list_change (gfs->modifications, attr0);
	if (attr1 != NULL) {
		attr1->start_index = 0;
		attr1->end_index = -1;
		pango_attr_list_change (gfs->modifications, attr1);
	}

}
static void
go_font_sel_emit_changed (GOFontSel *gfs)
{
	g_signal_emit (G_OBJECT (gfs),
		gfs_signals [FONT_CHANGED], 0, gfs->modifications);
	goc_item_set (gfs->font_preview_text,
		"attributes",  gfs->modifications,
		NULL);
}

static void
cb_list_adjust (GtkTreeView* view)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkScrolledWindow* scroll;
	GdkRectangle rect;
	GtkAdjustment *adj;
	int pos, height, child_height;

	if (gtk_tree_selection_get_selected (gtk_tree_view_get_selection (view), &model, &iter)) {
		GtkAllocation allocation;
		GtkRequisition requisition;
		gtk_widget_get_allocation (GTK_WIDGET (view), &allocation);
		gtk_widget_get_child_requisition (GTK_WIDGET (view), &requisition);
		path = gtk_tree_model_get_path (model, &iter);
		scroll = GTK_SCROLLED_WINDOW (gtk_widget_get_parent (GTK_WIDGET (view)));
		height = allocation.height;
		child_height = requisition.height;
		if (height < child_height) {
			gtk_tree_view_get_cell_area (view, path, NULL, &rect);
			adj = gtk_scrolled_window_get_vadjustment (scroll);
			pos = gtk_adjustment_get_value (adj);
			if (rect.y < 0)
				pos += rect.y;
			else if (rect.y + rect.height > height)
				pos += rect.y + rect.height - height;
			gtk_adjustment_set_value (adj, pos);
			gtk_scrolled_window_set_vadjustment (scroll, adj);
		}
		gtk_tree_path_free (path);
	}
}

static void
list_init (GtkTreeView* view)
{
	GtkCellRenderer *renderer;
	GtkListStore *store;
	GtkTreeViewColumn *column;

	gtk_tree_view_set_headers_visible (view, FALSE);
	store = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_tree_view_set_model (view, GTK_TREE_MODEL (store));
	g_object_unref (store);
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (
			NULL, renderer, "text", 0, NULL);
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_append_column (view, column);
	g_signal_connect (view, "realize", G_CALLBACK (cb_list_adjust), NULL);
}

static void
font_selected (GtkTreeSelection *selection, GOFontSel *gfs)
{
	gchar *text;
	GtkTreeModel *model;
	GtkTreeIter   iter;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, 0, &text, -1);
		gtk_entry_set_text (GTK_ENTRY (gfs->font_name_entry), text);
		go_font_sel_add_attr (gfs, pango_attr_family_new (text), NULL);
		go_font_sel_emit_changed (gfs);
		g_free (text);
	}
}

static void
gfs_fill_font_name_list (GOFontSel *gfs)
{
	GSList *ptr;
	GtkListStore *store;
	GtkTreeIter iter;
	PangoContext *context;

/* FIXME FIXME FIXME We need to do this when we realize the widget as we don't have a screen until then. */
	context = gtk_widget_get_pango_context (GTK_WIDGET (gfs));
	gfs->family_names = go_fonts_list_families (context);

	list_init (gfs->font_name_list);
	store = GTK_LIST_STORE (gtk_tree_view_get_model (gfs->font_name_list));
	for (ptr = gfs->family_names; ptr != NULL; ptr = ptr->next) {
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, ptr->data, -1);
	}

	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (gfs->font_name_list)),
			  "changed",
			  G_CALLBACK (font_selected), gfs);
}

static char const *styles[] = {
	 N_("Normal"),
	 N_("Bold"),
	 N_("Bold italic"),
	 N_("Italic"),
	 NULL
};

static void
style_selected (GtkTreeSelection *selection,
		GOFontSel *gfs)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreePath *path;
	int row;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		path = gtk_tree_model_get_path (model, &iter);
		row = *gtk_tree_path_get_indices (path);
		gtk_tree_path_free (path);
		gtk_entry_set_text (GTK_ENTRY (gfs->font_style_entry), _(styles[row]));
		go_font_sel_add_attr (gfs,
			pango_attr_weight_new ((row == 1 || row == 2)
				?  PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL),
			pango_attr_style_new ((row == 2 || row == 3)
				? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL));
		go_font_sel_emit_changed (gfs);
	}
}

static void
gfs_fill_font_style_list (GOFontSel *gfs)
{
	 int i;
	GtkListStore *store;
	GtkTreeIter iter;

	list_init (gfs->font_style_list);
	store = GTK_LIST_STORE (gtk_tree_view_get_model (gfs->font_style_list));
	for (i = 0; styles[i] != NULL; i++) {
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, _(styles[i]), -1);
	}
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (gfs->font_style_list)),
		"changed",
		G_CALLBACK (style_selected), gfs);
}

static void
select_row (GtkTreeView *list, int row)
{
	GtkTreePath *path;

	if (row < 0)
		gtk_tree_selection_unselect_all (gtk_tree_view_get_selection (list));
	else {
		path = gtk_tree_path_new_from_indices (row, -1);

		gtk_tree_selection_select_path (gtk_tree_view_get_selection (list), path);
		if (gtk_widget_get_realized (GTK_WIDGET (list)))
			cb_list_adjust (list);
		gtk_tree_path_free (path);
	}
}

static double
size_set_text (GOFontSel *gfs, char const *size_text)
{
	char *end;
	double size;
	errno = 0; /* strtol sets errno, but does not clear it.  */
	size = strtod (size_text, &end);
	size = ((int)floor ((size * 20.) + .5)) / 20.;	/* round .05 */

	if (size_text != end && errno != ERANGE && 1. <= size && size <= 400.) {
		gtk_entry_set_text (GTK_ENTRY (gfs->font_size_entry), size_text);
		go_font_sel_add_attr (gfs,
			pango_attr_size_new (size * PANGO_SCALE), NULL);
		go_font_sel_emit_changed (gfs);
		return size;
	}
	return -1;
}

static void
size_selected (GtkTreeSelection *selection,
	       GOFontSel *gfs)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *size_text;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, 0, &size_text, -1);
		size_set_text (gfs, size_text);
		g_free (size_text);
	}
}

static void
size_changed (GtkEntry *entry, GOFontSel *gfs)
{
	double size = size_set_text (gfs, gtk_entry_get_text (entry));

	if (size > 0) {
		int psize = (int)(size * PANGO_SCALE + 0.5);
		int i = 0;
		GSList *l;

		for (l = gfs->font_sizes; l; i++, l = l->next) {
			int this_psize = GPOINTER_TO_INT (l->data);
			if (this_psize == psize)
				break;
		}
		g_signal_handlers_block_by_func (
			gtk_tree_view_get_selection (gfs->font_size_list),
			size_selected, gfs);
		select_row (gfs->font_size_list, (l ? i : -1));
		g_signal_handlers_unblock_by_func (
			gtk_tree_view_get_selection (gfs->font_size_list),
			size_selected, gfs);
	}
}

static void
gfs_fill_font_size_list (GOFontSel *gfs)
{
	GtkListStore *store;
	GtkTreeIter   iter;
	GSList       *ptr;

	gfs->font_sizes = go_fonts_list_sizes ();

	list_init (gfs->font_size_list);
	store = GTK_LIST_STORE (gtk_tree_view_get_model (gfs->font_size_list));
	for (ptr = gfs->font_sizes; ptr != NULL ; ptr = ptr->next) {
		int psize = GPOINTER_TO_INT (ptr->data);
		char *size_text = g_strdup_printf ("%g", psize / (double)PANGO_SCALE);
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, size_text, -1);
		g_free (size_text);
	}
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (gfs->font_size_list)),
		"changed",
		G_CALLBACK (size_selected), gfs);
	g_signal_connect (G_OBJECT (gfs->font_size_entry),
		"changed",
		G_CALLBACK (size_changed), gfs);
}

static void
canvas_size_changed (G_GNUC_UNUSED GtkWidget *widget,
		     GtkAllocation *allocation, GOFontSel *gfs)
{
	int width  = allocation->width - 1;
	int height = allocation->height - 1;

	goc_item_set (gfs->font_preview_text,
		"x", (double)width/2.,
		"y", (double)height/2.,
		NULL);
}

static void
gfs_init (GOFontSel *gfs)
{
	GtkWidget *w;

	gfs->gui = go_gtk_builder_new ("go-font-sel.ui", GETTEXT_PACKAGE, NULL);
	if (gfs->gui == NULL)
                return;

	gfs->modifications = pango_attr_list_new ();

	gtk_box_pack_start (GTK_BOX (gfs),
		go_gtk_builder_get_widget (gfs->gui, "toplevel-table"),
  		TRUE, TRUE, 0);
	gfs->font_name_entry  = go_gtk_builder_get_widget (gfs->gui, "font-name-entry");
	gfs->font_style_entry = go_gtk_builder_get_widget (gfs->gui, "font-style-entry");
	gfs->font_size_entry  = go_gtk_builder_get_widget (gfs->gui, "font-size-entry");
	gfs->font_name_list  = GTK_TREE_VIEW (gtk_builder_get_object (gfs->gui, "font-name-list"));
	gfs->font_style_list = GTK_TREE_VIEW (gtk_builder_get_object (gfs->gui, "font-style-list"));
	gfs->font_size_list  = GTK_TREE_VIEW (gtk_builder_get_object (gfs->gui, "font-size-list"));

	w = GTK_WIDGET (g_object_new (GOC_TYPE_CANVAS, NULL));
	gfs->font_preview_canvas = GOC_CANVAS (w);
	gtk_widget_show_all (w);
	w = go_gtk_builder_get_widget (gfs->gui, "font-preview-frame");
	gtk_container_add (GTK_CONTAINER (w), GTK_WIDGET (gfs->font_preview_canvas));

	gfs->font_preview_text = goc_item_new (
		goc_canvas_get_root (gfs->font_preview_canvas),
		GOC_TYPE_TEXT,
		NULL);
	go_font_sel_set_sample_text (gfs, NULL); /* init to default */

	g_signal_connect (G_OBJECT (gfs->font_preview_canvas),
		"size-allocate",
		G_CALLBACK (canvas_size_changed), gfs);

	gfs_fill_font_name_list (gfs);
	gfs_fill_font_style_list (gfs);
	gfs_fill_font_size_list (gfs);
}

static void
gfs_destroy (GtkObject *object)
{
	GOFontSel *gfs = GO_FONT_SEL (object);

	if (gfs->gui) {
		g_object_unref (G_OBJECT (gfs->gui));
		gfs->gui = NULL;
	}
	if (gfs->base != NULL) {
		go_font_unref (gfs->base);
		gfs->base = NULL;
	}
	if (gfs->current != NULL) {
		go_font_unref (gfs->current);
		gfs->current = NULL;
	}
	if (gfs->modifications != NULL) {
		pango_attr_list_unref (gfs->modifications);
		gfs->modifications = NULL;
	}
	if (gfs->family_names) {
		g_slist_foreach (gfs->family_names, (GFunc)g_free, NULL);
		g_slist_free (gfs->family_names);
		gfs->family_names = NULL;
	}

	g_slist_free (gfs->font_sizes);
	gfs->font_sizes = NULL;

	gfs_parent_class->destroy (object);
}

static void
gfs_class_init (GObjectClass *klass)
{
	GtkObjectClass *gto_class = (GtkObjectClass *) klass;

	gto_class->destroy = gfs_destroy;

	gfs_parent_class = g_type_class_peek (gtk_hbox_get_type ());

	gfs_signals [FONT_CHANGED] =
		g_signal_new (
			"font_changed",
			G_OBJECT_CLASS_TYPE (klass),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (GOFontSelClass, font_changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE, 1, G_TYPE_POINTER);
}

GSF_CLASS (GOFontSel, go_font_sel,
	   gfs_class_init, gfs_init, GTK_TYPE_HBOX)

GtkWidget *
go_font_sel_new (void)
{
	return g_object_new (GO_TYPE_FONT_SEL, NULL);
}

void
go_font_sel_editable_enters (GOFontSel *gfs, GtkWindow *dialog)
{
	go_gtk_editable_enters (dialog,
		GTK_WIDGET (gfs->font_name_entry));
	go_gtk_editable_enters (dialog,
		GTK_WIDGET (gfs->font_style_entry));
	go_gtk_editable_enters (dialog,
		GTK_WIDGET (gfs->font_size_entry));
}

void
go_font_sel_set_sample_text (GOFontSel *gfs, char const *text)
{
	g_return_if_fail (GO_IS_FONT_SEL (gfs));
	goc_item_set (gfs->font_preview_text,
		/* xgettext: This text is used as a sample when selecting a font
		 * please choose a translation that would produce common
		 * characters specific to the target alphabet. */
		"text",  ((text == NULL) ? _("AaBbCcDdEe12345") : text),
		NULL);
}

GOFont const *
go_font_sel_get_font (GOFontSel const *gfs)
{
	g_return_val_if_fail (GO_IS_FONT_SEL (gfs), NULL);
	return gfs->current;
}

static void
go_font_sel_set_name (GOFontSel *gfs, char const *font_name)
{
	GSList *ptr;
	int row;

	for (row = 0, ptr = gfs->family_names; ptr != NULL; ptr = ptr->next, row++)
		if (g_ascii_strcasecmp (font_name, ptr->data) == 0)
			break;
	select_row (gfs->font_name_list, (ptr != NULL) ? row : -1);
}

static void
go_font_sel_set_style (GOFontSel *gfs, gboolean is_bold, gboolean is_italic)
{
	int n;

	if (is_bold) {
		if (is_italic)
			n = 2;
		else
			n = 1;
	} else {
		if (is_italic)
			n = 3;
		else
			n = 0;
	}
	select_row (gfs->font_style_list, n);

	go_font_sel_add_attr (gfs,
		pango_attr_weight_new (is_bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL),
		pango_attr_style_new (is_italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL));
/* FIXME FIXME FIXME Do we really need the following line? */
	go_font_sel_emit_changed (gfs);
}

static void
go_font_sel_set_points (GOFontSel *gfs,
			double point_size)
{
	const char *old_text = gtk_entry_get_text (GTK_ENTRY (gfs->font_size_entry));
	char *buffer = g_strdup_printf ("%g", point_size);
	if (strcmp (old_text, buffer) != 0)
		gtk_entry_set_text (GTK_ENTRY (gfs->font_size_entry), buffer);
	g_free (buffer);
}

static void
go_font_sel_set_strike (GOFontSel *gfs, gboolean strike)
{
}

static void
go_font_sel_set_uline (GOFontSel *gfs, int uline)
{
}

static void
go_font_sel_set_color (GOFontSel *gfs, GOColor c)
{
}

void
go_font_sel_set_font (GOFontSel *gfs, GOFont const *font)
{
	g_return_if_fail (GO_IS_FONT_SEL (gfs));

	go_font_sel_set_name (gfs, pango_font_description_get_family (font->desc));
	go_font_sel_set_style (gfs,
		pango_font_description_get_weight (font->desc) >= PANGO_WEIGHT_BOLD,
		pango_font_description_get_style (font->desc) != PANGO_STYLE_NORMAL);
	go_font_sel_set_points (gfs,
		pango_font_description_get_size (font->desc) / PANGO_SCALE);
	go_font_sel_set_strike (gfs, font->strikethrough);
	go_font_sel_set_uline (gfs, font->underline);
	go_font_sel_set_color (gfs, font->color);
}
