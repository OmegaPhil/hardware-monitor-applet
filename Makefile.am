SUBDIRS = src po # help

# server file
serverdir       = $(libdir)/bonobo/servers
server_in_files = HardwareMonitor.server.in
server_DATA     = $(server_in_files:.server.in=.server)

$(server_in_files): $(server_in_files:.server.in=.server.in.in) Makefile
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" -e "s|\@PACKAGE\@|$(PACKAGE)|" $< > $@

@INTLTOOL_SERVER_RULE@

# icon
icondir = $(datadir)/pixmaps
icon_DATA = $(PACKAGE)-applet.png

EXTRA_DIST = $(icon_DATA) $(server_DATA:.server=.server.in.in) \
             intltool-merge.in intltool-extract.in intltool-update.in \
             MAINTAINERS
