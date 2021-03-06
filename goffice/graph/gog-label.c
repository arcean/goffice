/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-label.c
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
 * Copyright (C) 2005      Jean Brefort (jean.brefort@normalesup.org)
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

#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

/**
 * SECTION: gog-label
 * @short_description: A text label.
 *
 * A text label for use in a graph.
 * Can for instance be used as a title of a #GogChart or #GogGraph.
 */

static GType gog_text_view_get_type (void);

enum {
	TEXT_PROP_0,
	TEXT_PROP_ALLOW_MARKUP
};

static GObjectClass *text_parent_klass;

static void
gog_text_set_property (GObject *obj, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GogText *text = GOG_TEXT (obj);

	switch (param_id) {
	case TEXT_PROP_ALLOW_MARKUP :
		text->allow_markup = g_value_get_boolean (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static void
gog_text_get_property (GObject *obj, guint param_id,
			     GValue *value, GParamSpec *pspec)
{
	GogText *text = GOG_TEXT (obj);

	switch (param_id) {
	case TEXT_PROP_ALLOW_MARKUP :
		g_value_set_boolean (value, text->allow_markup);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_text_init_style (GogStyledObject *gso, GOStyle *style)
{
	GogObject *parent;

	style->interesting_fields = GO_STYLE_OUTLINE | GO_STYLE_FILL |
		GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT;
	gog_theme_fillin_style (gog_object_get_theme (GOG_OBJECT (gso)),
		style, GOG_OBJECT (gso), 0, GO_STYLE_OUTLINE | GO_STYLE_FILL |
	        GO_STYLE_FONT | GO_STYLE_TEXT_LAYOUT);

	/* Kludge for default Y axis title orientation. This should have be done
	 * in GogTheme, but it's not possible without breaking graph persistence
	 * compatibility */
	parent = gog_object_get_parent (GOG_OBJECT (gso));
	if (GOG_IS_AXIS (parent)) {
		GogChart *chart = GOG_CHART (gog_object_get_parent (parent));
		if (!gog_chart_is_3d (chart)
		    && gog_axis_get_atype (GOG_AXIS (parent)) == GOG_AXIS_Y
		    && style->text_layout.auto_angle)
			style->text_layout.angle = 90.0;
	}
}

static void
gog_text_class_init (GogTextClass *klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) klass;
	GogObjectClass *gog_klass = (GogObjectClass *) klass;
	GogStyledObjectClass *style_klass = (GogStyledObjectClass *) klass;

	text_parent_klass = g_type_class_peek_parent (klass);
	gobject_klass->set_property = gog_text_set_property;
	gobject_klass->get_property = gog_text_get_property;

	g_object_class_install_property (gobject_klass, TEXT_PROP_ALLOW_MARKUP,
		g_param_spec_boolean ("allow-markup",
			_("Allow markup"),
			_("Support basic html-ish markup"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));

	gog_klass->view_type		= gog_text_view_get_type ();
	style_klass->init_style 	= gog_text_init_style;
}

static void
gog_text_init (GogText *text)
{
	text->allow_markup = FALSE;
}

char *
gog_text_get_str (GogText *text)
{
	GogTextClass *klass;

	g_return_val_if_fail (GOG_IS_TEXT (text), NULL);

       	klass = GOG_TEXT_GET_CLASS (text);

	if (klass->get_str != NULL)
		return (*klass->get_str) (text);

	return NULL;
}

GSF_CLASS_ABSTRACT (GogText, gog_text,
		    gog_text_class_init, gog_text_init,
		    GOG_TYPE_OUTLINED_OBJECT);

/************************************************************************/

struct _GogLabel {
	GogText base;

	GogDatasetElement text;
};

typedef GogTextClass GogLabelClass;

static GObjectClass *label_parent_klass;

#ifdef GOFFICE_WITH_GTK
static void
gog_label_populate_editor (GogObject *gobj,
			   GOEditor *editor,
			   GogDataAllocator *dalloc,
			   GOCmdContext *cc)
{
	static guint label_pref_page = 0;
	GtkWidget *hbox = gtk_hbox_new (FALSE, 12);
	GtkWidget *alignment = gtk_alignment_new (0, 0, 1, 0);
	GtkWidget *editor_widget =
		GTK_WIDGET
		(gog_data_allocator_editor (dalloc, GOG_DATASET (gobj),
					    0, GOG_DATA_SCALAR));

	gtk_container_set_border_width (GTK_CONTAINER (alignment), 12);
	gtk_box_pack_start (GTK_BOX (hbox),
		gtk_label_new_with_mnemonic (_("_Text:")), FALSE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (hbox), editor_widget, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (alignment), hbox);
	gtk_widget_show_all (alignment);

	go_editor_add_page (editor, alignment, _("Data"));

	(GOG_OBJECT_CLASS(label_parent_klass)->populate_editor) (gobj, editor, dalloc, cc);
	go_editor_set_store_page (editor, &label_pref_page);
}
#endif

static char *
gog_label_get_str (GogText *text)
{
	GogLabel *label = GOG_LABEL (text);

	g_return_val_if_fail (GOG_IS_LABEL (label), NULL);

	if (label->text.data != NULL)
		return go_data_get_scalar_string (label->text.data);

	return NULL;
}

static void
gog_label_finalize (GObject *obj)
{
	gog_dataset_finalize (GOG_DATASET (obj));
	(*label_parent_klass->finalize) (obj);
}

static void
gog_label_class_init (GogLabelClass *klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) klass;
	GogTextClass *got_klass = (GogTextClass *) klass;
#ifdef GOFFICE_WITH_GTK
	GogObjectClass *gog_klass = (GogObjectClass *) klass;
	gog_klass->populate_editor = gog_label_populate_editor;
#endif

	label_parent_klass = g_type_class_peek_parent (klass);
	gobject_klass->finalize	   = gog_label_finalize;
	got_klass->get_str	   = gog_label_get_str;
}

static void
gog_label_dims (GogDataset const *set, int *first, int *last)
{
	*first = *last = 0;
}

static GogDatasetElement *
gog_label_get_elem (GogDataset const *set, int dim_i)
{
	GogLabel *label = GOG_LABEL (set);
	return &label->text;
}

static void
gog_label_dim_changed (GogDataset *set, int dim_i)
{
	gog_object_emit_changed (GOG_OBJECT (set), TRUE);
}

static void
gog_label_dataset_init (GogDatasetClass *iface)
{
	iface->dims	   = gog_label_dims;
	iface->get_elem	   = gog_label_get_elem;
	iface->dim_changed = gog_label_dim_changed;
}

GSF_CLASS_FULL (GogLabel, gog_label,
		NULL, NULL, gog_label_class_init, NULL,
		NULL, GOG_TYPE_TEXT, 0,
		GSF_INTERFACE (gog_label_dataset_init, GOG_TYPE_DATASET))

/************************************************************************/

enum {
	REG_EQN_PROP_0,
	REG_EQN_SHOW_EQ,
	REG_EQN_SHOW_R2
};

struct  _GogRegEqn {
	GogLabel base;
	gboolean show_eq, show_r2;
};

typedef GogTextClass GogRegEqnClass;

static GObjectClass *reg_eqn_parent_klass;

static void
gog_reg_eqn_set_property (GObject *obj, guint param_id,
			GValue const *value, GParamSpec *pspec)
{
	GogRegEqn *reg_eqn = GOG_REG_EQN (obj);

	switch (param_id) {
	case REG_EQN_SHOW_EQ:
		reg_eqn->show_eq = g_value_get_boolean (value);
		break;
	case REG_EQN_SHOW_R2:
		reg_eqn->show_r2 = g_value_get_boolean (value);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return;
	}
}

static void
gog_reg_eqn_get_property (GObject *obj, guint param_id,
			GValue *value, GParamSpec *pspec)
{
	GogRegEqn *reg_eqn = GOG_REG_EQN (obj);

	switch (param_id) {
	case REG_EQN_SHOW_EQ:
		g_value_set_boolean (value, reg_eqn->show_eq);
		break;
	case REG_EQN_SHOW_R2:
		g_value_set_boolean (value, reg_eqn->show_r2);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return;
	}
}

#ifdef GOFFICE_WITH_GTK
static void
cb_text_visibility_changed (GtkToggleButton *button, GogObject *gobj)
{
	char const* property = (char const*) g_object_get_data (G_OBJECT (button), "prop");
	g_object_set (G_OBJECT (gobj), property, gtk_toggle_button_get_active (button), NULL);
	gog_object_emit_changed (gobj, TRUE);
}

static void
gog_reg_eqn_populate_editor (GogObject *gobj,
			     GOEditor *editor,
			     GogDataAllocator *dalloc,
			     GOCmdContext *cc)
{
	GtkWidget *w;
	GtkBuilder *gui;
	GogRegEqn *reg_eqn = GOG_REG_EQN (gobj);

	gui = go_gtk_builder_new ("gog-reg-eqn-prefs.ui", GETTEXT_PACKAGE, cc);
	if (gui == NULL)
		return;

	go_editor_add_page (editor,
			     go_gtk_builder_get_widget (gui, "reg-eqn-prefs"),
			     _("Details"));

	w = go_gtk_builder_get_widget (gui, "show_eq");
	g_object_set_data (G_OBJECT (w), "prop", (void*) "show-eq");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), reg_eqn->show_eq);
	g_signal_connect (w, "toggled", G_CALLBACK (cb_text_visibility_changed), gobj);

	w = go_gtk_builder_get_widget (gui, "show_r2");
	g_object_set_data (G_OBJECT (w), "prop", (void*) "show-r2");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), reg_eqn->show_r2);
	g_signal_connect (w, "toggled", G_CALLBACK (cb_text_visibility_changed), gobj);

	g_object_unref (gui);

	(GOG_OBJECT_CLASS(reg_eqn_parent_klass)->populate_editor) (gobj, editor, dalloc, cc);
}
#endif

static char const *
gog_reg_eqn_type_name (GogObject const *gobj)
{
	return N_("Regression Equation");
}

static char *
gog_reg_eqn_get_str (GogText *text)
{
	GogRegCurve *reg_curve = GOG_REG_CURVE ((GOG_OBJECT (text))->parent);
	GogRegEqn *reg_eqn = GOG_REG_EQN (text);

	if (!reg_eqn->show_r2)
		return reg_eqn->show_eq ?
			g_strdup (gog_reg_curve_get_equation (reg_curve)) :
			NULL;
	else
		return reg_eqn->show_eq ?
			g_strdup_printf ("%s\r\nR² = %g",
					 gog_reg_curve_get_equation (reg_curve),
					 gog_reg_curve_get_R2 (reg_curve)) :
			g_strdup_printf ("R² = %g", gog_reg_curve_get_R2 (reg_curve));
}

static void
gog_reg_eqn_class_init (GogObjectClass *gog_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *)gog_klass;
	GogTextClass *got_klass = (GogTextClass *) gog_klass;

	reg_eqn_parent_klass = g_type_class_peek_parent (gog_klass);

	gobject_klass->set_property	= gog_reg_eqn_set_property;
	gobject_klass->get_property	= gog_reg_eqn_get_property;
#ifdef GOFFICE_WITH_GTK
	gog_klass->populate_editor	= gog_reg_eqn_populate_editor;
#endif
	gog_klass->type_name		= gog_reg_eqn_type_name;
	got_klass->get_str	   	= gog_reg_eqn_get_str;

	g_object_class_install_property (gobject_klass, REG_EQN_SHOW_EQ,
		g_param_spec_boolean ("show-eq",
			_("Show equation"),
			_("Show the equation on the graph"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
	g_object_class_install_property (gobject_klass, REG_EQN_SHOW_R2,
		g_param_spec_boolean ("show-r2",
			_("Show coefficient"),
			_("Show the correlation coefficient on the graph"),
			TRUE,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
}

static void
gog_reg_eqn_init (GogRegEqn *reg_eqn)
{
	reg_eqn->show_eq = TRUE;
	reg_eqn->show_r2 = TRUE;
}

GSF_CLASS (GogRegEqn, gog_reg_eqn,
	   gog_reg_eqn_class_init, gog_reg_eqn_init,
	   GOG_TYPE_TEXT)

/************************************************************************/

typedef GogOutlinedView		GogTextView;
typedef GogOutlinedViewClass	GogTextViewClass;

#define GOG_TYPE_LABEL_VIEW	(gog_label_view_get_type ())
#define GOG_LABEL_VIEW(o)	(G_TYPE_CHECK_INSTANCE_CAST ((o), GOG_TYPE_LABEL_VIEW, GogLabelView))
#define GOG_IS_LABEL_VIEW(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GOG_TYPE_LABEL_VIEW))

static GogViewClass *text_view_parent_klass;

static void
gog_text_view_size_request (GogView *v,
			    GogViewRequisition const *available,
			    GogViewRequisition *req)
{
	GogText *text = GOG_TEXT (v->model);
	char *str = gog_text_get_str (text);
	GOGeometryAABR aabr;

	req->w = req->h = 0.;
	if (str != NULL) {
		gog_renderer_push_style (v->renderer, text->base.base.style);
		gog_renderer_get_text_AABR (v->renderer, str, FALSE, &aabr);
		gog_renderer_pop_style (v->renderer);
		req->w = aabr.w;
		req->h = aabr.h;
		g_free (str);
	}
	text_view_parent_klass->size_request (v, available, req);
}

static void
gog_text_view_render (GogView *view, GogViewAllocation const *bbox)
{
	GogText *text = GOG_TEXT (view->model);
	GogOutlinedObject *goo = GOG_OUTLINED_OBJECT (text);
	GOStyle *style = text->base.base.style;
	char *str = gog_text_get_str (text);

	gog_renderer_push_style (view->renderer, style);
	if (str != NULL) {
		double outline = gog_renderer_line_size (view->renderer,
							 goo->base.style->line.width);
		if (style->fill.type != GO_STYLE_FILL_NONE || outline > 0.) {
			GogViewAllocation rect;
			GOGeometryAABR aabr;
			double pad_x = gog_renderer_pt2r_x (view->renderer, goo->padding_pts);
			double pad_y = gog_renderer_pt2r_y (view->renderer, goo->padding_pts);

			gog_renderer_get_text_AABR (view->renderer, str, FALSE, &aabr);
			rect = view->allocation;
			rect.w = aabr.w + 2. * outline + pad_x;
			rect.h = aabr.h + 2. * outline + pad_y;
			gog_renderer_draw_rectangle (view->renderer, &rect);
		}
		gog_renderer_draw_text (view->renderer, str,
					&view->residual, GTK_ANCHOR_NW, FALSE);
		g_free (str);
	}
	gog_renderer_pop_style (view->renderer);
}

static void
gog_text_view_class_init (GogTextViewClass *gview_klass)
{
	GogViewClass *view_klass    = (GogViewClass *) gview_klass;

	text_view_parent_klass = g_type_class_peek_parent (gview_klass);
	view_klass->size_request    = gog_text_view_size_request;
	view_klass->render	    = gog_text_view_render;
}

GSF_CLASS (GogTextView, gog_text_view,
	   gog_text_view_class_init, NULL,
	   GOG_TYPE_OUTLINED_VIEW)
