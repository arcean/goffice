/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * smoothing/gog-exp-smooth.c :
 *
 * Copyright (C) 2006 Jean Brefort (jean.brefort@normalesup.org)
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
#include "gog-exp-smooth.h"
#include <goffice/app/go-plugin.h>
#include <goffice/data/go-data.h>
#include <goffice/graph/gog-data-allocator.h>
#include <goffice/math/go-math.h>
#include <goffice/math/go-rangefunc.h>
#include <goffice/utils/go-persist.h>
#include <gsf/gsf-impl-utils.h>
#include <glib/gi18n-lib.h>

enum {
	EXP_SMOOTH_PROP_0,
	EXP_SMOOTH_PROP_STEPS,
};

static GObjectClass *exp_smooth_parent_klass;

#ifdef GOFFICE_WITH_GTK
#include <goffice/gtk/goffice-gtk.h>
#include <gtk/gtk.h>

static void
steps_changed_cb (GtkSpinButton *button, GObject *es)
{
	g_object_set (es, "steps", gtk_spin_button_get_value_as_int (button), NULL);
}

static void
gog_exp_smooth_populate_editor (GogObject *obj,
				GOEditor *editor,
				GogDataAllocator *dalloc,
				GOCmdContext *cc)
{
	GogExpSmooth *es = GOG_EXP_SMOOTH (obj);
	GogDataset *set = GOG_DATASET (obj);
	char const *dir = go_plugin_get_dir_name (
		go_plugins_get_plugin_by_id ("GOffice_smoothing"));
	char	 *path = g_build_filename (dir, "gog-exp-smooth.ui", NULL);
	GtkBuilder *gui = go_gtk_builder_new (path, GETTEXT_PACKAGE, cc);
	GtkWidget *label, *box, *w = go_gtk_builder_get_widget (gui, "steps");
	GtkTable *table;

	go_widget_set_tooltip_text (w, _("Number of interpolation steps"));
	gtk_spin_button_set_range (GTK_SPIN_BUTTON (w), 10, G_MAXINT);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), es->steps);
	g_signal_connect (G_OBJECT (w), "value-changed", G_CALLBACK (steps_changed_cb), obj);
	table = GTK_TABLE (gtk_builder_get_object (gui, "exp-smooth-prefs"));
	w = GTK_WIDGET (gog_data_allocator_editor (dalloc, set, 0, GOG_DATA_SCALAR));
	box = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (box), w);
	go_widget_set_tooltip_text (box, _("Default period is 10 * (xmax - xmin)/(nvalues - 1)\n"
					"If no value or a negative (or nul) value is provided, the "
					"default will be used"));
	gtk_widget_show_all (box);
	gtk_table_attach (table, box, 1, 2, 0, 1, GTK_FILL | GTK_EXPAND, 0, 0, 0);
	label = go_gtk_builder_get_widget (gui, "period-lbl");
	g_object_set (G_OBJECT (label), "mnemonic-widget", w, NULL);
	go_editor_add_page (editor, table, _("Properties"));
	g_object_unref (gui);

	(GOG_OBJECT_CLASS (exp_smooth_parent_klass)->populate_editor) (obj, editor, dalloc, cc);
}
#endif

static void
gog_exp_smooth_update (GogObject *obj)
{
	GogExpSmooth *es = GOG_EXP_SMOOTH (obj);
	GogSeries *series = GOG_SERIES (obj->parent);
	double const *y_vals, *x_vals;
	unsigned nb, i, n;
	double period = -1., xmin, xmax, delta, t, u, r;
	double *x, *y, *w, *incr;
	double epsilon;

	g_free (es->base.x);
	es->base.x = NULL;
	g_free (es->base.y);
	es->base.y = NULL;
	if (!gog_series_is_valid (series))
		return;

	nb = gog_series_get_xy_data (series, &x_vals, &y_vals);
	if (nb == 0)
		return;
	x = g_new (double, nb);
	y = g_new (double, nb);
	/* Remove invalid data */
	for (i = 0, n = 0; i < nb; i++) {
		if (!go_finite (x_vals[i]) || !go_finite (y_vals[i]))
			continue;
		x[n] = x_vals[i];
		y[n++] = y_vals[i];
	}
	go_range_min (x, n, &xmin);
	go_range_max (x, n, &xmax);
	if (n < 2) {
		g_free (x);
		g_free (y);
		return;
	}
	go_range_min (x, n, &xmin);
	go_range_max (x, n, &xmax);
	if (es->period->data != NULL)
		period = go_data_get_scalar_value (es->period->data);
	if (period <= 0.)
		period = 10. * (xmax - xmin) / (n - 1);

	delta = (xmax - xmin) / es->steps;
	es->base.nb = es->steps + 1;
	es->base.x = g_new (double, es->base.nb);
	es->base.y = g_new (double, es->base.nb);
	incr = g_new0 (double, es->base.nb);
	w = g_new0 (double, es->base.nb);
	epsilon = DBL_EPSILON * es->steps;
	for (i = 0; i < n; i++) {
		nb = (unsigned) ceil ((x[i] - xmin) / delta - epsilon);
		t = pow (2., (x[i] - xmin - nb * delta) / period);
		incr[nb] += t * y[i];
		w[nb] += t;
	}
	r = pow (2., -delta / period);
	t = u = 0.;
	for (i = 0; i < es->base.nb; i++) {
		t = t * r + incr[i];
		u = u * r + w[i];
		es->base.x[i] = xmin + i * delta;
		es->base.y[i] = t / u;
	}

	g_free (x);
	g_free (y);
	g_free (w);
	g_free (incr);
	gog_object_emit_changed (GOG_OBJECT (obj), FALSE);
}

static char const *
gog_exp_smooth_type_name (G_GNUC_UNUSED GogObject const *item)
{
	/* xgettext : the base for how to name exponentially smoothed curves objects
	 * eg The 2nd one for a series will be called
	 * 	Exponentially smoothed curve2 */
	return N_("Exponentially smoothed curve");
}

static void
gog_exp_smooth_get_property (GObject *obj, guint param_id,
		       GValue *value, GParamSpec *pspec)
{
	GogExpSmooth *es = GOG_EXP_SMOOTH (obj);
	switch (param_id) {
	case EXP_SMOOTH_PROP_STEPS:
		g_value_set_int (value, es->steps);
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 break;
	}
}

static void
gog_exp_smooth_set_property (GObject *obj, guint param_id,
		       GValue const *value, GParamSpec *pspec)
{
	GogExpSmooth *es = GOG_EXP_SMOOTH (obj);
	switch (param_id) {
	case EXP_SMOOTH_PROP_STEPS:
		es->steps = g_value_get_int (value);
		gog_object_request_update (GOG_OBJECT (obj));
		break;

	default: G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, param_id, pspec);
		 return; /* NOTE : RETURN */
	}
}

static void
gog_exp_smooth_finalize (GObject *obj)
{
	GogExpSmooth *es = GOG_EXP_SMOOTH (obj);
	if (es->period != NULL) {
		gog_dataset_finalize (GOG_DATASET (obj));
		g_free (es->period);
		es->period = NULL;
	}
	(*exp_smooth_parent_klass->finalize) (obj);
}

static void
gog_exp_smooth_class_init (GogSmoothedCurveClass *curve_klass)
{
	GObjectClass *gobject_klass = (GObjectClass *) curve_klass;
	GogObjectClass *gog_object_klass = (GogObjectClass *) curve_klass;
	exp_smooth_parent_klass = g_type_class_peek_parent (curve_klass);

	gobject_klass->finalize = gog_exp_smooth_finalize;
	gobject_klass->get_property = gog_exp_smooth_get_property;
	gobject_klass->set_property = gog_exp_smooth_set_property;

#ifdef GOFFICE_WITH_GTK
	gog_object_klass->populate_editor = gog_exp_smooth_populate_editor;
#endif
	gog_object_klass->update = gog_exp_smooth_update;
	gog_object_klass->type_name	= gog_exp_smooth_type_name;

	g_object_class_install_property (gobject_klass, EXP_SMOOTH_PROP_STEPS,
		g_param_spec_int ("steps",
			_("Steps"),
			_("Number of interpolation steps"),
			10, G_MAXINT, 100,
			GSF_PARAM_STATIC | G_PARAM_READWRITE | GO_PARAM_PERSISTENT));
}

static void
gog_exp_smooth_init (GogExpSmooth *es)
{
	es->steps = 100;
	es->period = g_new0 (GogDatasetElement, 1);
}

static void
gog_exp_smooth_dataset_dims (GogDataset const *set, int *first, int *last)
{
	*first = 0;
	*last = 0;
}

static GogDatasetElement *
gog_exp_smooth_dataset_get_elem (GogDataset const *set, int dim_i)
{
	GogExpSmooth const *es = GOG_EXP_SMOOTH (set);
	g_return_val_if_fail (dim_i == 0, NULL);
	return es->period;
}

static void
gog_exp_smooth_dataset_dim_changed (GogDataset *set, int dim_i)
{
	gog_object_request_update (GOG_OBJECT (set));
}

static void
gog_exp_smooth_dataset_init (GogDatasetClass *iface)
{
	iface->get_elem	   = gog_exp_smooth_dataset_get_elem;
	iface->dims	   = gog_exp_smooth_dataset_dims;
	iface->dim_changed = gog_exp_smooth_dataset_dim_changed;
}

GSF_DYNAMIC_CLASS_FULL (GogExpSmooth, gog_exp_smooth,
	 NULL, NULL, gog_exp_smooth_class_init, NULL,
	gog_exp_smooth_init, GOG_TYPE_SMOOTHED_CURVE, 0,
	GSF_INTERFACE (gog_exp_smooth_dataset_init, GOG_TYPE_DATASET))
