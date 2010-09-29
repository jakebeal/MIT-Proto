pkglib_LTLIBRARIES = libexampleplugin.la
libexampleplugin_la_SOURCES = \
	ExamplePlugin.cpp
libexampleplugin_la_LDFLAGS = -dynamiclib -avoid-version -lprotosimplugin

registrydir="${prefix}/lib/proto/"

install-data-hook:
	$(registrydir)/buildregistry
