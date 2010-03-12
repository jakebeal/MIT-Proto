AC_DEFUN([PROTO_PROTOLIBDIR], [
    protolibdir="${datarootdir}/proto/lib"
    AC_SUBST(protolibdir)
    AC_DEFINE_DIR([PROTOLIBDIR], [protolibdir], [default directory containing .proto files])

    protoplatdir="${datarootdir}/proto/platforms"
    AC_SUBST(protoplatdir)
    AC_DEFINE_DIR([PROTOPLATDIR], [protoplatdir], [base directory for platform-specific files])
])
