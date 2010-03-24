# TODO:
# SUBS = utils.o
# ifndef NO_GL
#   SUBS += Trackball.o drawing_primitives.o palette.o visualizer.o
# endif

noinst_LTLIBRARIES = libshared.la

INCLUDES = \
	-I$(top_srcdir)/src

libshared_la_SOURCES = \
	utils.cpp \
        drawing_primitives.cpp
if WANT_GLUT
libshared_la_SOURCES += \
        Trackball.cpp \
        palette.cpp \
        visualizer.cpp
endif

# TODO conditional here

pkginclude_HEADERS = \
	utils.h \
	Trackball.h \
	drawing_primitives.h \
	palette.h \
	proto_opcodes.h \
	visualizer.h

