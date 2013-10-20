# Based off original GNOME 2 applet build files copyright:
#
# 2003-2007 Ole Laursen <olau@hardworking.dk>
#
# XFCE4 modifications copyright:
#
# 2013 OmegaPhil (OmegaPhil+hardware.monitor@gmail.com)


# Defining libraries to be built and their installation destination
plugindir = $(libdir)/xfce4/panel/plugins
plugin_LTLIBRARIES = libhardware-monitor.la

# List of source files needed to build the library
libhardware_monitor_la_SOURCES = \
                      applet.cpp applet.hpp \
                      bar-view.cpp bar-view.hpp \
                      choose-monitor-window.cpp choose-monitor-window.hpp \
                      canvas-view.cpp canvas-view.hpp \
                      column-view.cpp column-view.hpp \
                      curve-view.cpp curve-view.hpp \
                      flame-view.cpp flame-view.hpp \
                      gui-helpers.hpp \
                      helpers.cpp helpers.hpp \
                      i18n.hpp \
                      monitor.hpp \
                      monitor-impls.cpp monitor-impls.hpp \
                      preferences-window.cpp preferences-window.hpp \
                      pixbuf-drawing.hpp \
                      register_plugin.c \
                      text-view.cpp text-view.hpp \
                      view.cpp view.hpp \
                      value-history.cpp value-history.hpp \
                      ucompose.hpp

libhardware_monitor_la_LDFLAGS = \
                      -avoid-version \
                      -module \
                      -no-undefined \
    -export-symbols-regex '^xfce_panel_module_(preinit|init|construct)'


libhardware_monitor_la_CPPFLAGS = \
                        -DHARDWARE_MONITOR_GLADEDIR=\""$(gladedir)/"\" \
                        -DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
                        @LIBXFCE4PANEL_CFLAGS@ \
                        @LIBXFCE4UI_CFLAGS@

# TODO: This doesnt appear to be used?
#                        -DGNOMEICONDIR=\""$(icondir)"\"

libhardware_monitor_la_LIBADD = \
                        @LIBXFCE4PANEL_LIBS@ \
                        @LIBXFCE4UI_LIBS@ \
                        $(DEPS_LIBS) $(SENSORS_LIBS)


AM_CXXFLAGS = $(DEPS_CFLAGS) $(SENSORS_CFLAGS) -Wall -O3

glade_DATA = ui.glade
gladedir = $(datadir)/$(PACKAGE_NAME)/glade

# FIXME: both here and in ../Makefile.am?
icondir = $(datadir)/pixmaps

EXTRA_DIST = $(glade_DATA)
