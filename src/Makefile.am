# pass info on to source
INCLUDES = \
-DHARDWARE_MONITOR_GLADEDIR=\""$(gladedir)/"\" \
-DGNOMELOCALEDIR=\""$(gnomelocaledir)"\" \
-DGNOMEICONDIR=\""$(icondir)"\"

libexec_PROGRAMS = hardware-monitor

# list of source files needed to build the executable
hardware_monitor_SOURCES = \
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
                      main.cpp \
                      monitor.hpp \
                      monitor-impls.cpp monitor-impls.hpp \
                      preferences-window.cpp preferences-window.hpp \
                      pixbuf-drawing.hpp \
                      text-view.cpp text-view.hpp \
                      view.cpp view.hpp \
                      value-history.cpp value-history.hpp \
                      ucompose.hpp

AM_CXXFLAGS = $(DEPS_CFLAGS) $(SENSORS_CFLAGS) -Wall -O3

LDADD = $(DEPS_LIBS) $(SENSORS_LIBS)

glade_DATA = ui.glade
gladedir = $(datadir)/$(PACKAGE)/glade

# FIXME: both here and in ../Makefile.am?
icondir = $(datadir)/pixmaps

gnomelocaledir = $(datadir)/locale

EXTRA_DIST = $(glade_DATA)
