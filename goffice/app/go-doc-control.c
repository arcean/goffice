/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-doc-control.c : A controller for GOffice Document
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
#include <goffice/app/goffice-app.h>
#include <goffice/app/go-doc-control-impl.h>

#include <gsf/gsf-impl-utils.h>

static void
go_doc_control_class_init (GObjectClass *klass)
{
}

static void
go_doc_control_init (GODocControl *obj)
{
}

GODoc *
go_doc_control_get_doc (GODocControl *dc)
{
	return dc->doc;
}

void
go_doc_control_set_doc (GODocControl *dc, GODoc *doc)
{
	dc->doc = doc;
}

GSF_CLASS (GODocControl, go_doc_control,
	   go_doc_control_class_init, go_doc_control_init,
	   G_TYPE_OBJECT)

