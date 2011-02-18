AUTOMAKE_OPTIONS = subdir-objects

pkglib_LTLIBRARIES = libproto-ode.la
include_HEADERS  = src/odedynamics.h  src/odebody.h  src/odebodyfactory.h config.h

libproto_ode_la_SOURCES  =  src/odebody.cpp src/odedynamics.cpp src/odebodyfactory.cpp src/xerces_bodies.cpp
libproto_ode_la_LDFLAGS  = -module $(ODELDFLAGS) -dynamiclib -avoid-version -lprotosimplugin -lode

libproto_ode_la_INCLUDE = $(ODEINCLUDE)
libproto_ode_la_LIBADD   = $(ODELIBS)  $(XERCES_LIBS)
libproto_ode_la_CPPFLAGS = $(XERCES_CFLAGS)

registrydir="${prefix}/lib/proto/"
install-data-hook:
	$(registrydir)/buildregistry