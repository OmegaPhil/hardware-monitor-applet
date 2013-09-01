/* The applet class which coordinates everything.
 *
 * Copyright (c) 2003, 04, 05 Ole Laursen.
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

#ifndef APPLET_HPP
#define APPLET_HPP

#include <memory>
#include <list>

#include <sigc++/connection.h>

#include <gtkmm/eventbox.h>
#include <gtkmm/window.h>
#include <gtkmm/tooltips.h>
#include <gtkmm/aboutdialog.h>

#include <glibmm/ustring.h>

#include <gconfmm/client.h>
#include <gconfmm/entry.h>


#include <panel-applet.h>

#include "monitor.hpp"

class PreferencesWindow; 
class View;


// main monster GUI class
class Applet: public Gtk::EventBox
{
public:
  Applet(PanelApplet *panel_applet);
  ~Applet();

  Gtk::Container &get_container();
  
  unsigned int get_fg_color();	// return varying foreground colours
  int get_size() const;		// in pixels
  bool horizontal() const; 	// whether we're in horizontal mode
  void set_view(View *view);	// use this view to monitor

  Glib::RefPtr<Gnome::Conf::Client> &get_gconf_client();
  Glib::ustring get_gconf_dir() const;

  Glib::RefPtr<Gdk::Pixbuf> get_icon();	// get the application icon

  void add_monitor(Monitor *monitor); // take over ownership of monitor
  void remove_monitor(Monitor *monitor); // get rid of the monitor
  void replace_monitor(Monitor *prev_monitor, Monitor *new_monitor);

  static int const update_interval = 1000;
  
private:
  // monitors
  monitor_seq monitors;
  void add_sync_for(Monitor *monitor);
  void remove_sync_for(Monitor *monitor);

  // the context menu
  void on_preferences_activated();
  void on_help_activated();
  void on_about_activated();

  // looping
  bool main_loop();
  sigc::connection timer;

  // GConf
  Glib::RefPtr<Gnome::Conf::Client> gconf_client;
  Glib::ustring gconf_dir;

  void viewer_type_listener(unsigned int, Gnome::Conf::Entry entry);
  void background_color_listener(unsigned int, Gnome::Conf::Entry entry);
  void use_background_color_listener(unsigned int, Gnome::Conf::Entry entry);
  
  Glib::ustring find_empty_monitor_dir();
  
  // data
  PanelApplet *panel_applet;
  Glib::RefPtr<Gdk::Pixbuf> icon;
  std::auto_ptr<Gtk::AboutDialog> about;
  std::auto_ptr<View> view;
  std::auto_ptr<PreferencesWindow> preferences_window;

  Gtk::Tooltips tooltips;
  
  friend void display_preferences(BonoboUIComponent *, void *applet, const gchar *);

  friend void display_help(BonoboUIComponent *, void *applet, const gchar *);
  friend void display_about(BonoboUIComponent *, void *applet, const gchar *);
};

#endif
