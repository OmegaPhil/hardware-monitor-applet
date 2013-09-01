/* Main program.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of the
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

#include <config.h>

#include <unistd.h>		// for nice
#include <libintl.h>

#include <libgnomecanvasmm/init.h>
#include <gconfmm/init.h>
#include <gtkmm/main.h>

#include <libgnomeui/gnome-ui-init.h>
#include <libgnomeui/gnome-client.h>
#include <panel-applet.h>

#include "applet.hpp"
#include "helpers.hpp"
#include "i18n.hpp"

gboolean hardware_monitor_factory(PanelApplet *panel_applet, const gchar *iid,
				  void *)
{
  // construct applet
  Applet *applet = manage(new Applet(panel_applet));
  applet->show();
  
  gtk_container_add(GTK_CONTAINER(panel_applet), GTK_WIDGET(applet->gobj()));
  gtk_widget_show(GTK_WIDGET(panel_applet));
  
  return true;
}

int main(int argc, char *argv[])
{
  nice(5);			// don't eat up too much CPU

  Gtk::Main main(argc, argv);
  Gnome::Conf::init();
  Gnome::Canvas::init();

  // this is necessary for panel applets
  gnome_program_init(PACKAGE, VERSION,
		     LIBGNOMEUI_MODULE,
		     argc, argv, GNOME_CLIENT_PARAM_SM_CONNECT, FALSE,
		     NULL);
		   
  try {
    // i18n
    bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    panel_applet_factory_main("OAFIID:HardwareMonitor_Factory",
			      PANEL_TYPE_APPLET,
			      &hardware_monitor_factory, 0);
  }
  catch(const Glib::Error &ex) {
    fatal_error(ex.what());
  }
}
