noinst_LTLIBRARIES = libsim.la libdefaultplugin.la
lib_LTLIBRARIES = libprotosimplugin.la

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/shared \
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

buildregistrydir=$(plugindir)
buildregistry_SOURCES = \
	createRegistry.cpp
buildregistry_LDADD = \
	../shared/libshared.la libsim.la
buildregistry_LDFLAGS = -lltdl -export-dynamic -rdynamic
buildregistry_PROGRAMS = \
	buildregistry

defops_DATA = \
	platform_ops.proto \
	PerfectLocalizer.proto \
	SimpleDynamics.proto \
	simple-life-cycle.proto \
	mote-io.proto
defopsdir = $(protoplatdir)/sim/
EXTRA_DIST = $(defops_DATA)

# Basic plugins
pkglib_LTLIBRARIES = libdistributions.la libsimplelifecycle.la libmica2mote.la libradiomodels.la libstopwhen.la
pluginflags = -dynamiclib -avoid-version
pluginlibs = libprotosimplugin.la
# On non-Mac systems, might need to have -dynamiclib removed, -module added, or other such changes

# shared library for distributions
libdistributions_la_SOURCES = \
	DistributionsPlugin.cpp
libdistributions_la_LIBADD = $(pluginlibs)
libdistributions_la_LDFLAGS = $(pluginflags)

# shared library for SimpleLifeCycle plugin
libsimplelifecycle_la_SOURCES = \
	SimpleLifeCyclePlugin.cpp
libsimplelifecycle_la_LIBADD = $(pluginlibs)
libsimplelifecycle_la_LDFLAGS = $(pluginflags)

# shared library for Mica2Motes plugin
libmica2mote_la_SOURCES = \
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

# shared library for program-controlled stopping
libstopwhen_la_SOURCES = \
	stop-when.cpp
libstopwhen_la_LIBADD = $(pluginlibs)
libstopwhen_la_LDFLAGS = $(pluginflags)

# run the registry builder once everything's installed
install-data-hook:
	$(buildregistrydir)/buildregistry
