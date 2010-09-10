noinst_LTLIBRARIES = libsim.la 
lib_LTLIBRARIES = libprotoplugin.la

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
	unitdiscradio.cpp \
	simpledynamics.cpp \
	UniformRandom.cpp \
	FixedIntervalTime.cpp \
	spatialcomputer.cpp \
	DefaultsPlugin.cpp

libsim_la_CPPFLAGS =
libsim_la_LDFLAGS = 
libsim_la_LIBADD = \
	libprotoplugin.la

libprotoplugin_la_SOURCES = \
	LibRegistry.cpp \
	Distribution.cpp \
	radio.cpp \
	plugin-support.cpp
libprotoplugin_la_LDFLAGS = -export-dynamic

pkginclude_HEADERS = \
	basic-hardware.h \
	customizations.h \
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
	ProtoPluginLibrary.h \
	Distribution.h \
	UniformRandom.h \
	SimpleLifeCycle.h \
	Mica2Mote.h \
	DeviceTimer.h \
	TimeModel.h \
	FixedIntervalTime.h \
	DefaultsPlugin.h

simplatdir=$(protoplatdir)/sim
simplat_DATA = \
	platform_ops.proto

buildregistrydir=$(plugindir)
buildregistry_SOURCES = \
	createRegistry.cpp
buildregistry_LDADD = \
	../shared/libshared.la libsim.la
buildregistry_LDFLAGS = -lltdl -export-dynamic
buildregistry_PROGRAMS = \
	buildregistry

EXTRA_DIST = $(simplat_DATA)

# Basic plugins
pkglib_LTLIBRARIES = libdistributions.la libsimplelifecycle.la libmica2mote.la libradiomodels.la
pluginflags = -dynamiclib -avoid-version #-no-undefined
pluginlibs = ../shared/libshared.la libprotoplugin.la
# On non-Mac systems, might need to have -dynamiclib removed, -module added, or other such changes

# shared library for distributions
libdistributions_la_SOURCES = \
	XGrid.cpp \
	FixedPoint.cpp \
	Grid.cpp \
	GridRandom.cpp \
	Cylinder.cpp \
	Torus.cpp \
	Distribution.cpp \
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

#SUBDIRS=plugins

# run_scons:
# 	scons "glut_libs=$(GLUT_LIBS)" "glu_libs=$(GLU_LIBS)" "gl_libs=$(GL_LIBS)"
#
# run_createRegistry: run_scons
# 	./createRegistry
#
# copy_registry_file:run_createRegistry
# 	cp registry.txt $(simplatdir)
# 	cp -r plugin_lib $(simplatdir)
# 	cp 
#
# install-exec-hook: copy_registry_file	
#
#uninstall-local:
#	rm ../tests/registry.txt
#	rm -rf ../tests/plugin-lib


install-data-hook:
	$(buildregistrydir)/buildregistry
