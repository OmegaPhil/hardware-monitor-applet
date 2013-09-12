AC_INIT(src/main.cpp)

AM_CONFIG_HEADER(config.h)

dnl package name and version number

PACKAGE=hardware-monitor
VERSION=1.4.3

AM_INIT_AUTOMAKE($PACKAGE, $VERSION)

dnl checks for programs
AC_PROG_INTLTOOL
AC_PROG_CXX
AC_PROG_CC


dnl checks for libraries
PKG_CHECK_MODULES(DEPS, gtkmm-2.4 >= 2.6.0 \
                        libgnomecanvasmm-2.6 >= 2.6.0 \
			libgnomeui-2.0 >= 2.0.0 \
			libglademm-2.4 >= 2.4.0 \
                        libgtop-2.0 >= 2.6.0)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

_cppflags="${CPPFLAGS}"
_ldflags="${LDFLAGS}"
 
AC_ARG_WITH(libsensors,
[  --with-libsensors[=DIR]   use libsensors in DIR],[
        if test "$withval" != "no" -a "$withval" != "yes"; then
                SENSORS_DIR=$withval
                CPPFLAGS="${CPPFLAGS} -I$withval/include/sensors"
                LDFLAGS="${LDFLAGS} -L$withval/lib"
        fi
])
if test "$with_libsensors" = "no"; then
        echo "Disabling lmsensors support"
else
        AC_CHECK_HEADERS(sensors/sensors.h,
                AC_CHECK_LIB(sensors, sensors_init,[
                        AC_DEFINE(HAVE_LIBSENSORS,1,[libsensors is available])
                        if test "x${SENSORS_DIR}" != "x"; then
                                SENSORS_CFLAGS="-I${SENSORS_DIR}/include"
                                SENSORS_LIBS="-L${SENSORS_DIR}/lib -lsensors"
                                [case ${host} in
                                        *-*-solaris*)
                                                SENSORS_LIBS="-L${SENSORS_DIR}/lib -R${SENSORS_DIR}/lib -lsensors"
                                                ;;
                                esac]
                        else
                                SENSORS_LIBS="-lsensors"
                        fi]))
fi

AC_SUBST(SENSORS_CFLAGS)
AC_SUBST(SENSORS_LIBS)

CPPFLAGS=${_cppflags}
LDFLAGS=${_ldflags}

dnl i18n
ALL_LINGUAS="ar as az be bg bn_IN bs ca cs cy da de dz el en_CA en_GB es eu fi fr gl gu he hi hr hu it ja kn ko lv mr ms nb ne nl oc pa pl pt pt_BR ru rw sl sq sr sr@Latn sv ta th tr uk vi zh_CN zh_HK zh_TW"
	
GETTEXT_PACKAGE=$PACKAGE
AC_SUBST(GETTEXT_PACKAGE)
AM_GLIB_GNU_GETTEXT
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", "definition of GETTEXT_PACKAGE")

dnl extra stuff

AM_MAINTAINER_MODE

dnl create makefiles
AC_OUTPUT(
	Makefile
	src/Makefile
	po/Makefile.in
dnl	help/Makefile
dnl	help/C/Makefile
)
