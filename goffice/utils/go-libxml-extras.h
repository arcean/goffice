/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * go-libxml-extras.h :
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
#ifndef GO_LIBXML_EXTRAS_H
#define GO_LIBXML_EXTRAS_H

#include <goffice/goffice.h>
#include <libxml/tree.h>
#include <gsf/gsf-libxml.h>

G_BEGIN_DECLS

xmlDocPtr  go_xml_parse_file    (const char *filename);

xmlChar   *go_xml_node_get_cstr	(xmlNodePtr node, char const *name);
void	   go_xml_node_set_cstr	(xmlNodePtr node, char const *name, char const *val);
gboolean   go_xml_node_get_bool	(xmlNodePtr node, char const *name, gboolean *result);
void       go_xml_node_set_bool	(xmlNodePtr node, char const *name, gboolean val);
gboolean   go_xml_node_get_int	(xmlNodePtr node, char const *name, int *result);
void       go_xml_node_set_int	(xmlNodePtr node, char const *name, int  val);
gboolean   go_xml_node_get_double	(xmlNodePtr node, char const *name, double *result);
void       go_xml_node_set_double	(xmlNodePtr node, char const *name, double  val, int precision);
gboolean   go_xml_node_get_gocolor (xmlNodePtr node, char const *name, GOColor *result);
void	   go_xml_node_set_gocolor (xmlNodePtr node, char const *name, GOColor  val);
gboolean   go_xml_node_get_enum    (xmlNodePtr node, char const *name, GType etype, gint *val);
void       go_xml_node_set_enum    (xmlNodePtr node, char const *name, GType etype, gint val);

xmlNode *go_xml_get_child_by_name	 (xmlNode const *tree, char const *name);
xmlNode *go_xml_get_child_by_name_no_lang (xmlNode const *tree, char const *name);
xmlNode *go_xml_get_child_by_name_by_lang (xmlNode const *tree, char const *name);

void	   go_xml_out_add_color (GsfXMLOut *out, char const *id, GOColor c);

G_END_DECLS

#endif /* GO_LIBXML_EXTRAS_H */