AUTOMAKE_OPTIONS = subdir-objects

pkglib_LTLIBRARIES = libproto-ode.la
include_HEADERS  = src/odedynamics.h  src/odebody.h  src/odebodyfactory.h config.h
#noinst_HEADERS  = src/odedynamics.h  src/odebody.h src/odeshapes.h  config.h

libproto_ode_la_SOURCES  =  src/odebody.cpp src/odedynamics.cpp src/odebodyfactory.cpp src/xerces_bodies.cpp
#src/odecube.cpp
libproto_ode_la_LDFLAGS  = -module $(ODELDFLAGS) -dynamiclib -avoid-version -lprotosimplugin -lode

libproto_ode_la_INCLUDES = $(ODEINCLUDE)
libproto_ode_la_LIBADD   = $(ODELIBS)  -lxerces-c 

#ACLOCAL_AMFLAGS = -I ../../config

registrydir="${prefix}/lib/proto/"
libdir="${registrydir}"
install-data-hook:
	$(registrydir)/buildregistry