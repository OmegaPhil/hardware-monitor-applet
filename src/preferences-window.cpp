/* Implementation of the PreferencesWindow class.
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

#include <sigc++/bind.h>

#include <gtkmm/button.h>
#include <gconfmm/client.h>

#include <cassert>

#include "preferences-window.hpp"
#include "choose-monitor-window.hpp"
#include "gui-helpers.hpp"
#include "applet.hpp"
#include "monitor.hpp"
#include "i18n.hpp"


void PreferencesWindow::connect_monitor_colorbutton(Gtk::ColorButton
						    *colorbutton)
{
  colorbutton->signal_color_set()
    .connect(sigc::bind(sigc::mem_fun(*this, &PreferencesWindow::on_monitor_colorbutton_set),
			colorbutton));
}

PreferencesWindow::PreferencesWindow(Applet &applet_, monitor_seq monitors)
  : applet(applet_)
{
  ui = get_glade_xml("preferences_window");

  ui->get_widget("preferences_window", window);
  window->set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
  window->set_icon(applet.get_icon());
  

  // connect the Viewer tab widgets
  ui->get_widget("curve_radiobutton", curve_radiobutton);
  curve_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &PreferencesWindow::
			on_curve_radiobutton_toggled));
  
  ui->get_widget("bar_radiobutton", bar_radiobutton);
  bar_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &PreferencesWindow::
			on_bar_radiobutton_toggled));
  
  ui->get_widget("vbar_radiobutton", vbar_radiobutton);
  vbar_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &PreferencesWindow::
			on_vbar_radiobutton_toggled));
  
  ui->get_widget("column_radiobutton", column_radiobutton);
  column_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &PreferencesWindow::
			on_column_radiobutton_toggled));
  
  ui->get_widget("text_radiobutton", text_radiobutton);
  text_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &PreferencesWindow::
			on_text_radiobutton_toggled));

  ui->get_widget("flame_radiobutton", flame_radiobutton);
  flame_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &PreferencesWindow::
			on_flame_radiobutton_toggled));
  
  ui->get_widget("size_outer_vbox", size_outer_vbox);

  ui->get_widget("size_scale", size_scale);
  size_scale_cb =  size_scale->signal_value_changed()
    .connect(sigc::mem_fun(*this, &PreferencesWindow::on_size_scale_changed));

  ui->get_widget("font_outer_vbox", font_outer_vbox);

  ui->get_widget("font_checkbutton", font_checkbutton);
  font_checkbutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &PreferencesWindow::
			on_font_checkbutton_toggled));

  ui->get_widget("fontbutton", fontbutton);
  fontbutton->signal_font_set()
    .connect(sigc::mem_fun(*this, &PreferencesWindow::on_fontbutton_set));


  ui->get_widget("background_colorbutton", background_colorbutton);
  background_colorbutton->signal_color_set()
    .connect(sigc::mem_fun(*this, &PreferencesWindow::
			on_background_colorbutton_set));

  ui->get_widget("panel_background_radiobutton", panel_background_radiobutton);
  ui->get_widget("background_color_radiobutton", background_color_radiobutton);
  background_color_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &PreferencesWindow::
			on_background_color_radiobutton_toggled));
  
  
  // connect the Monitor tab widgets
  Gtk::Button *add_button;
  ui->get_widget("add_button", add_button);
  add_button->signal_clicked()
    .connect(sigc::mem_fun(*this, &PreferencesWindow::on_add_button_clicked));

  ui->get_widget("remove_button", remove_button);
  remove_button->signal_clicked()
    .connect(sigc::mem_fun(*this, &PreferencesWindow::on_remove_button_clicked));

  ui->get_widget("change_button", change_button);
  change_button->signal_clicked()
    .connect(sigc::mem_fun(*this, &PreferencesWindow::on_change_button_clicked));

  ui->get_widget("monitor_treeview", monitor_treeview);
  monitor_treeview->get_selection()->signal_changed()
    .connect(sigc::mem_fun(*this, &PreferencesWindow::on_selection_changed));

  ui->get_widget("monitor_options", monitor_options);

  
  static MonitorColumns mc;
  monitor_store = Gtk::ListStore::create(mc);
  monitor_treeview->set_model(monitor_store);
  monitor_treeview->append_column(_("Device"), mc.name);
  
  ui->get_widget("monitor_curve_options", monitor_curve_options);
  ui->get_widget("line_colorbutton", line_colorbutton);
  connect_monitor_colorbutton(line_colorbutton);
  
  ui->get_widget("monitor_bar_options", monitor_bar_options);
  ui->get_widget("bar_colorbutton", bar_colorbutton);
  connect_monitor_colorbutton(bar_colorbutton);
  
  ui->get_widget("monitor_vbar_options", monitor_vbar_options);
  ui->get_widget("vbar_colorbutton", vbar_colorbutton);
  connect_monitor_colorbutton(vbar_colorbutton);
  
  ui->get_widget("monitor_column_options", monitor_column_options);
  ui->get_widget("column_colorbutton", column_colorbutton);
  connect_monitor_colorbutton(column_colorbutton);

  ui->get_widget("monitor_flame_options", monitor_flame_options);
  ui->get_widget("flame_colorbutton", flame_colorbutton);
  connect_monitor_colorbutton(flame_colorbutton);
  
  
  // connect GConf
  Glib::RefPtr<Gnome::Conf::Client> &client = applet.get_gconf_client();
  Glib::ustring dir = applet.get_gconf_dir();
  
  client->notify_add(dir + "/viewer_type",
		     sigc::mem_fun(*this, &PreferencesWindow::
				viewer_type_listener));

  client->notify_add(dir + "/background_interval",
		     sigc::mem_fun(*this, &PreferencesWindow::
				background_color_listener));

  client->notify_add(dir + "/viewer/size",
		     sigc::mem_fun(*this, &PreferencesWindow::size_listener));

  client->notify_add(dir + "/viewer/font",
		     sigc::mem_fun(*this, &PreferencesWindow::font_listener));

  
  // fill in values
  viewer_type_listener(0, client->get_entry(dir + "/viewer_type"));
  background_color_listener(0, client->get_entry(dir + "/background_color"));
  use_background_color_listener(0, client
				->get_entry(dir + "/use_background_color"));
  size_listener(0, client->get_entry(dir + "/viewer/size"));
  font_listener(0, client->get_entry(dir + "/viewer/font"));

  for (monitor_iter i = monitors.begin(), end = monitors.end(); i != end; ++i)
    add_to_monitors_list(*i);
  
  // deselect all to allow the user to discover the relationship
  // between the greyed-out buttons and the treeview
  monitor_treeview->get_selection()->unselect_all();

  // make sure background colorbutton is grayed out
  background_color_radiobutton->toggled();
  
  // connect close operations
  Gtk::Button *close_button;
  ui->get_widget("close_button", close_button);

  close_button->signal_clicked()
    .connect(sigc::mem_fun(*this, &PreferencesWindow::on_close_button_clicked));

  window->signal_delete_event()
    .connect(sigc::mem_fun(*this, &PreferencesWindow::on_closed));
}

PreferencesWindow::~PreferencesWindow()
{
  window->hide();
  stop_monitor_listeners();
}

void PreferencesWindow::show()
{
  window->show();
  window->raise();
}


namespace 
{
  void update_colorbutton_if_different(Gtk::ColorButton *colorbutton,
				       unsigned char r,
				       unsigned char g,
				       unsigned char b,
				       unsigned char a)
  {
    unsigned char pa, pr, pg, pb;
    
    pa = colorbutton->get_alpha() >> 8;
  
    Gdk::Color c = colorbutton->get_color();
    pr = c.get_red() >> 8;
    pg = c.get_green() >> 8;
    pb = c.get_blue() >> 8;
    
    if (pr != r || pg != g || pb != b) {
      Gdk::Color new_c;
      new_c.set_rgb(gushort(r) << 8, gushort(g) << 8, gushort(b) << 8);
      colorbutton->set_color(new_c);
    }
    else if (pa != a)
      colorbutton->set_alpha(gushort(a) << 8);
  }
}


// GConf callbacks

void PreferencesWindow::viewer_type_listener(unsigned int,
					     Gnome::Conf::Entry entry)
{
  if (entry.get_value().get_type() != Gnome::Conf::VALUE_STRING)
    return;

  Glib::ustring s = entry.get_value().get_string();
  
  if (s == "curve") {
    if (!curve_radiobutton->get_active())
      curve_radiobutton->property_active() = true;
    size_outer_vbox->property_visible() = true;
    monitor_curve_options->property_visible() = true;
  }
  else if (s == "bar") {
    if (!bar_radiobutton->get_active())
      bar_radiobutton->property_active() = true;
    size_outer_vbox->property_visible() = true;
    monitor_bar_options->property_visible() = true;
  }
  else if (s == "vbar") {
    if (!vbar_radiobutton->get_active())
      vbar_radiobutton->property_active() = true;
    size_outer_vbox->property_visible() = true;
    monitor_vbar_options->property_visible() = true;
  }
  else if (s == "column") {
    if (!column_radiobutton->get_active())
      column_radiobutton->property_active() = true;
    size_outer_vbox->property_visible() = true;
    monitor_column_options->property_visible() = true;
  }
  else if (s == "text") {
    if (!text_radiobutton->get_active())
      text_radiobutton->property_active() = true;
    font_outer_vbox->property_visible() = true;
  }
  else if (s == "flame") {
    if (!flame_radiobutton->get_active())
      flame_radiobutton->property_active() = true;
    size_outer_vbox->property_visible() = true;
    monitor_flame_options->property_visible() = true;
  }
}

void PreferencesWindow::background_color_listener(unsigned int,
						  Gnome::Conf::Entry entry)
{
  if (entry.get_value().get_type() != Gnome::Conf::VALUE_INT)
    return;

  unsigned int i = entry.get_value().get_int();
  
  unsigned char r = i >> 24, g = i >> 16, b = i >> 8, a = i;

  update_colorbutton_if_different(background_colorbutton, r, g, b, a);
}

void PreferencesWindow::use_background_color_listener(unsigned int,
						      Gnome::Conf::Entry entry)
{
  if (entry.get_value().get_type() != Gnome::Conf::VALUE_BOOL)
    return;

  bool b = entry.get_value().get_bool();

  if (b)
    background_color_radiobutton->set_active();
  else
    panel_background_radiobutton->set_active();
}

void PreferencesWindow::size_listener(unsigned int,
				      Gnome::Conf::Entry entry)
{
  if (entry.get_value().get_type() != Gnome::Conf::VALUE_INT)
    return;

  int i = entry.get_value().get_int();
  
  if (size_scale_to_pixels(int(size_scale->get_value())) != i)
    size_scale->set_value(pixels_to_size_scale(i));
}

void PreferencesWindow::font_listener(unsigned int, Gnome::Conf::Entry entry)
{
  if (entry.get_value().get_type() != Gnome::Conf::VALUE_STRING)
    return;

  Glib::ustring font = entry.get_value().get_string();

  if (font.empty())
    font_checkbutton->set_active(false);
  else {
    font_checkbutton->set_active(true);
    if (fontbutton->get_font_name() != font)
      fontbutton->set_font_name(font);
  }
}

void PreferencesWindow::monitor_color_listener(unsigned int,
					       Gnome::Conf::Entry entry)
{
  if (entry.get_value().get_type() != Gnome::Conf::VALUE_INT)
    return;

  unsigned int i = entry.get_value().get_int();

  unsigned char r = i >> 24, g = i >> 16, b = i >> 8, a = i;

  update_colorbutton_if_different(line_colorbutton, r, g, b, a);
  update_colorbutton_if_different(bar_colorbutton,  r, g, b, a);
  update_colorbutton_if_different(vbar_colorbutton,  r, g, b, a);
  update_colorbutton_if_different(column_colorbutton, r, g, b, a);
  update_colorbutton_if_different(flame_colorbutton,  r, g, b, a);
}


// UI callbacks

namespace 
{
  // helper for avoiding clipping when shifting values
  unsigned int pack_int(unsigned int r, unsigned int g, unsigned int b,
		      unsigned int a)
  {
    return ((r & 255) << 24) | ((g & 255) << 16) | ((b & 255) << 8) | (a & 255);
  }

}

void PreferencesWindow::sync_conf_with_colorbutton(std::string path,
						   Gtk::ColorButton *button)
{
  // extract info from button
  unsigned char a, r, g, b;
    
  a = button->get_alpha() >> 8;
  
  Gdk::Color c = button->get_color();
  r = c.get_red() >> 8;
  g = c.get_green() >> 8;
  b = c.get_blue() >> 8;
  
  // update configuration
  Glib::RefPtr<Gnome::Conf::Client> &client = applet.get_gconf_client();

  client->set(path, int(pack_int(r, g, b, a)));
}


void PreferencesWindow::on_background_colorbutton_set()
{
  sync_conf_with_colorbutton(applet.get_gconf_dir() + "/background_color",
			     background_colorbutton);
}

void PreferencesWindow::on_background_color_radiobutton_toggled()
{
  Glib::RefPtr<Gnome::Conf::Client> &client = applet.get_gconf_client();
  Glib::ustring dir = applet.get_gconf_dir();

  bool on = background_color_radiobutton->get_active();
  
  background_colorbutton->set_sensitive(on);
  
  client->set(dir + "/use_background_color", on);
}

void PreferencesWindow::on_curve_radiobutton_toggled()
{
  Glib::RefPtr<Gnome::Conf::Client> &client = applet.get_gconf_client();
  Glib::ustring dir = applet.get_gconf_dir();

  bool active = curve_radiobutton->get_active();
  
  if (active)
    client->set(dir + "/viewer_type", Glib::ustring("curve"));

  size_outer_vbox->property_visible() = active;
  monitor_curve_options->property_visible() = active;
}

void PreferencesWindow::on_bar_radiobutton_toggled()
{
  Glib::RefPtr<Gnome::Conf::Client> &client = applet.get_gconf_client();
  Glib::ustring dir = applet.get_gconf_dir();

  bool active = bar_radiobutton->get_active();
  
  if (active)
    client->set(dir + "/viewer_type", Glib::ustring("bar"));

  size_outer_vbox->property_visible() = active;
  monitor_bar_options->property_visible() = active;
}

void PreferencesWindow::on_vbar_radiobutton_toggled()
{
  Glib::RefPtr<Gnome::Conf::Client> &client = applet.get_gconf_client();
  Glib::ustring dir = applet.get_gconf_dir();

  bool active = vbar_radiobutton->get_active();
  
  if (active)
    client->set(dir + "/viewer_type", Glib::ustring("vbar"));

  size_outer_vbox->property_visible() = active;
  monitor_vbar_options->property_visible() = active;
}

void PreferencesWindow::on_column_radiobutton_toggled()
{
  Glib::RefPtr<Gnome::Conf::Client> &client = applet.get_gconf_client();
  Glib::ustring dir = applet.get_gconf_dir();

  bool active = column_radiobutton->get_active();
  
  if (active)
    client->set(dir + "/viewer_type", Glib::ustring("column"));

  size_outer_vbox->property_visible() = active;
  monitor_column_options->property_visible() = active;
}

void PreferencesWindow::on_text_radiobutton_toggled()
{
  Glib::RefPtr<Gnome::Conf::Client> &client = applet.get_gconf_client();
  Glib::ustring dir = applet.get_gconf_dir();

  bool active = text_radiobutton->get_active();
  
  if (active)
    client->set(dir + "/viewer_type", Glib::ustring("text"));

  font_outer_vbox->property_visible() = active;
}

void PreferencesWindow::on_flame_radiobutton_toggled()
{
  Glib::RefPtr<Gnome::Conf::Client> &client = applet.get_gconf_client();
  Glib::ustring dir = applet.get_gconf_dir();

  bool active = flame_radiobutton->get_active();
  
  if (active)
    client->set(dir + "/viewer_type", Glib::ustring("flame"));

  size_outer_vbox->property_visible() = active;
  monitor_flame_options->property_visible() = active;
}

void PreferencesWindow::on_size_scale_changed()
{
  Glib::RefPtr<Gnome::Conf::Client> &client = applet.get_gconf_client();
  Glib::ustring dir = applet.get_gconf_dir();

  size_scale_cb.block();
  int i = int(size_scale->get_value() + 0.5);
  size_scale->set_value(i);
  client->set(dir + "/viewer/size", size_scale_to_pixels(i));
  size_scale_cb.unblock();
}

void PreferencesWindow::on_font_checkbutton_toggled()
{
  Glib::RefPtr<Gnome::Conf::Client> &client = applet.get_gconf_client();
  Glib::ustring dir = applet.get_gconf_dir();

  bool active = font_checkbutton->get_active();
  
  fontbutton->set_sensitive(active);

  if (active)
    client->set(dir + "/viewer/font", fontbutton->get_font_name());
  else
    client->set(dir + "/viewer/font", Glib::ustring(""));
}

void PreferencesWindow::on_fontbutton_set()
{
  Glib::RefPtr<Gnome::Conf::Client> &client = applet.get_gconf_client();
  Glib::ustring dir = applet.get_gconf_dir();

  client->set(dir + "/viewer/font", fontbutton->get_font_name());
}

void PreferencesWindow::on_add_button_clicked()
{
  Monitor *monitor = run_choose_monitor_window(Glib::ustring());

  if (monitor) {
    applet.add_monitor(monitor);
    add_to_monitors_list(monitor);
  }
}

void PreferencesWindow::on_remove_button_clicked()
{
  static MonitorColumns mc;
  
  store_iter i = monitor_treeview->get_selection()->get_selected();
  
  if (i) {
    Monitor *mon = (*i)[mc.monitor];
    monitor_store->erase(i);
    applet.remove_monitor(mon);
  }
}

void PreferencesWindow::on_change_button_clicked()
{
  static MonitorColumns mc;
  
  store_iter i = monitor_treeview->get_selection()->get_selected();
  
  if (i) {
    Monitor *prev_monitor = (*i)[mc.monitor];
    Monitor *new_monitor
      = run_choose_monitor_window(prev_monitor->get_gconf_dir());

    if (new_monitor) {
      applet.replace_monitor(prev_monitor, new_monitor);

      (*i)[mc.name] = new_monitor->get_name();
      (*i)[mc.monitor] = new_monitor;
    }
  }
}

void PreferencesWindow::stop_monitor_listeners()
{
  Glib::RefPtr<Gnome::Conf::Client> &client = applet.get_gconf_client();
  
  for (std::vector<unsigned int>::iterator i = monitor_listeners.begin(),
	 end = monitor_listeners.end(); i != end; ++i)
    client->notify_remove(*i);

  monitor_listeners.clear();
}

void PreferencesWindow::on_selection_changed()
{
  Glib::RefPtr<Gnome::Conf::Client> &client = applet.get_gconf_client();

  static MonitorColumns mc;
  store_iter i = monitor_treeview->get_selection()->get_selected();

  bool sel = i;

  stop_monitor_listeners();

  if (sel) {
    Glib::ustring key, mon_dir = (*(*i)[mc.monitor]).get_gconf_dir();
    unsigned int con;

    key = mon_dir + "/color";
    con = client->notify_add(key, sigc::mem_fun(*this, &PreferencesWindow::
					     monitor_color_listener));

    monitor_color_listener(0, client->get_entry(key));

    monitor_listeners.push_back(con);
  }
  
  remove_button->set_sensitive(sel);
  change_button->set_sensitive(sel);
  monitor_options->set_sensitive(sel);
}

void PreferencesWindow::on_monitor_colorbutton_set(Gtk::ColorButton *colorbutton)
{
  static MonitorColumns mc;
  
  store_iter i = monitor_treeview->get_selection()->get_selected();
  
  if (i) {
    Glib::ustring mon_dir = (*(*i)[mc.monitor]).get_gconf_dir();

    sync_conf_with_colorbutton(mon_dir + "/color", colorbutton);
  }
}

void PreferencesWindow::on_close_button_clicked()
{
  window->hide();
}

bool PreferencesWindow::on_closed(GdkEventAny *)
{
  window->hide();
  return false;
}

Monitor *PreferencesWindow::run_choose_monitor_window(const Glib::ustring &str)
{
  ChooseMonitorWindow chooser(applet.get_icon(), *window);
  
  return chooser.run(applet.get_gconf_client(), str);
}

void PreferencesWindow::add_to_monitors_list(Monitor *mon)
{
  MonitorColumns mc;
  
  store_iter i = monitor_store->append();
  (*i)[mc.name] = mon->get_name();
  (*i)[mc.monitor] = mon;
      
  monitor_treeview->get_selection()->select(i);
}


// for converting between size_scale units and pixels
int const pixel_size_table_size = 10;
int pixel_size_table[pixel_size_table_size]
  = { 32, 48, 64, 96, 128, 192, 256, 384, 512, 1024 };


int PreferencesWindow::size_scale_to_pixels(int size)
{
  assert(size >= 0 && size < pixel_size_table_size);

  return pixel_size_table[size];
}

int PreferencesWindow::pixels_to_size_scale(int pixels)
{
  // we may not have an exact match, so just find the nearest
  int min_diff = 1000000, min_i = 0;
  for (int i = 0; i < pixel_size_table_size; ++i) {
    int diff = std::abs(pixel_size_table[i] - pixels);
    if (diff < min_diff) {
      min_diff = diff;
      min_i = i;
    }
  }

  return min_i;
}
