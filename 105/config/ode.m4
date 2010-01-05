AC_DEFUN([PROTO_WITH_ODE], [
    AC_ARG_WITH([ode],
                [AS_HELP_STRING([--with-ode[=DIR]],
                   [use Open Dynamics Engine, optionally in DIR/lib and DIR/include])],
                [use_ode=$withval],
                [use_ode=meh])

    if test x"$use_ode" != x"no"; then

        ODELDFLAGS=""
        ODEINCLUDE=""
        # if the user gave a path, append it to LDFLAGS
        case "$use_ode" in
            */*)
                ODELDFLAGS="-L$use_ode/lib"
                ODEINCLUDE="-I$use_ode/include"
                use_ode=yes
                ;;
        esac

        save_LDFLAGS="$LDFLAGS"
        save_CPPFLAGS="$CPPFLAGS"

        LDFLAGS="$LDFLAGS $ODELDFLAGS"
        CPPFLAGS="$CPPFLAGS $ODEINCLUDE"

        AC_CHECK_LIB([ode], [dBodyCreate],
            [
                have_ode=yes
                ODELIBS="-lode $LIBS"
            ], [
                have_ode=no
            ])

        LDFLAGS="$save_LDFLAGS"
        CPPFLAGS="$save_CPPFLAGS"

        if test x"$have_ode" = x"yes"; then
            WANT_ODE=true
            AC_SUBST(ODELDFLAGS)
            AC_SUBST(ODELIBS)
            AC_SUBST(ODEINCLUDE)
        else
            case $use_ode in
                yes)
                    AC_MSG_ERROR([Cannot find libode.])
                    ;;
                meh)
                    AC_MSG_WARN([Cannot find libode; will build without it])
                    ;;
            esac
            WANT_ODE=false
        fi
    else
        WANT_ODE=false
    fi

    AM_CONDITIONAL(WANT_ODE, $WANT_ODE)
    if $WANT_ODE; then
        AC_DEFINE([WANT_ODE], 1,
                  [Define if you are building against libode])
    fi
])
