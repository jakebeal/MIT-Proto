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
	LibRegistry.cpp \
	Distribution.cpp \
	XGrid.cpp \
	UniformRandom.cpp \
	FixedPoint.cpp \
	Grid.cpp \
	GridRandom.cpp \
	Cylinder.cpp \
	Torus.cpp \
	SimpleLifeCycle.cpp \
	Mica2Mote.cpp \
	FixedIntervalTime.cpp \
	DefaultsPlugin.cpp

libsim_la_CPPFLAGS =
libsim_la_LDFLAGS =

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
	XGrid.h \
	UniformRandom.h \
	FixedPoint.h \
	Grid.h \
	GridRandom.h \
	Cylinder.h \
	Torus.h \
	SimpleLifeCycle.h \
	Mica2Mote.h \
	DeviceTimer.h \
	TimeModel.h \
	FixedIntervalTime.h \
	DefaultsPlugin.h

simplatdir=$(protoplatdir)/sim
simplat_DATA = \
	platform_ops.proto
EXTRA_DIST = $(simplat_DATA)

SUBDIRS=plugins

run_scons:
	scons "glut_libs=$(GLUT_LIBS)" "glu_libs=$(GLU_LIBS)" "gl_libs=$(GL_LIBS)"

run_createRegistry: run_scons
	./createRegistry

copy_registry_file:run_createRegistry
	cp registry.txt ../tests
	cp -r plugin_lib ../tests    

install-exec-hook: copy_registry_file	

uninstall-local:
	rm ../tests/registry.txt
	rm -rf ../tests/plugin-lib

