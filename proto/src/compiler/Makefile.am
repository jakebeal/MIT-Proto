noinst_LTLIBRARIES = libcompiler.la

INCLUDES = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/shared

libcompiler_la_SOURCES = \
	lisp.cpp \
	reader.cpp \
	compiler-utils.cpp \
	lexer.cpp \
	enumlexer.cpp \
	compiler.cpp \
	interpreter.cpp \
	analyzer.cpp \
	emitter.cpp \
	neocompiler.cpp \
	nicenames.cpp

noinst_HEADERS = \
	sexpr.h \
	compiler-utils.h \
	lisp.h \
	reader.h \
	compiler.h \
	neocompiler.h \
	nicenames.h

enumlexer.cpp: $(srcdir)/one_enum.flex
	$(LEX) -Pzz -oenumlexer.cpp $(srcdir)/one_enum.flex

lexer.cpp: $(srcdir)/proto_syntax.flex sexpr.h compiler-utils.h
	$(LEX) -olexer.cpp $(srcdir)/proto_syntax.flex

# distribute the flex-generated files, since recent-enough versions of flex++
# are few and far between.
EXTRA_DIST = enumlexer.cpp lexer.cpp one_enum.flex proto_syntax.flex
MAINTAINERCLEANFILES = enumlexer.cpp lexer.cpp
