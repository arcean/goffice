/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gog-bubble-prefs.c
 *
 * Copyright (C) 2003-2004 Jody Goldberg (jody@gnome.org)
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
#include "gog-xy.h"
#include <goffice/gtk/goffice-gtk.h>
#include <goffice/app/go-plugin.h>
#include <goffice/gtk/go-gtk-compat.h>

#include <string.h>

GtkWidget *gog_bubble_plot_pref   (GogBubblePlot *bubble, GOCmdContext *cc);

static void
cb_type_changed (GtkToggleButton* button, GObject *bubble)
{
	if (gtk_toggle_button_get_active (button))
		g_object_set (bubble, "size-as-area",
			strcmp (gtk_buildable_get_name (GTK_BUILDABLE (button)), "area")? FALSE: TRUE, NULL);
}

static void
cb_style_changed (GtkToggleButton* button, GObject *bubble)
{
	g_object_set (bubble, "vary-style-by-element",
		gtk_toggle_button_get_active (button), NULL);
}

static void
cb_3d_changed (GtkToggleButton* button, GObject *bubble)
{
	g_object_set (bubble, "in-3d",
		gtk_toggle_button_get_active (button), NULL);
}

static void
cb_negatives_changed (GtkToggleButton* button, GObject *bubble)
{
	g_object_set (bubble, "show-negatives",
		gtk_toggle_button_get_active (button), NULL);
}

static void
cb_scale_changed (GtkAdjustment *adj, GObject *bubble)
{
	g_object_set (bubble, "bubble-scale", gtk_adjustment_get_value (adj) / 100., NULL);
}

GtkWidget *
gog_bubble_plot_pref (GogBubblePlot *bubble, GOCmdContext *cc)
{
	GtkWidget  *w;
	char const *dir = go_plugin_get_dir_name (
		go_plugins_get_plugin_by_id ("GOffice_plot_xy"));
	char	 *path = g_build_filename (dir, "gog-bubble-prefs.ui", NULL);
	GtkBuilder *gui = go_gtk_builder_new (path, GETTEXT_PACKAGE, cc);

	g_free (path);
        if (gui == NULL)
                return NULL;

	w = go_gtk_builder_get_widget (gui, "area");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), bubble->size_as_area);
	g_signal_connect (G_OBJECT (w),
		"toggled",
		G_CALLBACK (cb_type_changed), bubble);

	w = go_gtk_builder_get_widget (gui, "diameter");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), !bubble->size_as_area);
	g_signal_connect (G_OBJECT (w),
		"toggled",
		G_CALLBACK (cb_type_changed), bubble);

	w = go_gtk_builder_get_widget (gui, "vary_style_by_element");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), bubble->base.base.vary_style_by_element);
	g_signal_connect (G_OBJECT (w),
		"toggled",
		G_CALLBACK (cb_style_changed), bubble);

	w = go_gtk_builder_get_widget (gui, "3d");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), bubble->in_3d);
	g_signal_connect (G_OBJECT (w),
		"toggled",
		G_CALLBACK (cb_3d_changed), bubble);

	/* TODO Add support for 3D bubbles. Hide the button for now. */
	gtk_widget_hide (w);

	w = go_gtk_builder_get_widget (gui, "scale");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (w), bubble->bubble_scale * 100.);
	g_signal_connect (G_OBJECT (gtk_spin_button_get_adjustment (GTK_SPIN_BUTTON (w))),
		"value_changed",
		G_CALLBACK (cb_scale_changed), bubble);

	w = go_gtk_builder_get_widget (gui, "show_negative_values");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), bubble->show_negatives);
	g_signal_connect (G_OBJECT (w),
		"toggled",
		G_CALLBACK (cb_negatives_changed), bubble);

	w = GTK_WIDGET (g_object_ref (gtk_builder_get_object (gui, "gog_bubble_prefs")));
	g_object_unref (gui);

	return w;
}
