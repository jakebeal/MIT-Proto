AC_DEFUN([PROTO_WITH_NEOCOMPILER], [
    AC_ARG_WITH([neocompiler],
                [AS_HELP_STRING([--with-neocompiler],
                   [use new-version Proto compiler])],
                [use_neocompiler=$withval],
#                [use_neocompiler=meh])
                [use_neocompiler="no"])

    if test x"$use_neocompiler" != x"no"; then
       	__USE_NEOCOMPILER__=true
    else
        __USE_NEOCOMPILER__=false
    fi

    AM_CONDITIONAL(__USE_NEOCOMPILER__, $__USE_NEOCOMPILER__)
    if $__USE_NEOCOMPILER__; then
        AC_DEFINE([__USE_NEOCOMPILER__], 1,
                  [Define if you want to use the new-version Proto compiler])
    fi
])
