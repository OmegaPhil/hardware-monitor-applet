/* The preferences window.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
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

#ifndef PREFERENCES_WINDOW_HPP
#define PREFERENCES_WINDOW_HPP

#include <memory>
#include <vector>

#include <libglademm/xml.h>
#include <sigc++/trackable.h>
#include <sigc++/connection.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/colorbutton.h>
#include <gtkmm/fontbutton.h>
#include <gtkmm/label.h>
#include <gtkmm/liststore.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/scale.h>
#include <gtkmm/treeview.h>
#include <gtkmm/window.h>

#include "monitor.hpp"


class Applet;

class PreferencesWindow: public sigc::trackable
{
public:
  PreferencesWindow(Applet &applet, monitor_seq monitors);
  ~PreferencesWindow();

  void show();
  
private:
  Glib::RefPtr<Gnome::Glade::Xml> ui;

  Gtk::Window *window;
  
  Gtk::SpinButton *update_interval_spinbutton;
  Gtk::RadioButton *panel_background_radiobutton;
  Gtk::RadioButton *background_color_radiobutton;
  Gtk::ColorButton *background_colorbutton;
  
  Gtk::RadioButton *curve_radiobutton;
  Gtk::RadioButton *bar_radiobutton;
  Gtk::RadioButton *vbar_radiobutton;
  Gtk::RadioButton *column_radiobutton;
  Gtk::RadioButton *text_radiobutton;
  Gtk::RadioButton *flame_radiobutton;

  Gtk::Widget *size_outer_vbox;
  Gtk::Scale *size_scale;
  Gtk::Widget *font_outer_vbox;
  Gtk::CheckButton *font_checkbutton;
  Gtk::FontButton *fontbutton;

  Gtk::Button *remove_button;
  Gtk::Button *change_button;
  Gtk::TreeView *monitor_treeview;
  Gtk::Widget *monitor_options;

  Gtk::Widget *monitor_curve_options;
  Gtk::ColorButton *line_colorbutton;
  Gtk::Widget *monitor_bar_options;
  Gtk::ColorButton *bar_colorbutton;
  Gtk::Widget *monitor_vbar_options;
  Gtk::ColorButton *vbar_colorbutton;
  Gtk::Widget *monitor_column_options;
  Gtk::ColorButton *column_colorbutton;
  Gtk::Widget *monitor_flame_options;
  Gtk::ColorButton *flame_colorbutton;
  
  class MonitorColumns: public Gtk::TreeModel::ColumnRecord
  {
  public:
    Gtk::TreeModelColumn<Glib::ustring> name;
    Gtk::TreeModelColumn<Monitor *> monitor;

    MonitorColumns() { add(name); add(monitor); }
  };
  
  Glib::RefPtr<Gtk::ListStore> monitor_store;
  typedef Gtk::ListStore::iterator store_iter;
  
  // Originally gconf callbacks
  void viewer_type_listener(const Glib::ustring viewer_type);
  void background_color_listener(unsigned int background_color);
  void use_background_color_listener(bool use_background_color);
  void size_listener(int viewer_size);
  void font_listener(const Glib::ustring viewer_font);
  void monitor_color_listener(unsigned int color);

  void stop_monitor_listeners();
  
  std::vector<unsigned int> monitor_listeners;

  // GUI
  void on_background_colorbutton_set();
  void on_background_color_radiobutton_toggled();
  
  void on_curve_radiobutton_toggled();
  void on_bar_radiobutton_toggled();
  void on_vbar_radiobutton_toggled();
  void on_column_radiobutton_toggled();
  void on_text_radiobutton_toggled();
  void on_flame_radiobutton_toggled();
  
  void on_size_scale_changed();
  sigc::connection size_scale_cb; 
  void on_font_checkbutton_toggled();
  void on_fontbutton_set();

  void on_add_button_clicked();
  void on_remove_button_clicked();
  void on_change_button_clicked();
  void on_selection_changed();

  void on_monitor_colorbutton_set(Gtk::ColorButton *colorbutton);

  void on_close_button_clicked();
  bool on_closed(GdkEventAny *);

  Monitor *run_choose_monitor_window(const Glib::ustring &str);
  void add_to_monitors_list(Monitor *monitor);
  // for converting between size_scale units and pixels
  int size_scale_to_pixels(int size);
  int pixels_to_size_scale(int pixels);
  void sync_conf_with_colorbutton(Glib::ustring settings_dir,
    Glib::ustring setting_name, Gtk::ColorButton *button);
  void connect_monitor_colorbutton(Gtk::ColorButton *colorbutton);

  void save_font_name(Glib::ustring font_name);
  
  Applet &applet;
};

#endif
