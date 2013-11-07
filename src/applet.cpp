/* Implementation of the Applet class.
 *
 * Copyright (c) 2003, 04, 05 Ole Laursen.
 * Copyright (c) 2013 OmegaPhil (OmegaPhil+hardware.monitor@gmail.com)
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

// autoconf-generated, above source directory
#include <config.h>

#include <algorithm>
#include <vector>
#include <iostream>

#include <gtkmm/main.h>
#include <cassert>
#include <unistd.h>  // for nice
#include <libgnomecanvasmm/init.h>

extern "C"
{
#include <libxfce4util/libxfce4util.h>
}

#include "ucompose.hpp"
#include "helpers.hpp"

#include "applet.hpp"

#include "column-view.hpp"
#include "curve-view.hpp"
#include "bar-view.hpp"
#include "text-view.hpp"
#include "flame-view.hpp"
#include "monitor.hpp"
#include "preferences-window.hpp"
#include "i18n.hpp"

// XFCE4 functions to create and destroy applet
extern "C" void applet_construct(XfcePanelPlugin* plugin)
{
  nice(5);  // Don't eat up too much CPU

  try {

    // Initialising GTK and GNOME canvas
    /* Testing not initialising GTK, as this isn't a standalone application
     * but a library? Otherwise seems to fail */
    //Gtk::Main main(NULL, NULL);
    Gnome::Canvas::init();

    // i18n
    xfce_textdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

    // Actually creating the applet/plugin
    Applet *applet = new Applet(plugin);
    applet->show();
  }
  catch(const Glib::Error &ex)
  {
    // From helpers
    fatal_error(ex.what());
  }
}

// Does not need C linkage as its called via a function pointer?
void applet_free(XfcePanelPlugin*, Applet* applet)
{
  // Called by 'free-data' signal
  delete applet;
  applet = NULL;
}

// Helpers for popping up the various things
void display_preferences(Applet *applet)
{
  applet->on_preferences_activated();
}

void display_help(Applet *applet)
{
  applet->on_help_activated();
}

void display_about(Applet *applet)
{
  applet->on_about_activated();
}

/* Function declared here as its a callback for a C signal, so cant be a
 * method */
void save_monitors(Applet *applet)
{
  // Getting at applet/plugin objects
  XfcePanelPlugin *panel_applet = applet->panel_applet;

  // Search for a writeable settings file, create one if it doesnt exist
  gchar* file = xfce_panel_plugin_save_location(panel_applet, true);

  if (file)
  {
    // Opening setting file
    XfceRc* settings = xfce_rc_simple_open(file, false);
    g_free(file);

    // Looping for all monitors and calling save on each
    for (monitor_iter i = applet->monitors.begin(),
         end = applet->monitors.end(); i != end; ++i)
      (*i)->save(settings);

    // Close settings file
    xfce_rc_close(settings);
  }
  else
  {
    // Unable to obtain writeable config file - informing user and exiting
    std::cerr << _("Unable to obtain writeable config file path in order to"
      " save monitors!\n");
  }
}

// Same for this function
// Not needed as the canvas resizes on the fly
/*
gboolean size_changed(XfcePanelPlugin* plugin, gint size, Applet* applet)
{
  // Debug code
  std::cout << "Size changed event detected: " << size << "\n";

  return true;
}
* */


Applet::Applet(XfcePanelPlugin *plugin)
  : panel_applet(plugin),

  // Setting defaults
  icon_path("/usr/share/pixmaps/hardware-monitor-applet.png"),
  viewer_type("curve"),
  viewer_font(""),
  viewer_size(96),  // Arbitrary default, see later in this function for notes
  background_color(0x00000000),  // black as the night
  use_background_color(false),
  next_color(0)
{
  // Search for settings file
  XfceRc* settings = NULL;
  gchar* file = xfce_panel_plugin_lookup_rc_file(panel_applet);
  
  if (file)
  {
    // One exists - loading readonly settings
    settings = xfce_rc_simple_open(file, true);
    g_free(file);

    // Ensuring default group is in focus
    xfce_rc_set_group(settings, "[NULL]");

    icon_path = xfce_rc_read_entry(settings, "icon-path", icon_path.c_str());
    viewer_type = xfce_rc_read_entry(settings, "viewer_type",
      viewer_type.c_str());
    viewer_size = xfce_rc_read_int_entry(settings, "viewer_size",
      viewer_size);
    viewer_font = xfce_rc_read_entry(settings, "viewer_font",
      viewer_font.c_str());
    background_color = xfce_rc_read_int_entry(settings, "background_color",
      background_color);
    use_background_color = xfce_rc_read_bool_entry(settings,
      "use_background_color", use_background_color);
    next_color = xfce_rc_read_int_entry(settings, "next_color",
      next_color);
  }
  
  // Loading icon
  try
  {
    icon = Gdk::Pixbuf::create_from_file(icon_path);
  }
  catch (...)
  {
    std::cerr <<
      String::ucompose(_("Hardware Monitor: cannot load the icon '%1'.\n"),
          icon_path);

    // It's a minor problem if we can't find the icon
    icon = Glib::RefPtr<Gdk::Pixbuf>();
  }

  // Configuring viewer type
  viewer_type_listener(viewer_type);

  /* Actually setting the viewer size has no effect in this function -
   * seems that it needs to be done in or after the mainloop kicks off */

  // Loading up monitors
  monitor_seq mon = load_monitors(settings);
  for (monitor_iter i = mon.begin(), end = mon.end(); i != end; ++i)
    add_monitor(*i);

  // All settings loaded
  if (settings)
    xfce_rc_close(settings);

  /* Connect plugin signals to functions - since I'm not really interested
   * in the plugin but the applet pointer, swapped results in the signal
   * handler getting the applet reference first - the plugin pointer is
   * passed next, but since the handler only takes one parameter this is
   * discarded */
  // Providing About option
  g_signal_connect_swapped(panel_applet, "about", G_CALLBACK(display_about),
    this);

  // Hooking into Properties option
  g_signal_connect_swapped(panel_applet, "configure-plugin",
    G_CALLBACK(display_preferences), this);

  // Hooking into plugin destruction signal
  g_signal_connect_swapped(panel_applet, "free-data", G_CALLBACK(applet_free),
    this);

  // Hooking into save signal
  g_signal_connect_swapped(panel_applet, "save", G_CALLBACK(save_monitors),
    this);

  /* Not needed as the canvas resizes on the fly
  // Hooking into size changed signal
  g_signal_connect(panel_applet, "size-changed", G_CALLBACK(size_changed),
    this);
  */

  // Adding configure and about to the applet's right-click menu
  xfce_panel_plugin_menu_show_configure(panel_applet);
  xfce_panel_plugin_menu_show_about(panel_applet);

  /* Add applet to panel - I need to turn the Applet (which inherits from
   * Gtk::EventBox) into a GtkWidget* - to do this I get at the GObject
   * pointer underneath the gtkmm layer */
  gtk_container_add(GTK_CONTAINER(plugin), GTK_WIDGET(this->gobj()));

  // Initialising timer to run every second (by default) to trigger main_loop
  timer =
    Glib::signal_timeout().connect(sigc::mem_fun(*this, &Applet::main_loop),
      update_interval);

  // Initial main_loop run
  main_loop();
}

Applet::~Applet()
{
  timer.disconnect();
  
  // Make sure noone is trying to read the monitors before we kill them
  if (view.get())
    for (monitor_iter i = monitors.begin(), end = monitors.end(); i != end; ++i)
      view->detach(*i);
  
  view.reset();

  // Save monitors configuration
  save_monitors(this);

  // Delete monitors
  for (monitor_iter i = monitors.begin(), end = monitors.end(); i != end; ++i) {
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

void Applet::viewer_type_listener(const Glib::ustring viewer_type)
{
  if (viewer_type == "curve")
  {
    // Setting view to CurveView if it isnt already
    if (!dynamic_cast<CurveView *>(view.get()))
      set_view(new CurveView);
  }
  else if (viewer_type == "bar")
  {
    // Setting view to horizontal BarView if it isnt already
    // It gets tricky here because them BarView can render 2 viewers.
    // Thus, we much also check the oriententation
    BarView *bar_view = dynamic_cast<BarView *>(view.get());
    if (!(bar_view && bar_view->is_horizontal()) )
      set_view(new BarView);
  }
  else if (viewer_type == "vbar")
  {
    // Setting view to vertical BarView if it isnt already
    // Same situation as with "bar"
    BarView *bar_view = dynamic_cast<BarView *>(view.get());
    if (!(bar_view && !bar_view->is_horizontal()) )
      set_view(new BarView(false));
  }
  else if (viewer_type == "text") {

    // Setting view to TextView if it isnt already
    if (!dynamic_cast<TextView *>(view.get()))
      set_view(new TextView);
  }
  else if (viewer_type == "flame") {

    // Setting view to FlameView if it isnt already
    if (!dynamic_cast<FlameView *>(view.get()))
      set_view(new FlameView);
  }
  else if (viewer_type == "column") {

    // Setting view to ColumnView if it isnt already
    if (!dynamic_cast<ColumnView *>(view.get()))
      set_view(new ColumnView);
  }

  // Make sure the view sets the background
  background_color_listener(background_color);

  // Update recorded viewer type
  this->viewer_type = viewer_type;
}

void Applet::background_color_listener(unsigned int background_color)
{
  if (use_background_color && view.get())
    view->set_background(background_color);

  // Update background_color
  this->background_color = background_color;
}

void Applet::use_background_color_listener(gboolean use_background_color)
{
  if (view.get())
  {
    if (use_background_color)
      view->set_background(background_color);
    else
      view->unset_background();
  }

  // Update use_background_color
  this->use_background_color = use_background_color;
}

bool Applet::main_loop()
{
  // Update view
  if (view.get())
    view->update();

  // Update tooltip
  Glib::ustring tip;
  for (monitor_iter i = monitors.begin(), end = monitors.end(); i != end; ++i) {
    Monitor &mon = **i;

    // Note to translators: %1 is the name of a monitor, e.g. "CPU 1", and %2 is
    // the current measurement, e.g. "78%"
    Glib::ustring next = String::ucompose(_("%1: %2"), mon.get_short_name(),
            mon.format_value(mon.value()));
    if (tip.empty())
      tip = next;
    else
      // Note to translators: this is used for composing a list of monitors; %1
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

  // Saving 'current' next color
  int color = next_color;
  
  // Updating next_color
  next_color = int((next_color + 1) %
    (sizeof(colors) / sizeof(unsigned int)));
  
  // Search for a writeable settings file, create one if it doesnt exist
  gchar* file = xfce_panel_plugin_save_location(panel_applet, true);
    
  if (file)
  {
    // Opening setting file
    XfceRc* settings = xfce_rc_simple_open(file, false);
    g_free(file);

    // Ensuring default group is in focus
    xfce_rc_set_group(settings, "[NULL]");

    // Saving next_color
    xfce_rc_write_int_entry(settings, "next_color", next_color);
    
    // Close settings file
    xfce_rc_close(settings);
  }
  else
  {
    // Unable to obtain writeable config file - informing user and exiting
    std::cerr << _("Unable to obtain writeable config file path in order to"
      " save next_color!\n");
  }

  // Returning actual next color
  return colors[color];
}

int Applet::get_size() const
{
  /* Returns the thickness of the panel (i.e. height in the normal
   * orientation or width in the vertical orientation) */
  return xfce_panel_plugin_get_size(panel_applet);
}

bool Applet::horizontal() const
{
  GtkOrientation orient = xfce_panel_plugin_get_orientation(panel_applet);
  return orient == GTK_ORIENTATION_HORIZONTAL;
}

Glib::RefPtr<Gdk::Pixbuf> Applet::get_icon()
{
  return icon;
}

const Glib::ustring Applet::get_viewer_type()
{
  return viewer_type;
}

int Applet::get_background_color() const
{
  return background_color;
}

gboolean Applet::get_use_background_color() const
{
  return use_background_color;
}

int Applet::get_viewer_size() const
{
  return viewer_size;
}

void Applet::set_viewer_size(const int size)
{
  // See header file viewer_size_configured notes

  // Obtaining current widget dimensions
  GtkRequisition req_size;
  gtk_widget_size_request(GTK_WIDGET(panel_applet), &req_size);

  /*
  // Debug code
  std::cout << "Size information: " << req_size.width << "x"
    << req_size.height << "\n";
  */

  // Make sure on every call that the viewer size is being honoured
  if (horizontal())
  {
    if (req_size.width != size)
      gtk_widget_set_size_request(GTK_WIDGET(panel_applet), size, -1);
  }
  else
  {
    if (req_size.height != size)
      gtk_widget_set_size_request(GTK_WIDGET(panel_applet), -1, size);
  }

  // Exiting if the size hasn't changed from this program's perspective
  if (viewer_size == size)
    return;

  viewer_size = size;

  // Debug code
  //std::cout << "Viewer size set to " << viewer_size << "\n";
}

const Glib::ustring Applet::get_viewer_font()
{
  return viewer_font;
}

void Applet::set_viewer_font(const Glib::ustring font_name)
{
  viewer_font = font_name;
}

void Applet::add_monitor(Monitor *monitor)
{
  add_sync_for(monitor);
  monitors.push_back(monitor);

  /* Read and write config locations and the open call are be different
   * in XFCE4 - hence the duplication here */

  /* Checking if monitor has a defined settings directory and therefore
   * settings to load */
  if (monitor->get_settings_dir().empty())
  {
    // It hasn't - creating one and saving
    monitor->set_settings_dir(find_empty_monitor_dir());

    // Search for a writeable settings file, create one if it doesnt exist
    gchar* file = xfce_panel_plugin_save_location(panel_applet, true);
      
    if (file)
    {
      // Opening setting file
      XfceRc* settings = xfce_rc_simple_open(file, false);
      g_free(file);

      // Saving monitor
      monitor->save(settings);

      // Close settings file
      xfce_rc_close(settings);
    }
    else
    {
      // Unable to obtain writeable config file - informing user
      std::cerr << _("Unable to obtain writeable config file path in "
        "order to save monitor in add_monitor call!\n");
    }
  }
  else
  {
    /* Monitor has a saved settings directory and therefore settings to
     * load
     * Search for settings file */
    gchar* file = xfce_panel_plugin_lookup_rc_file(panel_applet);
 
    if (file)
    {
      // One exists - loading readonly settings
      XfceRc* settings = xfce_rc_simple_open(file, true);
      g_free(file);

      // Load settings for monitor
      monitor->load(settings);
      
      // Close settings file
      xfce_rc_close(settings);
    }
    else
    {
      // Unable to obtain read only config file - informing user
      std::cerr << _("Unable to obtain read-only config file path in "
        "order to load monitor settings in add_monitor call!\n");
    }
  }

  // Attaching monitor to view
  if (view.get())
    view->attach(monitor);
}

void Applet::remove_monitor(Monitor *monitor)
{
  // Detatching monitor
  if (view.get())
    view->detach(monitor);

  // Search for a writeable settings file, create one if it doesnt exist
  gchar* file = xfce_panel_plugin_save_location(panel_applet, true);
    
  if (file)
  {
    // Opening setting file
    XfceRc* settings = xfce_rc_simple_open(file, false);
    g_free(file);

    // Removing settings group associated with the monitor if it exists
    if (xfce_rc_has_group(settings, monitor->get_settings_dir().c_str()))
      xfce_rc_delete_group(settings, monitor->get_settings_dir().c_str(),
        FALSE);

    // Close settings file
    xfce_rc_close(settings);
  }
  else
  {
    // Unable to obtain writeable config file - informing user
    std::cerr << _("Unable to obtain writeable config file path in "
      "order to remove a monitor!\n");
  }

  // Everyone has been notified, it's now safe to remove and delete
  // the monitor
  monitors.remove(monitor);
  remove_sync_for(monitor);
  
  delete monitor;
}

void Applet::replace_monitor(Monitor *prev_mon, Monitor *new_mon)
{
  // Locating monitor of interest
  monitor_iter i = std::find(monitors.begin(), monitors.end(), prev_mon);
  assert(i != monitors.end());

  // Basic configuration
  add_sync_for(new_mon);
  *i = new_mon;
  new_mon->set_settings_dir(prev_mon->get_settings_dir());

  /* Loading monitor with previous monitor's settings - XFCE4 needs
   * different code to read a config file as compared to writing to it
   * Search for settings file */
  gchar* file = xfce_panel_plugin_lookup_rc_file(panel_applet);

  if (file)
  {
    // One exists - loading readonly settings
    XfceRc* settings = xfce_rc_simple_open(file, true);
    g_free(file);

    // Load settings
    new_mon->load(settings);
    
    // Close settings file
    xfce_rc_close(settings);
  }
  else
  {
    // Unable to obtain read-only config file - informing user
    std::cerr << _("Unable to obtain read-only config file path in "
      "order to load monitor settings in replace_monitor call!\n");
  }

  // Search for a writeable settings file, create one if it doesnt exist
  file = xfce_panel_plugin_save_location(panel_applet, true);
    
  if (file)
  {
    // Opening setting file
    XfceRc* settings = xfce_rc_simple_open(file, false);
    g_free(file);

    // Saving settings
    new_mon->save(settings);
    
    // Close settings file
    xfce_rc_close(settings);
  }
  else
  {
    // Unable to obtain writeable config file - informing user
    std::cerr << _("Unable to obtain writeable config file path in "
      "order to save monitor settings in replace_monitor call!\n");
  }

  // Reattach monitor if its attached to the current view
  if (view.get()) {
    view->detach(prev_mon);
    view->attach(new_mon);
  }

  // Deleting previous monitor
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

Glib::ustring Applet::find_empty_monitor_dir()
{
  Glib::ustring mon_dir;
  int c = 1;

  // Search for read-only settings file
  gchar* file = xfce_panel_plugin_lookup_rc_file(panel_applet);

  if (file)
  {
    // One exists - loading readonly settings
    XfceRc* settings = xfce_rc_simple_open(file, true);
    g_free(file);

    do {
      mon_dir = String::ucompose("%1", c++);
    } while (xfce_rc_has_group(settings, mon_dir.c_str()));
    
    // Close settings file
    xfce_rc_close(settings);
  }
  else
  {
    /* No configuration file exists yet - setting mon_dir to 1 and
     * informing user */
    mon_dir = String::ucompose("%1", c);
  }  

  // Returning next free monitor directory (number)
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
  authors.push_back("OmegaPhil <OmegaPhil+hardware.monitor@gmail.com>");
  
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
    about->set_copyright(String::ucompose(_("Copyright %1 2003 Ole "
      "Laursen\nCopyright %1 2013 OmegaPhil"), "\xc2\xa9"));
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
