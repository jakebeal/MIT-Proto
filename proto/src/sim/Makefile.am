noinst_LTLIBRARIES = libsim.la libdefaultplugin.la
lib_LTLIBRARIES = libprotosimplugin.la
bin_PROGRAMS = opsim

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/shared \
	-I$(top_srcdir)/src/compiler \
	-I$(top_srcdir)/src/kernel

libprotosimplugin_la_SOURCES = \
	radio.cpp \
	plugin-support.cpp
libprotosimplugin_la_LDFLAGS = -export-dynamic

libdefaultplugin_la_SOURCES = \
	basic-hardware.cpp \
	unitdiscradio.cpp \
	simpledynamics.cpp \
	FixedIntervalTime.cpp \
	DefaultsPlugin.cpp
libdefaultplugin_la_LDFLAGS = -export-dynamic

libsim_la_SOURCES = \
	kernel_extension.cpp \
	scheduler.cpp \
	sim-hardware.cpp \
	spatialcomputer.cpp

libsim_la_CPPFLAGS =
libsim_la_LDFLAGS = 
libsim_la_LIBADD = \
	libprotosimplugin.la \
	libdefaultplugin.la

# Note: scheduler.h is not user-interesting, but is used by spatialcomputer.h
# TODO: proto_platform.h should go in sim subdir
pkginclude_HEADERS = \
	basic-hardware.h \
	proto_platform.h \
	scheduler.h \
	sim-hardware.h \
	simpledynamics.h \
	spatialcomputer.h \
	unitdiscradio.h \
	radio.h \
	UniformRandom.h \
	FixedIntervalTime.h

opsim_SOURCES = opsim.cpp

opsim_LDADD = \
	../kernel/libkernel.la \
	../shared/libshared.la \
	../compiler/libcompiler.la \
	../motelink/libmotelink.la \
	libsim.la \
	libprotosimplugin.la \
	libdefaultplugin.la \
	../shared/libshared.la \
	../kernel/libkernel.la \
	../compiler/libcompiler.la

opsim_LDFLAGS = -lltdl -export-dynamic -rdynamic -rpath ${libdir}
