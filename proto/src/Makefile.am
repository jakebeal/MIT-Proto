# NOTE: order is important in this list!
SUBDIRS = \
	shared \
	compiler \
	sim \
	kernel \
	motelink \
	tests \
	.

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/shared \
	-I$(top_srcdir)/src/kernel \
	-I$(top_srcdir)/src/compiler \
	-I$(top_srcdir)/src/motelink \
	-I$(top_srcdir)/src/sim

bin_PROGRAMS = \
	proto \
	p2b

proto_SOURCES = \
	sim-app.cpp

# FIXME: have to list libkernel twice here, due to circular dependency
# with libsim...
proto_LDADD = \
	kernel/libkernel.la \
	shared/libshared.la \
	compiler/libcompiler.la \
	motelink/libmotelink.la \
	sim/libsim.la \
	sim/libprotosimplugin.la \
	sim/libdefaultplugin.la \
	shared/libshared.la \
	kernel/libkernel.la \
	compiler/libcompiler.la

proto_LDFLAGS = \
	-export-dynamic

p2b_SOURCES = \
	compiler-app.cpp

p2b_LDADD = \
	shared/libshared.la \
	compiler/libcompiler.la
