dnl Based off original GNOME 2 applet build files copyright:
dnl
dnl 2003-2007 Ole Laursen <olau@hardworking.dk>
dnl

dnl Based off xfce4-genmon-plugins build files, which are copyright:
dnl
dnl 2004 Roger Seguin <roger_seguin@msn.com>
dnl 2006 Julien Devemy <jujucece@gmail.com>
dnl

dnl XFCE4 modifications copyright:
dnl
dnl 2013 OmegaPhil (OmegaPhil+hardware.monitor@gmail.com)
dnl


dnl Initialising autoconf - this provides PACKAGE_NAME and PACKAGE_VERSION
dnl output variables
AC_INIT([hardware-monitor], [1.4.3])

dnl Initialise automake with minimum version 1.13.2 (based off https://www.flameeyes.eu/autotools-mythbuster/forwardporting/automake.html),
dnl enabling all warnings and errors
AM_INIT_AUTOMAKE([1.13.2 -Wall -Werror])

dnl Collecting all macro definitions together into this header
AM_CONFIG_HEADER(config.h)

dnl Tidy directory for containing extra m4 macros
AC_CONFIG_MACRO_DIR([m4])

AM_MAINTAINER_MODE

dnl Check for UNIX variants
AC_AIX
AC_ISC_POSIX
AC_MINIX

dnl Check for basic programs - AM_PROG_AR gets around non-POSIX archiver
dnl warning
AC_PROG_CXX
AC_PROG_INSTALL
IT_PROG_INTLTOOL([0.40.0])
AM_PROG_AR

dnl Initialize libtool, prevent static libraries to remove pointless
dnl compilation
LT_PREREQ([2.2.6])
LT_INIT([disable-static])

dnl Check for standard header files
AC_HEADER_STDC

dnl Configure the panel plugin (XFCE4 xdt-depends.m4)
XDT_CHECK_PACKAGE([LIBXFCE4PANEL], [libxfce4panel-1.0], [4.8.0])
XDT_CHECK_PACKAGE([LIBXFCE4UI], [libxfce4ui-1], [4.8.0])

dnl Checks for libraries
PKG_CHECK_MODULES([DEPS], [gtkmm-2.4 >= 2.6.0 \
                        libgnomecanvasmm-2.6 >= 2.6.0 \
			libglademm-2.4 >= 2.4.0 \
                        libgtop-2.0 >= 2.6.0])

dnl Translations (XFCE4 xdt-i18n.m4) - this needs to be done before the
dnl libsensors checking for some reason?? Otherwise the assembler tries
dnl to open these files in the current directory??
XDT_I18N([ar as az be bg bn_IN bs ca cs cy da de dz el en_CA en_GB es eu fi fr gl gu he hi hr hu it ja kn ko lv mr ms nb ne nl oc pa pl pt pt_BR ru rw sl sq sr sr@Latn sv ta th tr uk vi zh_CN zh_HK zh_TW])

AC_SUBST([DEPS_CFLAGS])
AC_SUBST([DEPS_LIBS])

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

AC_SUBST([SENSORS_CFLAGS])
AC_SUBST([SENSORS_LIBS])

CPPFLAGS=${_cppflags}
LDFLAGS=${_ldflags}

dnl Check for debugging support (XFCE4 xdt-features.m4). TODO: This macro appears to be buggy - complaints about AC_LANG_SOURCE?
dnl XDT_FEATURE_DEBUG()

dnl Create makefiles
AC_CONFIG_FILES([Makefile 
	src/Makefile
	po/Makefile.in
])
AC_OUTPUT
