/* Copyright (c) 2013 OmegaPhil (OmegaPhil+hardware.monitor@gmail.com)
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 */


#include <libxfce4panel/xfce-panel-plugin.h>

/* It looks like plugin registration MUST happen in a pure C file - doing
 * this in a C linkage block in applet.cpp is not good enough. Because
 * of this, AC_PROG_CC must be present in configure.ac as well */
extern void applet_construct(XfcePanelPlugin* plugin);

/* 'Registering' the applet - in reality this substitutes into a load
 * of functions including a main */
XFCE_PANEL_PLUGIN_REGISTER(applet_construct)
