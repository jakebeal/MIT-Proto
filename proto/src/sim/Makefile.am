noinst_LTLIBRARIES = libsim.la

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/shared \
	-I$(top_srcdir)/src/kernel

libsim_la_SOURCES = \
	basic-hardware.cpp \
	customizations.cpp \
	kernel_extension.cpp \
	scheduler.cpp \
	sim-hardware.cpp \
	simpledynamics.cpp \
	spatialcomputer.cpp \
	unitdiscradio.cpp \
	radio.cpp \
	wormhole-radio.cpp \
	multiradio.cpp \
	LibRegistry.cpp 

libsim_la_CPPFLAGS =
libsim_la_LDFLAGS =

pkginclude_HEADERS = \
	basic-hardware.h \
	customizations.h \
	platform_ops.h \
	proto_platform.h \
	scheduler.h \
	sim-hardware.h \
	simpledynamics.h \
	spatialcomputer.h \
	unitdiscradio.h \
	radio.h \
	wormhole-radio.h \
	multiradio.h \
	LibRegistry.h \
	ProtoPluginLibrary.h

simplatdir=$(protoplatdir)/sim
simplat_DATA = \
	platform_ops.h \
	platform_ops.proto
EXTRA_DIST = $(simplat_DATA)

SUBDIRS=plugins
