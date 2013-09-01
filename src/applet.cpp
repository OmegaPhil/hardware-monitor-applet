/* Implementation of the Applet class.
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


#include <config.h>

#include <algorithm>
#include <vector>

#include <gtkmm/main.h>
#include <cassert>

#include "ucompose.hpp"

#include "applet.hpp"
#include "column-view.hpp"
#include "curve-view.hpp"
#include "bar-view.hpp"
#include "text-view.hpp"
#include "flame-view.hpp"
#include "monitor.hpp"
#include "preferences-window.hpp"
#include "i18n.hpp"

// helpers for popping up the various things

void display_preferences(BonoboUIComponent *, void *applet, const gchar *)
{
  static_cast<Applet *>(applet)->on_preferences_activated();
}

void display_help(BonoboUIComponent *, void *applet, const gchar *)
{
  static_cast<Applet *>(applet)->on_help_activated();
}

void display_about(BonoboUIComponent *, void *applet, const gchar *)
{
  static_cast<Applet *>(applet)->on_about_activated();
}


namespace
{
  template <typename T>
    Gnome::Conf::Entry get_entry_with_default(
      Glib::RefPtr<Gnome::Conf::Client> &gconf_client,
      const Glib::ustring &key,
      const T &value )
    {
      Gnome::Conf::Entry entry = gconf_client->get_entry(key);
      if (!entry.gobj())
      {
        gconf_client->set( key, value );
        entry = gconf_client->get_entry(key);
      }
      return entry;
    }
}


Applet::Applet(PanelApplet *a)
  : panel_applet(a)
{
  // load icon
  std::string icon_name = GNOMEICONDIR "/hardware-monitor-applet.png";
  try {
    icon = Gdk::Pixbuf::create_from_file(icon_name);
  }
  catch(...) {
    std::cerr <<
      String::ucompose(_("Hardware Monitor: cannot load the icon '%1'.\n"),
		       icon_name);
    // it's a minor problem if we can't find the icon
    icon = Glib::RefPtr<Gdk::Pixbuf>();
  }
  
  // setup GConf
  gconf_dir = panel_applet_get_preferences_key(panel_applet);
  gconf_client = Gnome::Conf::Client::get_default_client();
  gconf_client->add_dir(gconf_dir);

  // circumvent GConf bug (FIXME: report it)
  gconf_client->set(gconf_dir + "/dummy", 0);
  gconf_client->set(gconf_dir + "/monitors/dummy", 0);
  
  // connect GConf
  gconf_client->notify_add(gconf_dir + "/viewer_type",
			   sigc::mem_fun(*this, &Applet::viewer_type_listener));
  gconf_client->notify_add(gconf_dir + "/background_color",
			   sigc::mem_fun(*this, &Applet::
				      background_color_listener));
  gconf_client->notify_add(gconf_dir + "/use_background_color",
			   sigc::mem_fun(*this, &Applet::
				      use_background_color_listener));

  monitor_seq mon = load_monitors(gconf_client, gconf_dir);
  for (monitor_iter i = mon.begin(), end = mon.end(); i != end; ++i)
    add_monitor(*i);
  
  // we need this #if 0 so the strings will be in the message catalog for
  // translation below in menu_xml
#if 0
  // pop-up menu strings
    _("_Preferences...") _("_Help") _("_About...")
#endif
  
  // setup menu
  static const char menu_xml[] =
    "<popup name=\"button3\">\n"
    "   <menuitem name=\"Properties Item\"\n"
    "             verb=\"HardwareMonitorPreferences\" _label=\"_Preferences...\"\n"
    "             pixtype=\"stock\" pixname=\"gtk-properties\"/>\n"
    "   <menuitem name=\"Help Item\"\n"
    "             verb=\"HardwareMonitorHelp\" _label=\"_Help\"\n"
    "             pixtype=\"stock\" pixname=\"gtk-help\"/>\n"
    "   <menuitem name=\"About Item\"\n"
    "             verb=\"HardwareMonitorAbout\" _label=\"_About...\"\n"
    "             pixtype=\"stock\" pixname=\"gnome-stock-about\"/>\n"
    "</popup>\n";

  static const BonoboUIVerb menu_verbs[] = {
    BONOBO_UI_VERB("HardwareMonitorPreferences", &display_preferences),
    BONOBO_UI_VERB("HardwareMonitorHelp", &display_help),
    BONOBO_UI_VERB("HardwareMonitorAbout", &display_about),
    
    BONOBO_UI_VERB_END
  };
  
  panel_applet_setup_menu(panel_applet, menu_xml, menu_verbs, this);
  
  // start displaying something
  // Ensure config values aren't null
  get_entry_with_default( gconf_client, gconf_dir + "/viewer_type",
    Glib::ustring("curve") );
  get_entry_with_default( gconf_client, gconf_dir + "/background_color",
    0x00000000 );
  get_entry_with_default( gconf_client, gconf_dir + "/use_background_color",
    false );
  viewer_type_listener(0, gconf_client->get_entry(gconf_dir + "/viewer_type"));
  background_color_listener(0, gconf_client->get_entry(gconf_dir
						       + "/background_color"));
  use_background_color_listener(0, gconf_client->get_entry(gconf_dir
							   + "/use_background_color"));

  timer =
    Glib::signal_timeout().connect(sigc::mem_fun(*this, &Applet::main_loop),
				   update_interval);
  main_loop();
}

Applet::~Applet()
{
  timer.disconnect();
  
  // make sure noone is trying to read the monitors before we kill them
  if (view.get())
    for (monitor_iter i = monitors.begin(), end = monitors.end(); i != end; ++i)
      view->detach(*i);
  
  view.reset();
  
  for (monitor_iter i = monitors.begin(), end = monitors.end(); i != end; ++i) {
    (*i)->save(gconf_client);	// save max. values
    delete *i;
  }
}

void Applet::set_view(View *v)
{
  if (view.get())
    for (monitor_iter i = monitors.begin(), end = monitors.end(); i != end; ++i)
      view->detach(*i);
  
  view.reset(v);
  view->display(*this);

  for (monitor_iter i = monitors.begin(), end = monitors.end(); i != end; ++i)
    view->attach(*i);
}

bool Applet::main_loop()
{
  // update view
  if (view.get())
    view->update();
  
  // update tooltip
  Glib::ustring tip;
  for (monitor_iter i = monitors.begin(), end = monitors.end(); i != end; ++i) {
    Monitor &mon = **i;

    // note to translators: %1 is the name of a monitor, e.g. "CPU 1", and %2 is
    // the current measurement, e.g. "78%"
    Glib::ustring next = String::ucompose(_("%1: %2"), mon.get_short_name(),
					  mon.format_value(mon.value()));
    if (tip.empty())
      tip = next;
    else
      // note to translators: this is used for composing a list of monitors; %1
      // is the previous part of the list and %2 is the part to append
      tip = String::ucompose(_("%1\n%2"), tip, next);
  }
  tooltips.set_tip(get_container(), tip);

  return true;
}

Gtk::Container &Applet::get_container()
{
  return *this;
}

unsigned int Applet::get_fg_color()
{
  static unsigned int colors[] = {
    0x83A67FB0, 0xC1665AB0, 0x7590AEB0, 0xE0C39ED0, 0x887FA3B0
  };

  Gnome::Conf::Value v = gconf_client->get(gconf_dir + "/next_color");
  int c;
      
  if (v.get_type() == Gnome::Conf::VALUE_INT)
    c = v.get_int();
  else
    c = 0;

 gconf_client->set(gconf_dir + "/next_color",
		   int((c + 1) % (sizeof(colors) / sizeof(unsigned int))));
  
 return colors[c];
}

int Applet::get_size() const
{
  // FIXME: the panel appears to lie about its true size
  // 2 pixels of frame decoration on both sides
  return panel_applet_get_size(panel_applet) - 2 * 2;
}

bool Applet::horizontal() const
{
  PanelAppletOrient orient = panel_applet_get_orient(panel_applet);
  return orient == PANEL_APPLET_ORIENT_UP || orient == PANEL_APPLET_ORIENT_DOWN;
}

Glib::RefPtr<Gnome::Conf::Client> &Applet::get_gconf_client()
{
  return gconf_client;
}

Glib::ustring Applet::get_gconf_dir() const
{
  return gconf_dir;
}

Glib::RefPtr<Gdk::Pixbuf> Applet::get_icon()
{
  return icon;
}

void Applet::add_monitor(Monitor *monitor)
{
  add_sync_for(monitor);
  monitors.push_back(monitor);

  if (monitor->get_gconf_dir().empty()) {
    monitor->set_gconf_dir(find_empty_monitor_dir());
    monitor->save(gconf_client);
  }
  else
    monitor->load(gconf_client);
  
  if (view.get())
    view->attach(monitor);
}

void Applet::remove_monitor(Monitor *monitor)
{
  if (view.get())
    view->detach(monitor);

  // delete the GConf keys
  Glib::SListHandle<Gnome::Conf::Entry> list
    = gconf_client->all_entries(monitor->get_gconf_dir());

  for (Glib::SListHandle<Gnome::Conf::Entry>::const_iterator i = list.begin(),
	end = list.end(); i != end; ++i)
    gconf_client->unset((*i).get_key());

  // everyone has been notified, it's now safe to remove and delete
  // the monitor
  monitors.remove(monitor);
  remove_sync_for(monitor);
  
  delete monitor;
}

void Applet::replace_monitor(Monitor *prev_mon, Monitor *new_mon)
{
  monitor_iter i = std::find(monitors.begin(), monitors.end(), prev_mon);
  assert(i != monitors.end());
  
  add_sync_for(new_mon);
  *i = new_mon;

  new_mon->set_gconf_dir(prev_mon->get_gconf_dir());
  new_mon->load(gconf_client);
  new_mon->save(gconf_client);
  
  if (view.get()) {
    view->detach(prev_mon);
    view->attach(new_mon);
  }

  remove_sync_for(prev_mon);
  delete prev_mon;
}

void Applet::add_sync_for(Monitor *monitor)
{
  for (monitor_iter i = monitors.begin(), end = monitors.end(); i != end; ++i)
    (*i)->possibly_add_sync_with(monitor);
}

void Applet::remove_sync_for(Monitor *monitor)
{
  for (monitor_iter i = monitors.begin(), end = monitors.end(); i != end; ++i)
    (*i)->remove_sync_with(monitor);
}

void Applet::viewer_type_listener(unsigned int, Gnome::Conf::Entry entry)
{
  if (entry.get_value().get_type() != Gnome::Conf::VALUE_STRING) {
    // FIXME: use schema for this?
    Gnome::Conf::Value v(Gnome::Conf::VALUE_STRING);
    v.set(Glib::ustring("curve"));
    entry.set_value(v);

    gconf_client->set(entry.get_key(), Glib::ustring("curve"));
  }

  Glib::ustring s = entry.get_value().get_string();

  // FIXME: move this
  if (s == "curve") {
    if (!dynamic_cast<CurveView *>(view.get()))
      set_view(new CurveView);
  }
  else if (s == "bar") {
    // It get's tricky here because them BarView can render 2 viewers.
    // Thus, we much also check the oriententation
    BarView *bar_view = dynamic_cast<BarView *>(view.get());
    if (!(bar_view && bar_view->is_horizontal()) )
      set_view(new BarView);
  }
  else if (s == "vbar") {
    // Same situation as with "bar"
    BarView *bar_view = dynamic_cast<BarView *>(view.get());
    if (!(bar_view && !bar_view->is_horizontal()) )
      set_view(new BarView(false));
  }
  else if (s == "text") {
    if (!dynamic_cast<TextView *>(view.get()))
      set_view(new TextView);
  }
  else if (s == "flame") {
    if (!dynamic_cast<FlameView *>(view.get()))
      set_view(new FlameView);
  }
  else if (s == "column") {
    if (!dynamic_cast<ColumnView *>(view.get()))
      set_view(new ColumnView);
  }

  // make sure the view sets the background
  background_color_listener(0, gconf_client->get_entry(gconf_dir
						       + "/background_color"));
}

void Applet::background_color_listener(unsigned int, Gnome::Conf::Entry entry)
{
  if (entry.get_value().get_type() != Gnome::Conf::VALUE_INT) {
    // FIXME: use schema for this?
    Gnome::Conf::Value v(Gnome::Conf::VALUE_INT);
    v.set(0x00000000);		// black as the night
    entry.set_value(v);
  }

  unsigned int color = entry.get_value().get_int();

  Gnome::Conf::Value v
    = gconf_client->get(gconf_dir + "/use_background_color");

  bool use;

  if (v.get_type() != Gnome::Conf::VALUE_BOOL)
    use = false;
  else
    use = v.get_bool();

  if (view.get())
    if (use)
      view->set_background(color);
}

void Applet::use_background_color_listener(unsigned int, Gnome::Conf::Entry entry)
{
  if (entry.get_value().get_type() != Gnome::Conf::VALUE_BOOL) {
    // FIXME: use schema for this?
    Gnome::Conf::Value v(Gnome::Conf::VALUE_BOOL);
    v.set(false);
    entry.set_value(v);
  }

  bool use = entry.get_value().get_bool();

  Gnome::Conf::Value v
    = gconf_client->get(gconf_dir + "/background_color");

  unsigned int color;

  if (v.get_type() != Gnome::Conf::VALUE_INT)
    color = 0x00000000;
  else
    color = v.get_int();

  if (view.get()) {
    if (use)
      view->set_background(color);
    else
      view->unset_background();
  }
}

Glib::ustring Applet::find_empty_monitor_dir()
{
  Glib::SListHandle<Glib::ustring> list
    = gconf_client->all_dirs(gconf_dir + "/monitors");

  Glib::SListHandle<Glib::ustring>::const_iterator
    begin = list.begin(),
    end = list.end();

  int c = 1;
  Glib::ustring mon_dir;
  do {
    mon_dir = String::ucompose(gconf_dir + "/monitors/%1", c++);
  } while (std::find(begin, end, mon_dir) != end);

  return mon_dir;
}

void Applet::on_preferences_activated()
{
  preferences_window.reset(new PreferencesWindow(*this, monitors));
  preferences_window->show();
}

void Applet::on_help_activated()
{
  // FIXME: do something
}

void Applet::on_about_activated()
{
  std::vector<Glib::ustring> authors;
  authors.push_back("Ole Laursen <olau@hardworking.dk>");
  
  std::vector<Glib::ustring> documenters;
  // add documenters here

  Glib::ustring description =
    _("Monitor various hardware-related information, such as CPU usage, "
      "memory usage etc. Supports curve graphs, bar plots, "
      "column diagrams, textual monitoring and fluctuating flames.");
  
  if (about.get() == 0) {
    about.reset(new Gtk::AboutDialog());
    about->set_name(_("Hardware Monitor"));
    about->set_version(VERSION);
    // %1 is the copyright symbol
    about->set_copyright(String::ucompose(_("Copyright %1 2003 Ole Laursen"),
					  "\xc2\xa9"));
    about->set_authors(authors);
    if (!documenters.empty())
      about->set_documenters(documenters);
    about->set_comments(description);
    // note to translators: please fill in your names and email addresses
    about->set_translator_credits(_("translator-credits"));
    about->set_logo(icon);
    about->set_icon(icon);
    about->signal_response().connect(
    				sigc::hide(sigc::mem_fun(*about, &Gtk::Widget::hide)));
    about->show();
  }
  else {
    about->show();
    about->raise();
  }
}
