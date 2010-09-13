noinst_LTLIBRARIES = libsim.la 
lib_LTLIBRARIES = libprotosimplugin.la

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/shared \
	-I$(top_srcdir)/src/kernel

libprotosimplugin_la_SOURCES = \
	radio.cpp \
	plugin-support.cpp
libprotosimplugin_la_LDFLAGS = -export-dynamic

libsim_la_SOURCES = \
	basic-hardware.cpp \
	kernel_extension.cpp \
	scheduler.cpp \
	sim-hardware.cpp \
	unitdiscradio.cpp \
	simpledynamics.cpp \
	UniformRandom.cpp \
	FixedIntervalTime.cpp \
	spatialcomputer.cpp \
	DefaultsPlugin.cpp

libsim_la_CPPFLAGS =
libsim_la_LDFLAGS = 
libsim_la_LIBADD = \
	libprotosimplugin.la

pkginclude_HEADERS = \
	basic-hardware.h \
	proto_platform.h \
	scheduler.h \
	sim-hardware.h \
	simpledynamics.h \
	spatialcomputer.h \
	unitdiscradio.h \
	radio.h \
	wormhole-radio.h \
	multiradio.h \
	UniformRandom.h \
	Mica2Mote.h \
	DeviceTimer.h \
	TimeModel.h \
	FixedIntervalTime.h \
	DefaultsPlugin.h

buildregistrydir=$(plugindir)
buildregistry_SOURCES = \
	createRegistry.cpp
buildregistry_LDADD = \
	../shared/libshared.la libsim.la
buildregistry_LDFLAGS = -lltdl -export-dynamic
buildregistry_PROGRAMS = \
	buildregistry

# Basic plugins
pkglib_LTLIBRARIES = libdistributions.la libsimplelifecycle.la libmica2mote.la libradiomodels.la
pluginflags = -dynamiclib -avoid-version #-no-undefined
pluginlibs = ../shared/libshared.la libprotosimplugin.la
# On non-Mac systems, might need to have -dynamiclib removed, -module added, or other such changes

# shared library for distributions
libdistributions_la_SOURCES = \
	XGrid.cpp \
	FixedPoint.cpp \
	Grid.cpp \
	GridRandom.cpp \
	Cylinder.cpp \
	Torus.cpp \
	UniformRandom.cpp \
	DistributionsPlugin.cpp
libdistributions_la_LIBADD = $(pluginlibs)
libdistributions_la_LDFLAGS = $(pluginflags)

# shared library for SimpleLifeCycle plugin
libsimplelifecycle_la_SOURCES = \
	SimpleLifeCycle.cpp \
	SimpleLifeCyclePlugin.cpp
libsimplelifecycle_la_LIBADD = $(pluginlibs)
libsimplelifecycle_la_LDFLAGS = $(pluginflags)

# shared library for Mica2Motes plugin
libmica2mote_la_SOURCES = \
	Mica2Mote.cpp \
	Mica2MotePlugin.cpp
libmica2mote_la_LIBADD = $(pluginlibs)
libmica2mote_la_LDFLAGS = $(pluginflags)

# shared library for radio plugin
libradiomodels_la_SOURCES = \
	wormhole-radio.cpp \
	multiradio.cpp \
	RadioModelsPlugin.cpp
libradiomodels_la_LIBADD = $(pluginlibs)
libradiomodels_la_LDFLAGS = $(pluginflags)

# run the registry builder once everything's installed
install-data-hook:
	$(buildregistrydir)/buildregistry
