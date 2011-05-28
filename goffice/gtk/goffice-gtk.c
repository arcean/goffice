/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * goffice-gtk.c: Misc gtk utilities
 *
 * Copyright (C) 2004 Jody Goldberg (jody@gnome.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#include <goffice/goffice-config.h>
#include <goffice/goffice.h>
#include <goffice/goffice-priv.h>
#include <goffice/gtk/go-gtk-compat.h>

#include <gdk/gdkkeysyms.h>
#include <atk/atkrelation.h>
#include <atk/atkrelationset.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gsf/gsf-input-stdio.h>
#include <gsf/gsf-input-textline.h>

#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>

#define PREVIEW_HSIZE 150
#define PREVIEW_VSIZE 150

/* ------------------------------------------------------------------------- */


/**
 * go_gtk_button_new_with_stock:
 * @text : button label
 * @stock_id : id for stock icon
 *
 * FROM : gedit
 * Creates a new GtkButton with custom label and stock image.
 *
 * Returns: newly created button
 **/
GtkWidget*
go_gtk_button_new_with_stock (char const *text, char const* stock_id)
{
	GtkStockItem item;
	GtkWidget *button = gtk_button_new_with_mnemonic (text);
	if (gtk_stock_lookup (stock_id, &item))
		gtk_button_set_image (GTK_BUTTON (button),
			gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON));
	return button;
}

/**
 * go_gtk_dialog_add_button :
 * @dialog: dialog you want to add a button
 * @text: button label
 * @stock_id: stock icon id
 * @response_id: respond id when button clicked
 *
 * FROM : gedit
 * Creates and adds a button with stock image to the action area of an existing dialog.
 *
 * Returns: newly created button
 **/
GtkWidget*
go_gtk_dialog_add_button (GtkDialog *dialog, char const* text, char const* stock_id,
			  gint response_id)
{
	GtkWidget *button;

	g_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);
	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (stock_id != NULL, NULL);

	button = go_gtk_button_new_with_stock (text, stock_id);
	g_return_val_if_fail (button != NULL, NULL);

	gtk_widget_set_can_default (button, TRUE);

	gtk_widget_show (button);

	gtk_dialog_add_action_widget (dialog, button, response_id);

	return button;
}

/**
 * go_gtk_builder_new :
 * @uifile : the name of the file load
 * @domain : the translation domain
 * @gcc : #GOCmdContext
 *
 * Simple utility to open ui files
 *
 * Returns: a new #GtkBuilder or NULL
 **/
GtkBuilder *
go_gtk_builder_new (char const *uifile,
		    char const *domain, GOCmdContext *gcc)
{
	GtkBuilder *gui;
	char *f;
	GError *error = NULL;

	g_return_val_if_fail (uifile != NULL, NULL);

	if (!g_path_is_absolute (uifile))
		f = g_build_filename (go_sys_data_dir (), "ui", uifile, NULL);
	else
		f = g_strdup (uifile);

	gui = gtk_builder_new ();
	if (domain)
		gtk_builder_set_translation_domain (gui, domain);
	if (!gtk_builder_add_from_file (gui, f, &error)) {
		g_object_unref (gui);
		gui = NULL;
	}
	if (gui == NULL && gcc != NULL) {
		char *msg;
		if (error) {
			msg = g_strdup (error->message);
			g_error_free (error);
		} else
			msg = g_strdup_printf (_("Unable to open file '%s'"), f);
		go_cmd_context_error_system (gcc, msg);
		g_free (msg);
	} else if (error)
		g_error_free (error);
	g_free (f);

	return gui;
}

/**
 * go_gtk_builder_signal_connect :
 * @gui : #GtkBuilder
 * @instance_name : widget name
 * @detailed_signal : signal name
 * @c_handler : #GCallback
 * @data : arbitrary
 *
 * Convenience wrapper around g_signal_connect for GtkBuilder.
 *
 * Returns: The signal id
 **/
gulong
go_gtk_builder_signal_connect	(GtkBuilder	*gui,
			 gchar const	*instance_name,
			 gchar const	*detailed_signal,
			 GCallback	 c_handler,
			 gpointer	 data)
{
	GObject *obj;
	g_return_val_if_fail (gui != NULL, 0);
	obj = gtk_builder_get_object (gui, instance_name);
	g_return_val_if_fail (obj != NULL, 0);
	return g_signal_connect (obj, detailed_signal, c_handler, data);
}

/**
 * go_xml_builder_signal_connect_swapped :
 * @gui : #GtkBuilder
 * @instance_name : widget name
 * @detailed_signal : signal name
 * @c_handler : #GCallback
 * @data : arbitary
 *
 * Convenience wrapper around g_signal_connect_swapped for GtkBuilder.
 *
 * Returns: The signal id
 **/
gulong
go_gtk_builder_signal_connect_swapped (GtkBuilder	*gui,
				 gchar const	*instance_name,
				 gchar const	*detailed_signal,
				 GCallback	 c_handler,
				 gpointer	 data)
{
	GObject *obj;
	g_return_val_if_fail (gui != NULL, 0);
	obj = gtk_builder_get_object (gui, instance_name);
	g_return_val_if_fail (obj != NULL, 0);
	return g_signal_connect_swapped (obj, detailed_signal, c_handler, data);
}

/**
 * go_gtk_builder_get_widget :
 * @gui : the #GtkBuilder
 * @widget_name : the name of the combo box in the ui file.
 *
 * Simple wrapper to #gtk_builder_get_object which returns the object
 * as a GtkWidget.
 *
 * Returns: a new #GtkWidget or NULL
 **/
GtkWidget *
go_gtk_builder_get_widget (GtkBuilder *gui, char const *widget_name)
{
	return GTK_WIDGET (gtk_builder_get_object (gui, widget_name));
}

/**
 * go_gtk_builder_combo_box_init_text :
 * @gui : the #GtkBuilder
 * @widget_name : the name of the combo box in the ui file.
 *
 * searches the #GtkComboBox in @gui and ensures it has a model and a
 * renderer appropriate for using with #gtk_combo_box_append_text and friends.
 *
 * Returns: the #GtkComboBox or NULL
 **/
GtkComboBox *
go_gtk_builder_combo_box_init_text (GtkBuilder *gui, char const *widget_name)
{
	GtkComboBox *box;
	GList *cells;
	g_return_val_if_fail (gui != NULL, NULL);
	box = GTK_COMBO_BOX (gtk_builder_get_object (gui, widget_name));
	/* search for the model and create one if none exists */
	g_return_val_if_fail (box != NULL, NULL);
	if (gtk_combo_box_get_model (box) == NULL) {
		GtkListStore *store = gtk_list_store_new (1, G_TYPE_STRING);
		gtk_combo_box_set_model (box, GTK_TREE_MODEL (store));
		g_object_unref (store);
	}
	cells = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT (box));
	if (g_list_length (cells) == 0) {
		/* populate the cell layout */
		GtkCellRenderer *cell = gtk_cell_renderer_text_new ();
		gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (box), cell, TRUE);
		gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (box), cell,
						"text", 0, NULL);
	}
	g_list_free (cells);
	return box;
}

int
go_gtk_builder_group_value (GtkBuilder *gui, char const * const group[])
{
	int i;
	for (i = 0; group[i]; i++) {
		GtkWidget *w = go_gtk_builder_get_widget (gui, group[i]);
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
			return i;
	}
	return -1;
}

/*
 * A variant of gtk_window_activate_default that does not end up reactivating
 * the widget that [Enter] was pressed in.
 */
static void
cb_activate_default (GtkWindow *window)
{
	GtkWidget *w = gtk_window_get_default_widget (window);
	if (w && gtk_widget_is_sensitive (w))
		gtk_widget_activate (w);
}

/**
 * go_gtk_editable_enters:
 * @window: #GtkWindow
 * @w:  #GtkWidget
 *
 * Normally if there's an editable widget (such as #GtkEntry) in your
 * dialog, pressing Enter will activate the editable rather than the
 * default dialog button. However, in most cases, the user expects to
 * type something in and then press enter to close the dialog. This
 * function enables that behavior.
 **/
void
go_gtk_editable_enters (GtkWindow *window, GtkWidget *w)
{
	g_return_if_fail (GTK_IS_WINDOW (window));
	g_signal_connect_swapped (G_OBJECT (w),
		"activate",
		G_CALLBACK (cb_activate_default), window);
}

/**
 * go_gtk_widget_disable_focus :
 * @w : #GtkWidget
 *
 * Convenience wrapper to disable focus on a container and it's children.
 **/
void
go_gtk_widget_disable_focus (GtkWidget *w)
{
	if (GTK_IS_CONTAINER (w))
		gtk_container_foreach (GTK_CONTAINER (w),
			(GtkCallback) go_gtk_widget_disable_focus, NULL);
	gtk_widget_set_can_focus (w, FALSE);
}

/**
 * go_pango_measure_string :
 * @context : #PangoContext
 * @font_desc : #PangoFontDescription
 * @str : The text to measure.
 *
 * A utility function to measure text.
 *
 * Returns: the pixel length of @str according to @context.
 **/
int
go_pango_measure_string (PangoContext *context, PangoFontDescription const *font_desc, char const *str)
{
	PangoLayout *layout = pango_layout_new (context);
	int width;

	pango_layout_set_text (layout, str, -1);
	pango_layout_set_font_description (layout, font_desc);
	pango_layout_get_pixel_size (layout, &width, NULL);

	g_object_unref (layout);

	return width;
}

static void
cb_parent_mapped (GtkWidget *parent, GtkWindow *window)
{
	if (gtk_widget_get_mapped (GTK_WIDGET (window))) {
		gtk_window_present (window);
		g_signal_handlers_disconnect_by_func (G_OBJECT (parent),
			G_CALLBACK (cb_parent_mapped), window);
	}
}

/**
 * go_gtk_window_set_transient
 * @toplevel	: The calling window
 * @window      : the transient window
 *
 * Make the window a child of the workbook in the command context, if there is
 * one.  The function duplicates the positioning functionality in
 * gnome_dialog_set_parent, but does not require the transient window to be
 * a GnomeDialog.
 **/
void
go_gtk_window_set_transient (GtkWindow *toplevel, GtkWindow *window)
{
#if 0
	GtkWindowPosition position = gnome_preferences_get_dialog_position(); */
	if (position == GTK_WIN_POS_NONE)
		position = GTK_WIN_POS_CENTER_ON_PARENT;
#else
	GtkWindowPosition position = GTK_WIN_POS_CENTER_ON_PARENT;
#endif

	g_return_if_fail (GTK_IS_WINDOW (toplevel));
	g_return_if_fail (GTK_IS_WINDOW (window));

	gtk_window_set_transient_for (window, toplevel);

	gtk_window_set_position (window, position);

	if (!gtk_widget_get_mapped (GTK_WIDGET (toplevel)))
		g_signal_connect_after (G_OBJECT (toplevel),
			"map",
			G_CALLBACK (cb_parent_mapped), window);
}

static gint
cb_non_modal_dialog_keypress (GtkWidget *w, GdkEventKey *e)
{
	if(e->keyval == GDK_Escape) {
		gtk_widget_destroy (w);
		return TRUE;
	}

	return FALSE;
}

/**
 * go_gtk_nonmodal_dialog :
 * @toplevel : #GtkWindow
 * @dialog : #GtkWindow
 *
 * Utility to set @dialog as a transient of @toplevel
 * and to set up a handler for "Escape"
 **/
void
go_gtk_nonmodal_dialog (GtkWindow *toplevel, GtkWindow *dialog)
{
	go_gtk_window_set_transient (toplevel, dialog);
	g_signal_connect (G_OBJECT (dialog),
		"key-press-event",
		G_CALLBACK (cb_non_modal_dialog_keypress), NULL);
}

static void
fsel_response_cb (GtkFileChooser *dialog,
		  gint response_id,
		  gboolean *result)
{
	if (response_id == GTK_RESPONSE_OK) {
		char *uri = gtk_file_chooser_get_uri (dialog);

		if (uri) {
			g_free (uri);
			*result = TRUE;
		}
	}

	gtk_main_quit ();
}

static gint
gu_delete_handler (GtkDialog *dialog,
		   GdkEventAny *event,
		   gpointer data)
{
	gtk_dialog_response (dialog, GTK_RESPONSE_CANCEL);
	return TRUE; /* Do not destroy */
}

/**
 * go_gtk_file_sel_dialog :
 * @toplevel : #GtkWindow
 * @w : #GtkWidget
 *
 * Runs a modal dialog to select a file.
 *
 * Returns: %TRUE if a file was selected.
 **/
gboolean
go_gtk_file_sel_dialog (GtkWindow *toplevel, GtkWidget *w)
{
	gboolean result = FALSE;
	gulong delete_handler;

	g_return_val_if_fail (GTK_IS_WINDOW (toplevel), FALSE);
	g_return_val_if_fail (GTK_IS_FILE_CHOOSER (w), FALSE);

	gtk_window_set_modal (GTK_WINDOW (w), TRUE);
	go_gtk_window_set_transient (toplevel, GTK_WINDOW (w));
	g_signal_connect (w, "response",
		G_CALLBACK (fsel_response_cb), &result);
	delete_handler = g_signal_connect (w, "delete_event",
		G_CALLBACK (gu_delete_handler), NULL);

	gtk_widget_show (w);
	gtk_grab_add (w);
	gtk_main ();

	g_signal_handler_disconnect (w, delete_handler);

	return result;
}

static gboolean have_pixbufexts = FALSE;
static GSList *pixbufexts = NULL;  /* FIXME: we leak this.  */

static gboolean
filter_images (const GtkFileFilterInfo *filter_info, gpointer data)
{
	if (filter_info->mime_type)
		return strncmp (filter_info->mime_type, "image/", 6) == 0;

	if (filter_info->display_name) {
		GSList *l;
		char const *ext = strrchr (filter_info->display_name, '.');
		if (!ext) return FALSE;
		ext++;

		if (!have_pixbufexts) {
			GSList *l, *pixbuf_fmts = gdk_pixbuf_get_formats ();

			for (l = pixbuf_fmts; l != NULL; l = l->next) {
				GdkPixbufFormat *fmt = l->data;
				gchar **support_exts = gdk_pixbuf_format_get_extensions (fmt);
				int i;

				for (i = 0; support_exts[i]; ++i)
					pixbufexts = g_slist_prepend (pixbufexts,
								      support_exts[i]);
				/*
				 * Use g_free here because the strings have been
				 * taken by pixbufexts.
				 */
				g_free (support_exts);
			}

			g_slist_free (pixbuf_fmts);
			have_pixbufexts = TRUE;
		}

		for (l = pixbufexts; l != NULL; l = l->next)
			if (g_ascii_strcasecmp (l->data, ext) == 0)
				return TRUE;
	}

	return FALSE;
}

static void
update_preview_cb (GtkFileChooser *chooser)
{
	gchar *filename = gtk_file_chooser_get_preview_filename (chooser);
	GtkWidget *label = g_object_get_data (G_OBJECT (chooser), "label-widget");
	GtkWidget *image = g_object_get_data (G_OBJECT (chooser), "image-widget");

	if (filename == NULL) {
		gtk_widget_hide (image);
		gtk_widget_hide (label);
	} else if (g_file_test (filename, G_FILE_TEST_IS_DIR)) {
		/* Not quite sure what to do here.  */
		gtk_widget_hide (image);
		gtk_widget_hide (label);
	} else {
		GdkPixbuf *buf;
		gboolean dummy;

		buf = gdk_pixbuf_new_from_file (filename, NULL);
		if (buf) {
			dummy = FALSE;
		} else {
			GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (chooser));
			buf = gtk_icon_theme_load_icon
				(gtk_icon_theme_get_for_screen (screen),
				 "unknown_image", 100, 100, NULL);
			dummy = buf != NULL;
		}

		if (buf) {
			GdkPixbuf *pixbuf = go_pixbuf_intelligent_scale (buf, PREVIEW_HSIZE, PREVIEW_VSIZE);
			gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);
			g_object_unref (pixbuf);
			gtk_widget_show (image);

			if (dummy)
				gtk_label_set_text (GTK_LABEL (label), "");
			else {
				int w = gdk_pixbuf_get_width (buf);
				int h = gdk_pixbuf_get_height (buf);
				char *size = g_strdup_printf (_("%d x %d"), w, h);
				gtk_label_set_text (GTK_LABEL (label), size);
				g_free (size);
			}
			gtk_widget_show (label);

			g_object_unref (buf);
		}

		g_free (filename);
	}
}

static GtkFileChooser *
gui_image_chooser_new (gboolean is_save)
{
	GtkFileChooser *fsel;

/* Use Maemo File Chooser */
	fsel = GTK_FILE_CHOOSER (hildon_file_chooser_dialog_new (NULL,is_save ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN));
/*
	fsel = GTK_FILE_CHOOSER
		(g_object_new (GTK_TYPE_FILE_CHOOSER_DIALOG,
			       "action", is_save ? GTK_FILE_CHOOSER_ACTION_SAVE : GTK_FILE_CHOOSER_ACTION_OPEN,
			       "local-only", FALSE,
			       "use-preview-label", FALSE,
			       NULL));
	gtk_dialog_add_buttons (GTK_DIALOG (fsel),
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				is_save ? GTK_STOCK_SAVE : GTK_STOCK_OPEN,
				GTK_RESPONSE_OK,
				NULL);
*/

	gtk_dialog_set_default_response (GTK_DIALOG (fsel), GTK_RESPONSE_OK);
	/* Filters */
	{
		GtkFileFilter *filter;

		filter = gtk_file_filter_new ();
		gtk_file_filter_set_name (filter, _("All Files"));
		gtk_file_filter_add_pattern (filter, "*");
		gtk_file_chooser_add_filter (fsel, filter);

		filter = gtk_file_filter_new ();
		gtk_file_filter_set_name (filter, _("Images"));
		gtk_file_filter_add_custom (filter, GTK_FILE_FILTER_MIME_TYPE,
					    filter_images, NULL, NULL);
		gtk_file_chooser_add_filter (fsel, filter);
		/* Make this filter the default */
		gtk_file_chooser_set_filter (fsel, filter);
	}

	/* Preview *//*
	{
		GtkWidget *vbox = gtk_vbox_new (FALSE, 2);
		GtkWidget *preview_image = gtk_image_new ();
		GtkWidget *preview_label = gtk_label_new ("");

		g_object_set_data (G_OBJECT (fsel), "image-widget", preview_image);
		g_object_set_data (G_OBJECT (fsel), "label-widget", preview_label);

		gtk_widget_set_size_request (vbox, PREVIEW_HSIZE, -1);

		gtk_box_pack_start (GTK_BOX (vbox), preview_image, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), preview_label, FALSE, FALSE, 0);
		gtk_file_chooser_set_preview_widget (fsel, vbox);
		g_signal_connect (fsel, "update-preview",
				  G_CALLBACK (update_preview_cb), NULL);
		update_preview_cb (fsel);
	}
*/
	return fsel;
}

char *
go_gtk_select_image (GtkWindow *toplevel, char const *initial)
{
	char const *key = "go_gtk_select_image";
	char *uri = NULL;
	GtkFileChooser *fsel;

	g_return_val_if_fail (GTK_IS_WINDOW (toplevel), NULL);

	fsel = gui_image_chooser_new (FALSE);

	if (!initial)
		initial = g_object_get_data (G_OBJECT (toplevel), key);
	if (initial)
		gtk_file_chooser_set_uri (fsel, initial);
	g_object_set (G_OBJECT (fsel), "title", _("Select an Image"), NULL);

	/* Show file selector */
	if (go_gtk_file_sel_dialog (toplevel, GTK_WIDGET (fsel))) {
		uri = gtk_file_chooser_get_uri (fsel);
		g_object_set_data_full (G_OBJECT (toplevel),
					key, g_strdup (uri),
					g_free);
	}
	gtk_widget_destroy (GTK_WIDGET (fsel));
	return uri;
}

typedef struct {
	char *uri;
	double resolution;
	gboolean is_expanded;
	GOImageFormat format;
} SaveInfoState;

static void
save_info_state_free (SaveInfoState *state)
{
	g_free (state->uri);
	g_free (state);
}

static void
cb_format_combo_changed (GtkComboBox *combo, GtkWidget *expander)
{
	GOImageFormatInfo const *format_info;

	format_info = go_image_get_format_info (gtk_combo_box_get_active (combo));
	gtk_widget_set_sensitive (expander,
				  format_info != NULL &&
				  format_info->is_dpi_useful);
}

/**
 * go_gui_get_image_save_info:
 * @toplevel: a #GtkWindow
 * @supported_formats: a #GSList of supported file formats
 * @ret_format: default file format
 * @resolution: export resolution
 *
 * Opens a file chooser and let user choose file URI and format in a list of
 * supported ones.
 *
 * Returns: file URI string, file #GOImageFormat stored in @ret_format, and
 * 	export resolution in @resolution.
 **/
char *
go_gui_get_image_save_info (GtkWindow *toplevel, GSList *supported_formats,
			    GOImageFormat *ret_format, double *resolution)
{
	GOImageFormat format;
	GOImageFormatInfo const *format_info;
	GtkComboBox *format_combo = NULL;
	GtkFileChooser *fsel = gui_image_chooser_new (TRUE);
	GtkWidget *expander = NULL;
	GtkWidget *resolution_spin = NULL;
	GtkWidget *resolution_table;
	GtkBuilder *gui;
	SaveInfoState *state;
	char const *key = "go_gui_get_image_save_info";
	char *uri = NULL;

	state = g_object_get_data (G_OBJECT (toplevel), key);
	if (state == NULL) {
		state = g_new (SaveInfoState, 1);
		g_return_val_if_fail (state != NULL, NULL);
		state->uri = NULL;
		state->resolution = 150.0;
		state->is_expanded = FALSE;
		state->format = GO_IMAGE_FORMAT_SVG;
		g_object_set_data_full (G_OBJECT (toplevel), key,
					state, (GDestroyNotify) save_info_state_free);
	}	       

	g_object_set (G_OBJECT (fsel), "title", _("Save as"), NULL);

	gui = go_gtk_builder_new ("go-image-save-dialog-extra.ui",
			       GETTEXT_PACKAGE, NULL);
	if (gui != NULL) {
		GtkWidget *widget;

		/* Format selection UI */
		if (supported_formats != NULL && ret_format != NULL) {
			int i;
			GSList *l;
			format_combo = go_gtk_builder_combo_box_init_text (gui, "format_combo");
			for (l = supported_formats, i = 0; l != NULL; l = l->next, i++) {
				format = GPOINTER_TO_UINT (l->data);
				format_info = go_image_get_format_info (format);
				gtk_combo_box_append_text (format_combo, _(format_info->desc));
				if (format == state->format)
					gtk_combo_box_set_active (format_combo, i);
			}
			if (gtk_combo_box_get_active (format_combo) < 0)
				gtk_combo_box_set_active (format_combo, 0);
		} else {
			widget = go_gtk_builder_get_widget (gui, "file_type_box");
			gtk_widget_hide (widget);
		}

		if (supported_formats != NULL && ret_format != NULL) {
			widget = go_gtk_builder_get_widget (gui, "image_save_dialog_extra");
			/* Maemo5 specific changes */
			gtk_widget_show_all(widget);
			gtk_box_pack_start (GTK_BOX (GTK_DIALOG (fsel)->vbox), widget, FALSE, TRUE, 6);
			
		}

		/* Export setting expander */
		expander = go_gtk_builder_get_widget (gui, "export_expander");
		if (resolution != NULL) {
			gtk_expander_set_expanded (GTK_EXPANDER (expander), state->is_expanded);
			resolution_spin = go_gtk_builder_get_widget (gui, "resolution_spin");
			gtk_spin_button_set_value (GTK_SPIN_BUTTON (resolution_spin), state->resolution);
		} else
			gtk_widget_hide (expander);

		if (resolution != NULL && supported_formats != NULL && ret_format != NULL) {
			resolution_table = go_gtk_builder_get_widget (gui, "resolution_table");

			cb_format_combo_changed (format_combo, resolution_table);
			g_signal_connect (GTK_WIDGET (format_combo), "changed",
					  G_CALLBACK (cb_format_combo_changed), resolution_table);
		}

		g_object_unref (G_OBJECT (gui));
	}

	if (state->uri != NULL) {
		gtk_file_chooser_set_uri (fsel, state->uri);
		gtk_file_chooser_unselect_all (fsel);
	}

	/* Show file selector */
 loop:
	if (!go_gtk_file_sel_dialog (toplevel, GTK_WIDGET (fsel)))
		goto out;
	uri = gtk_file_chooser_get_uri (fsel);
	if (format_combo) {
		char *new_uri = NULL;

		format = GPOINTER_TO_UINT (g_slist_nth_data
			(supported_formats,
			 gtk_combo_box_get_active (format_combo)));
		format_info = go_image_get_format_info (format);
		if (!go_url_check_extension (uri, format_info->ext, &new_uri) &&
		    !go_gtk_query_yes_no (GTK_WINDOW (fsel), TRUE,
		     _("The given file extension does not match the"
		       " chosen file type. Do you want to use this name"
		       " anyway?"))) {
			g_free (new_uri);
			g_free (uri);
			uri = NULL;
			goto loop;
		} else {
			g_free (uri);
			uri = new_uri;
		}
		*ret_format = format;
	}
	if (!go_gtk_url_is_writeable (GTK_WINDOW (fsel), uri, TRUE)) {
		g_free (uri);
		uri = NULL;
		goto loop;
	}
 out:
	if (uri != NULL && ret_format != NULL) {
		g_free (state->uri);
		state->uri = g_strdup (uri);
		state->format = *ret_format;
		if (resolution != NULL) {
			state->is_expanded = gtk_expander_get_expanded (GTK_EXPANDER (expander));
			*resolution =  gtk_spin_button_get_value (GTK_SPIN_BUTTON (resolution_spin));
			state->resolution = *resolution;
		}
	}

	gtk_widget_destroy (GTK_WIDGET (fsel));

	return uri;
}

static void
add_atk_relation (GtkWidget *w0, GtkWidget *w1, AtkRelationType type)
{
	AtkObject *atk0 = gtk_widget_get_accessible(w0);
	AtkObject *atk1 = gtk_widget_get_accessible(w1);
	AtkRelationSet *relation_set = atk_object_ref_relation_set (atk0);
	AtkRelation *relation = atk_relation_new (&atk1, 1, type);
	atk_relation_set_add (relation_set, relation);
	g_object_unref (relation_set);
	g_object_unref (relation);
}

/**
 * go_atk_setup_label :
 * @label : #GtkWidget
 * @target : #GtkWidget
 *
 * A convenience routine to setup label-for/labeled-by relationship between a
 * pair of widgets
 **/
void
go_atk_setup_label (GtkWidget *label, GtkWidget *target)
{
	 add_atk_relation (label, target, ATK_RELATION_LABEL_FOR);
	 add_atk_relation (target, label, ATK_RELATION_LABELLED_BY);
}

typedef struct {
	char const *data_dir;
	char const *app;
	char const *link;
} CBHelpPaths;

#ifdef G_OS_WIN32
#include <windows.h>

typedef HWND (* WINAPI HtmlHelpA_t) (HWND hwndCaller,
				     LPCSTR pszFile,
				     UINT uCommand,
				     DWORD_PTR dwData);
#define HH_HELP_CONTEXT 0x000F

static HtmlHelpA_t
html_help (void)
{
	HMODULE hhctrl;
	static HtmlHelpA_t result = NULL;
	static gboolean beenhere = FALSE;

	if (!beenhere) {
		hhctrl = LoadLibrary ("hhctrl.ocx");
		if (hhctrl != NULL)
			result = (HtmlHelpA_t) GetProcAddress (hhctrl, "HtmlHelpA");
		beenhere = TRUE;
	}

	return result;
}

#endif
static void
go_help_display (CBHelpPaths const *paths)
{
#if defined(G_OS_WIN32)
	static GHashTable* context_help_map = NULL;
	guint id;

	if (!context_help_map) {
		GsfInput *input;
		GsfInputTextline *textline;
		GError *err = NULL;
		gchar *mapfile = g_strconcat (paths->app, ".hhmap", NULL);
		gchar *path = g_build_filename (paths->data_dir, "doc", "C", mapfile, NULL);

		g_free (mapfile);

		if (!(input = gsf_input_stdio_new (path, &err)) ||
		    !(textline = (GsfInputTextline *) gsf_input_textline_new (input)))
			go_gtk_notice_dialog (NULL, GTK_MESSAGE_ERROR, "Cannot open '%s': %s",
					      path, err ? err->message :
							  "failed to create gsf-input-textline");
		else {
			gchar *line, *topic;
			gulong id, i = 1;
			context_help_map = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

			while ((line = gsf_input_textline_utf8_gets (textline)) != NULL) {
				if (!(id = strtoul (line, &topic, 10))) {
					go_gtk_notice_dialog (NULL, GTK_MESSAGE_ERROR,
							      "Invalid topic id at line %lu in %s: '%s'",
							      i, path, line);
					continue;
				}
				for (; *topic == ' ' || *topic == '\t'; ++topic);
				g_hash_table_insert (context_help_map, g_strdup (topic),
					(gpointer)id);
			}
			g_object_unref (G_OBJECT (textline));
		}
		if (input)
			g_object_unref (G_OBJECT (input));
		g_free (path);
	}

	if (!(id = (guint) g_hash_table_lookup (context_help_map, paths->link)))
		go_gtk_notice_dialog (NULL, GTK_MESSAGE_ERROR, "Topic '%s' not found.",
				      paths->link);
	else {
		gchar *chmfile = g_strconcat (paths->app, ".chm", NULL);
		gchar *path = g_build_filename (paths->data_dir, "doc", "C", chmfile, NULL);

		g_free (chmfile);
		if (html_help () == NULL ||
		    !(html_help ()) (GetDesktopWindow (), path, HH_HELP_CONTEXT, id))
			go_gtk_notice_dialog (NULL, GTK_MESSAGE_ERROR, "Failed to load HtmlHelp");
		g_free (path);
	}
#else
	char *uri = g_strconcat ("ghelp:", paths->app, "#", paths->link, NULL);
	go_url_show (uri);
	g_free (uri);
	return;
#endif
}

static void
cb_help (CBHelpPaths const *paths)
{
	go_help_display (paths);
}

void
go_gtk_help_button_init (GtkWidget *w, char const *data_dir, char const *app, char const *link)
{
	CBHelpPaths *paths = g_new (CBHelpPaths, 1);
	GtkWidget *parent = gtk_widget_get_parent (w);

	if (GTK_IS_BUTTON_BOX (parent))
		gtk_button_box_set_child_secondary (
			GTK_BUTTON_BOX (parent), w, TRUE);

	paths->data_dir = data_dir;
	paths->app	= app;
	paths->link	= link;
	g_signal_connect_data (G_OBJECT (w), "clicked",
		G_CALLBACK (cb_help), (gpointer) paths,
		(GClosureNotify)g_free, G_CONNECT_SWAPPED);
}

/**
 * go_gtk_url_is_writeable:
 * @parent : #GtkWindow
 * @uri : the uri to test.
 * @overwrite_by_default : gboolean
 *
 * Check if it makes sense to try saving.
 * If it's an existing file and writable for us, ask if we want to overwrite.
 * We check for other problems, but if we miss any, the saver will report.
 * So it doesn't have to be bulletproof.
 *
 * FIXME: The message boxes should really be children of the file selector,
 * not the workbook.
 *
 * Returns: %TRUE if @url is writable
 **/
gboolean
go_gtk_url_is_writeable (GtkWindow *parent, char const *uri,
			 gboolean overwrite_by_default)
{
	gboolean result = TRUE;
	char *filename;

	if (uri == NULL || uri[0] == '\0')
		result = FALSE;

	filename = go_filename_from_uri (uri);
	if (!filename)
		return TRUE;  /* Just assume writable.  */

	if (G_IS_DIR_SEPARATOR (filename [strlen (filename) - 1]) ||
	    g_file_test (filename, G_FILE_TEST_IS_DIR)) {
		go_gtk_notice_dialog (parent, GTK_MESSAGE_ERROR,
				      _("%s\nis a directory name"), uri);
		result = FALSE;
	} else if (go_file_access (uri, W_OK) != 0 && errno != ENOENT) {
		go_gtk_notice_dialog (parent, GTK_MESSAGE_ERROR,
				      _("You do not have permission to save to\n%s"),
				      uri);
		result = FALSE;
	} else if (g_file_test (filename, G_FILE_TEST_EXISTS)) {
		char *dirname = go_dirname_from_uri (uri, TRUE);
		char *basename = go_basename_from_uri (uri);
		GtkWidget *dialog = gtk_message_dialog_new_with_markup (parent,
			 GTK_DIALOG_DESTROY_WITH_PARENT,
			 GTK_MESSAGE_WARNING,
			 GTK_BUTTONS_OK_CANCEL,
			 _("A file called <i>%s</i> already exists in %s.\n\n"
			   "Do you want to save over it?"),
			 basename, dirname);
		gtk_dialog_set_default_response (GTK_DIALOG (dialog),
			overwrite_by_default ? GTK_RESPONSE_OK : GTK_RESPONSE_CANCEL);
		result = GTK_RESPONSE_OK ==
			go_gtk_dialog_run (GTK_DIALOG (dialog), parent);
		g_free (dirname);
		g_free (basename);
	}

	g_free (filename);
	return result;
}

/**
 * go_gtk_dialog_run:
 * @dialog : #GtkDialog
 * @parent : #GtkWindow
 *
 * Pop up a dialog as child of a window.
 *
 * Returns: result ID.
 **/
gint
go_gtk_dialog_run (GtkDialog *dialog, GtkWindow *parent)
{
	gint      result;

	g_return_val_if_fail (GTK_IS_DIALOG (dialog), GTK_RESPONSE_NONE);

	if (parent) {
		g_return_val_if_fail (GTK_IS_WINDOW (parent), GTK_RESPONSE_NONE);

		go_gtk_window_set_transient (parent, GTK_WINDOW (dialog));
	}

	g_object_ref (dialog);

	while ((result = gtk_dialog_run (dialog)) >= 0)
	       ;
	gtk_widget_destroy (GTK_WIDGET (dialog));

	g_object_unref (dialog);

	return result;
}

#define _VPRINTF_MESSAGE(format,args,msg) va_start (args, format); \
					  msg = g_strdup_vprintf (format, args); \
					  va_end (args);
#define VPRINTF_MESSAGE(format,args,msg) _VPRINTF_MESSAGE (format, args, msg); \
					 g_return_if_fail (msg != NULL);

/*
 * TODO:
 * Get rid of trailing newlines /whitespace.
 */
void
go_gtk_notice_dialog (GtkWindow *parent, GtkMessageType type,
		      char const *format, ...)
{
	va_list args;
	gchar *msg;
	GtkWidget *dialog;

	VPRINTF_MESSAGE (format, args, msg);
	dialog = gtk_message_dialog_new_with_markup (parent,
		GTK_DIALOG_DESTROY_WITH_PARENT, type, GTK_BUTTONS_OK, "%s", msg);
	g_free (msg);
	go_gtk_dialog_run (GTK_DIALOG (dialog), parent);
}

void
go_gtk_notice_nonmodal_dialog (GtkWindow *parent, GtkWidget **ref,
			       GtkMessageType type, char const *format, ...)
{
	va_list args;
	gchar *msg;
	GtkWidget *dialog;

	if (*ref != NULL)
		gtk_widget_destroy (*ref);

	VPRINTF_MESSAGE (format, args, msg);
	*ref = dialog = gtk_message_dialog_new (parent, GTK_DIALOG_DESTROY_WITH_PARENT, type,
					 GTK_BUTTONS_OK, "%s", msg);
	g_free (msg);

	g_signal_connect_object (G_OBJECT (dialog),
		"response",
		G_CALLBACK (gtk_widget_destroy), G_OBJECT (dialog), 0);
	g_signal_connect (G_OBJECT (dialog),
		"destroy",
		G_CALLBACK (gtk_widget_destroyed), ref);

	gtk_widget_show (dialog);
}

gboolean
go_gtk_query_yes_no (GtkWindow *parent, gboolean default_answer,
		     char const *format, ...)
{
	va_list args;
	gchar *msg;
	GtkWidget *dialog;

	_VPRINTF_MESSAGE (format, args, msg);
	g_return_val_if_fail (msg != NULL, default_answer);
	dialog = gtk_message_dialog_new (parent,
		GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_MESSAGE_QUESTION,
		GTK_BUTTONS_YES_NO,
		"%s", msg);
        g_free (msg);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog),
		default_answer ? GTK_RESPONSE_YES : GTK_RESPONSE_NO);
	return GTK_RESPONSE_YES ==
		go_gtk_dialog_run (GTK_DIALOG (dialog), parent);
}

#ifndef HAVE_GTK_WIDGET_SET_TOOLTIP_TEXT
static GtkTooltips *
get_tooltips (GObject *obj)
{
	GtkTooltips *tips = g_object_get_data (obj, "-go-tips");

	if (!tips) {
		tips = gtk_tooltips_new ();

#if GLIB_CHECK_VERSION(2,10,0) && GTK_CHECK_VERSION(2,8,14)
		g_object_ref_sink (tips);
#else
		g_object_ref (tips);
		gtk_object_sink (GTK_OBJECT (tips));
#endif
		g_object_set_data_full (obj, "-go-tips", tips,
					(GDestroyNotify)g_object_unref);
	}

	return tips;
}
#endif

void
go_widget_set_tooltip_text (GtkWidget *widget, char const *tip)
{
#ifdef HAVE_GTK_WIDGET_SET_TOOLTIP_TEXT
	gtk_widget_set_tooltip_text (widget, tip);
#else
	GtkTooltips *tips = get_tooltips (G_OBJECT (widget));
	gtk_tooltips_set_tip (tips, widget, tip, NULL);
#endif
}

void
go_tool_item_set_tooltip_text (GtkToolItem *item, char const *tip)
{
#ifdef HAVE_GTK_TOOL_ITEM_SET_TOOLTIP_TEXT
	gtk_tool_item_set_tooltip_text (item, tip);
#else
	GtkTooltips *tips = get_tooltips (G_OBJECT (item));
	gtk_tool_item_set_tooltip (item, tips, tip, NULL);
#endif
}


#ifndef HAVE_GTK_DIALOG_GET_RESPONSE_FOR_WIDGET
/* This is public from 2.8 onwards.   */
static gint
gtk_dialog_get_response_for_widget (GtkDialog *dialog, GtkWidget *widget)
{
	struct {
		gint response_id;
	} const *rd = g_object_get_data (G_OBJECT (widget),
				   "gtk-dialog-response-data");
	if (!rd)
		return GTK_RESPONSE_NONE;
	else
		return rd->response_id;
}
#endif


/**
 * go_dialog_guess_alternative_button_order:
 * @dialog : #GtkDialog
 *
 * This function inspects the buttons in the dialog and comes up
 * with a reasonable alternative dialog order.
 **/
void
go_dialog_guess_alternative_button_order (GtkDialog *dialog)
{
	GList *children, *tmp;
	int i, nchildren;
	int *new_order;
	int i_yes = -1, i_no = -1, i_ok = -1, i_cancel = -1, i_apply = -1;
	gboolean again;
	gboolean any = FALSE;
	int loops = 0;

	children = gtk_container_get_children (GTK_CONTAINER (gtk_dialog_get_action_area (dialog)));
	if (!children)
		return;

	nchildren = g_list_length (children);
	new_order = g_new (int, nchildren);

	for (tmp = children, i = 0; tmp; tmp = tmp->next, i++) {
		GtkWidget *child = tmp->data;
		int res = gtk_dialog_get_response_for_widget (dialog, child);
		new_order[i] = res;
		switch (res) {
		case GTK_RESPONSE_YES: i_yes = i; break;
		case GTK_RESPONSE_NO: i_no = i; break;
		case GTK_RESPONSE_OK: i_ok = i; break;
		case GTK_RESPONSE_CANCEL: i_cancel = i; break;
		case GTK_RESPONSE_APPLY: i_apply = i; break;
		}
	}
	g_list_free (children);

#define MAYBE_SWAP(ifirst,ilast)				\
	if (ifirst >= 0 && ilast >= 0 && ifirst > ilast) {	\
		int tmp;					\
								\
		tmp = new_order[ifirst];			\
		new_order[ifirst] = new_order[ilast];		\
		new_order[ilast] = tmp;				\
								\
		tmp = ifirst;					\
		ifirst = ilast;					\
		ilast = tmp;					\
								\
		again = TRUE;					\
		any = TRUE;					\
	}

	do {
		again = FALSE;
		MAYBE_SWAP (i_yes, i_no);
		MAYBE_SWAP (i_ok, i_cancel);
		MAYBE_SWAP (i_cancel, i_apply);
		MAYBE_SWAP (i_no, i_cancel);
	} while (again && ++loops < 2);

#undef MAYBE_SWAP

	if (any)
		gtk_dialog_set_alternative_button_order_from_array (dialog,
								    nchildren,
								    new_order);
	g_free (new_order);
}

/**
 * go_menu_position_below:
 * @menu: a #GtkMenu
 * @x: non-NULL storage for the X coordinate of the menu
 * @y: non-NULL storage for the Y coordinate of the menu
 * @push_in: non-NULL storage for the push-in distance
 * @user_data: arbitrary
 *
 * Implementation of a GtkMenuPositionFunc that positions
 * the child window under the parent one, for use with gtk_menu_popup.
 **/
void
go_menu_position_below (GtkMenu  *menu,
			gint     *x,
			gint     *y,
			gint     *push_in,
			gpointer  user_data)
{
	GtkWidget *widget = GTK_WIDGET (user_data);
	gint sx, sy;
	GtkRequisition req;
	GdkScreen *screen;
	gint monitor_num;
	GdkRectangle monitor;
	GdkWindow *window = gtk_widget_get_window (widget);
	GtkAllocation size;

	gtk_widget_get_allocation (widget, &size);

	if (window)
		gdk_window_get_origin (window, &sx, &sy);
	else
		sx = sy = 0;

	if (!gtk_widget_get_has_window (widget))
	{
		sx += size.x;
		sy += size.y;
	}

	gtk_widget_size_request (GTK_WIDGET (menu), &req);

	if (gtk_widget_get_direction (widget) == GTK_TEXT_DIR_LTR)
		*x = sx;
	else
		*x = sx + size.width - req.width;
	*y = sy;

	screen = gtk_widget_get_screen (widget);
	monitor_num = gdk_screen_get_monitor_at_window (screen, window);
	gdk_screen_get_monitor_geometry (screen, monitor_num, &monitor);

	if (*x < monitor.x)
		*x = monitor.x;
	else if (*x + req.width > monitor.x + monitor.width)
		*x = monitor.x + monitor.width - req.width;

	if (monitor.y + monitor.height - *y - size.height >= req.height)
		*y +=size.height;
	else if (*y - monitor.y >= req.height)
		*y -= req.height;
	else if (monitor.y + monitor.height - *y - size.height > *y - monitor.y)
		*y += size.height;
	else
		*y -= req.height;

	*push_in = FALSE;
}

/**
 * go_gtk_url_show:
 * @url: the url to show
 * @screen: screen to show the uri on or %NULL for the default screen
 *
 * This function is a simple convenience wrapper for #gtk_show_uri.
 *
 * Returns: %NULL on sucess, or a newly allocated #GError if something
 * went wrong.
 **/
GError *
go_gtk_url_show (gchar const *url, GdkScreen *screen)
{
#if defined(HAVE_GTK_SHOW_URI)
	GError *error = NULL;
	gtk_show_uri (screen, url, GDK_CURRENT_TIME, &error);
	return error;
#else
	return go_url_show (url);
#endif
}
