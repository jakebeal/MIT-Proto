lib_LTLIBRARIES =
noinst_HEADERS  =


INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/shared \
	-I$(top_srcdir)/src/kernel \
	-I$(top_srcdir)/src/sim

lib_LTLIBRARIES += proto-stop-when.la
noinst_HEADERS  += stop-when.h

proto_stop_when_la_SOURCES = stop-when.cpp
proto_stop_when_la_LDFLAGS = -module
