noinst_LTLIBRARIES = libgoffice-data.la

libgoffice_data_la_SOURCES =	\
	go-data.c 		\
	go-data-simple.c

libgoffice_data_ladir = $(goffice_include_dir)/data
libgoffice_data_la_HEADERS = 	\
	goffice-data.h		\
	go-data.h 		\
	go-data-impl.h 		\
	go-data-simple.h 

include $(top_srcdir)/goffice.mk

LIB_PUBLIC_HDRS = $(libgoffice_data_la_HEADERS)
