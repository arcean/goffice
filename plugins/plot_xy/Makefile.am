goffice_graph_xydir = $(goffice_plugindir)/plot_xy
xmldir	 = $(goffice_graph_xydir)
uidir = $(goffice_graph_xydir)

goffice_graph_xy_LTLIBRARIES = xy.la
xy_la_LDFLAGS = -module $(GOFFICE_PLUGIN_FLAGS)
xy_la_LIBADD = $(GOFFICE_PLUGIN_LIBADD)

xy_la_SOURCES = \
	gog-xy.c	\
	gog-xy.h \
	gog-xy-dropbar.c	\
	gog-xy-dropbar.h

xml_in_files = plugin.xml.in plot-types.xml.in
xml_DATA = $(xml_in_files:.xml.in=.xml)

@INTLTOOL_XML_RULE@

if WITH_GTK
xy_la_SOURCES  += gog-bubble-prefs.c
dist_ui_DATA = \
	gog-bubble-prefs.ui	\
	gog-xy-prefs.ui	\
	gog-xy-color-prefs.ui	\
	gog-xy-series-prefs.ui	\
	gog-xy-dropbar-prefs.ui
endif

# do not use the intl-tool stuff to merge the text back
# its simpler to just use gettext directly
plot-types.xml : plot-types.xml.in
	cp $< $@

EXTRA_DIST	= $(xml_in_files)
DISTCLEANFILES	= $(xml_in_files:.xml.in=.xml)

include $(top_srcdir)/goffice-plugins.mk
