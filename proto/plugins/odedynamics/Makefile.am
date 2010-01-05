lib_LTLIBRARIES = proto-ode.la
noinst_HEADERS  = odedynamics.h

proto_ode_la_SOURCES  = odedynamics.cpp
proto_ode_la_LDFLAGS  = -module $(ODELDFLAGS)
proto_ode_la_INCLUDES = $(ODEINCLUDE)
proto_ode_la_LIBADD   = $(ODELIBS)

ACLOCAL_AMFLAGS = -I ../../config
