/* Implementation of the ChooseMonitorWindow class.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
 * Copyright (c) 2013 OmegaPhil (OmegaPhil@startmail.com)
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

#include <config.h>

#include "choose-monitor-window.hpp"
#include "gui-helpers.hpp"
#include "monitor-impls.hpp"
#include "i18n.hpp"
#include "ucompose.hpp"


ChooseMonitorWindow::ChooseMonitorWindow(Glib::RefPtr<Gdk::Pixbuf> icon,
                                         Gtk::Window &parent)
{
  ui = get_glade_xml("choose_monitor_window");

  ui->get_widget("choose_monitor_window", window);
  window->set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
  window->set_icon(icon);
  window->set_transient_for(parent);

  ui->get_widget("device_notebook", device_notebook);

  ui->get_widget("cpu_usage_radiobutton", cpu_usage_radiobutton);
  ui->get_widget("memory_usage_radiobutton", memory_usage_radiobutton);
  ui->get_widget("swap_usage_radiobutton", swap_usage_radiobutton);
  ui->get_widget("load_average_radiobutton", load_average_radiobutton);
  ui->get_widget("disk_usage_radiobutton", disk_usage_radiobutton);
  ui->get_widget("network_load_radiobutton", network_load_radiobutton);
  ui->get_widget("temperature_radiobutton", temperature_radiobutton);
  ui->get_widget("fan_speed_radiobutton", fan_speed_radiobutton);

  ui->get_widget("cpu_usage_options", cpu_usage_options);
  ui->get_widget("disk_usage_options", disk_usage_options);
  ui->get_widget("network_load_options", network_load_options);

  ui->get_widget("all_cpus_radiobutton", all_cpus_radiobutton);
  ui->get_widget("one_cpu_radiobutton", one_cpu_radiobutton);
  ui->get_widget("cpu_no_spinbutton", cpu_no_spinbutton);

  ui->get_widget("mount_dir_entry", mount_dir_entry);
  ui->get_widget("show_free_checkbutton", show_free_checkbutton);

  ui->get_widget("network_type_optionmenu", network_type_optionmenu);
  ui->get_widget("network_direction_optionmenu", network_direction_optionmenu);

  ui->get_widget("temperature_box", temperature_box);
  ui->get_widget("temperature_options", temperature_options);
  ui->get_widget("temperature_optionmenu", temperature_optionmenu);

  ui->get_widget("fan_speed_box", fan_speed_box);
  ui->get_widget("fan_speed_options", fan_speed_options);
  ui->get_widget("fan_speed_optionmenu", fan_speed_optionmenu);
  
  cpu_usage_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_cpu_usage_radiobutton_toggled));

  disk_usage_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_disk_usage_radiobutton_toggled));

  network_load_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_network_load_radiobutton_toggled));

  temperature_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_temperature_radiobutton_toggled));

  fan_speed_radiobutton->signal_toggled()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::
                        on_fan_speed_radiobutton_toggled));

  // note 1 off to avoid counting from zero in the interface
  cpu_no_spinbutton->set_range(1, CpuUsageMonitor::max_no_cpus);

#if !HAVE_LIBSENSORS            // no sensors support, no options for it
  device_notebook->get_nth_page(3)->hide();
#endif

  // setup temperature option menu
  Sensors::FeatureInfoSequence seq
    = Sensors::instance().get_temperature_features();
  if (!seq.empty()) {
    temperature_box->set_sensitive(true);

    Gtk::Menu *menu = manage(new Gtk::Menu());
    int counter = 1;
    for (Sensors::FeatureInfoSequence::iterator i = seq.begin(),
           end = seq.end(); i != end; ++i) {
      Glib::ustring s;
      if (!i->description.empty())
        // %2 is a descriptive string from sensors.conf
        s = String::ucompose(_("Sensor %1: \"%2\""), counter, i->description);
      else
        s = String::ucompose(_("Sensor %1"), counter);
      
      menu->append(*manage(new Gtk::MenuItem(s)));
      ++counter;
    }

    temperature_optionmenu->set_menu(*menu);
    menu->show_all();
  }

  // setup fan option menu
  seq = Sensors::instance().get_fan_features();
  if (!seq.empty()) {
    fan_speed_box->set_sensitive(true);

    Gtk::Menu *menu = manage(new Gtk::Menu());
    int counter = 1;
    for (Sensors::FeatureInfoSequence::iterator i = seq.begin(),
           end = seq.end(); i != end; ++i) {
      Glib::ustring s;
      if (!i->description.empty())
        // %2 is a descriptive string from sensors.conf
        s = String::ucompose(_("Fan %1: \"%2\""), counter, i->description);
      else
        s = String::ucompose(_("Fan %1"), counter);
      
      menu->append(*manage(new Gtk::MenuItem(s)));
      ++counter;
    }

    fan_speed_optionmenu->set_menu(*menu);
    menu->show_all();
  }
  
  // connect close operations
  Gtk::Button *help_button;
  ui->get_widget("help_button", help_button);

  help_button->signal_clicked()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::on_help_button_clicked));
  
  window->signal_delete_event()
    .connect(sigc::mem_fun(*this, &ChooseMonitorWindow::on_closed));
}

ChooseMonitorWindow::~ChooseMonitorWindow()
{
  window->hide();  
}


Monitor *ChooseMonitorWindow::run(XfcePanelPlugin* panel_applet,
  const Glib::ustring &mon_dir)
{
  // setup dialog
  if (!mon_dir.empty())
  {
    // Search for settings file
    gchar* file = xfce_panel_plugin_lookup_rc_file(panel_applet);
    
    if (file)
    {
      // Loading settings
      XfceRc* settings = xfce_rc_simple_open(file, true);
      g_free(file);
      xfce_rc_set_group(settings, mon_dir.c_str());
      Glib::ustring type = xfce_rc_read_entry(settings, "type", "");

      if (type == "memory_usage")
      {
        device_notebook->set_current_page(1);
        memory_usage_radiobutton->set_active();
      }
      else if (type == "load_average")
      {
        device_notebook->set_current_page(0);
        load_average_radiobutton->set_active();
      }
      else if (type == "disk_usage")
      {
        device_notebook->set_current_page(1);
        disk_usage_radiobutton->set_active();
      }
      else if (type == "swap_usage")
      {
        device_notebook->set_current_page(1);
        swap_usage_radiobutton->set_active();
      }
      else if (type == "network_load")
      {
        device_notebook->set_current_page(2);
        network_load_radiobutton->set_active();
      }
      else if (type == "temperature")
      {
        device_notebook->set_current_page(3);
        temperature_radiobutton->set_active();
      }
      else
      {
        device_notebook->set_current_page(0);
        // FIXME: use schema?
        cpu_usage_radiobutton->set_active();
      }
      
      // Fill in cpu info
      if (xfce_rc_has_entry(settings, "cpu_no"))
      {
        int no = xfce_rc_read_int_entry(settings, "cpu_no", -1);
        if (no >= 0 && no < CpuUsageMonitor::max_no_cpus) {
          one_cpu_radiobutton->set_active();
          cpu_no_spinbutton->set_value(no + 1);
        }
        else {
          all_cpus_radiobutton->set_active();
        }
      }

      // Fill in disk usage info
      if (xfce_rc_has_entry(settings, "mount_dir"))
      {
        Glib::ustring mount_dir = xfce_rc_read_entry(settings,
          "mount_dir", "");
        mount_dir_entry->set_text(mount_dir);
      }
      if (xfce_rc_has_entry(settings, "show_free"))
      {
        bool show_free  = xfce_rc_read_bool_entry(settings,
          "show_free", false);
        show_free_checkbutton->set_active(show_free);
      }

      // Fill in network load info
      if (xfce_rc_has_entry(settings, "interface"))
      {
        Glib::ustring interface = xfce_rc_read_entry(settings,
          "interface", "eth");

        int interface_no = xfce_rc_read_int_entry(settings,
          "interface_no", 0);

        if (interface == "eth" && interface_no == 0)
          network_type_optionmenu->set_history(0);
        else if (interface == "eth" && interface_no == 1)
          network_type_optionmenu->set_history(1);
        else if (interface == "eth" && interface_no == 2)
          network_type_optionmenu->set_history(2);
        else if (interface == "ppp")
          network_type_optionmenu->set_history(3);
        else if (interface == "slip")
          network_type_optionmenu->set_history(4);
        else if (interface == "wlan")
          network_type_optionmenu->set_history(5);
        else
          network_type_optionmenu->set_history(0);
      }

      if (xfce_rc_has_entry(settings, "interface_direction"))
      {
        int direction = xfce_rc_read_int_entry(settings,
          "interface_direction", NetworkLoadMonitor::all_data);

        if (direction == NetworkLoadMonitor::incoming_data)
          network_direction_optionmenu->set_history(1);
        else if (direction == NetworkLoadMonitor::outgoing_data)
          network_direction_optionmenu->set_history(2);
        else if (direction == NetworkLoadMonitor::all_data)
          network_direction_optionmenu->set_history(0);
      }

      int temperature_no = xfce_rc_read_int_entry(settings,
          "temperature_no", 0);

      temperature_optionmenu->set_history(temperature_no);
    }
    else
    {
      // No configuration file present - default setting?
      device_notebook->set_current_page(0);
      // FIXME: use schema?
      cpu_usage_radiobutton->set_active();
    }
  }

  if (cpu_usage_radiobutton->get_active())
    cpu_usage_radiobutton->toggled(); // send a signal

  
  // then ask the user
  int response;
  
  do {
    response = window->run();

    if (response == Gtk::RESPONSE_OK) {
      Monitor *mon = 0;

      if (cpu_usage_radiobutton->get_active())
        if (one_cpu_radiobutton->get_active())
          mon = new CpuUsageMonitor(int(cpu_no_spinbutton->get_value()) - 1);
        else
          mon = new CpuUsageMonitor;
      else if (memory_usage_radiobutton->get_active())
        mon = new MemoryUsageMonitor;
      else if (swap_usage_radiobutton->get_active())
        mon = new SwapUsageMonitor;
      else if (load_average_radiobutton->get_active())
        mon = new LoadAverageMonitor;
      else if (disk_usage_radiobutton->get_active()) {
        Glib::ustring mount_dir = mount_dir_entry->get_text();
        bool show_free = show_free_checkbutton->get_active();
        // FIXME: check that mount_dir is valid
        mon = new DiskUsageMonitor(mount_dir, show_free);
      }
      else if (network_load_radiobutton->get_active()) {
        Glib::ustring interface;
        int interface_no;

        switch (network_type_optionmenu->get_history()) {
        case 1:
          interface = "eth";
          interface_no = 1;
          break;

        case 2:
          interface = "eth";
          interface_no = 2;
          break;

        case 3:
          interface = "ppp";
          interface_no = 0;
          break;

        case 4:
          interface = "slip";
          interface_no = 0;
          break;

        case 5:
          interface = "wlan";
          interface_no = 0;
          break;

        default:
          interface = "eth";
          interface_no = 0;
          break;
        }

        NetworkLoadMonitor::Direction dir;
        switch (network_direction_optionmenu->get_history()) {
        case NetworkLoadMonitor::incoming_data:
          dir = NetworkLoadMonitor::incoming_data;
          break;

        case NetworkLoadMonitor::outgoing_data:
          dir = NetworkLoadMonitor::outgoing_data;
          break;

        default:
          dir = NetworkLoadMonitor::all_data;
          break;
        }

        mon = new NetworkLoadMonitor(interface, interface_no, dir);
      }
      else if (temperature_radiobutton->get_active())
        mon = new TemperatureMonitor(temperature_optionmenu->get_history());
      else if (fan_speed_radiobutton->get_active())
        mon = new FanSpeedMonitor(fan_speed_optionmenu->get_history());

      return mon;
    }
  }
  while (response == Gtk::RESPONSE_HELP);

  return 0;
}


// UI callbacks

void ChooseMonitorWindow::on_cpu_usage_radiobutton_toggled()
{
  cpu_usage_options->property_sensitive()
    = cpu_usage_radiobutton->get_active();
}

void ChooseMonitorWindow::on_disk_usage_radiobutton_toggled()
{
  disk_usage_options->property_sensitive()
    = disk_usage_radiobutton->get_active();
}

void ChooseMonitorWindow::on_network_load_radiobutton_toggled()
{
  network_load_options->property_sensitive()
    = network_load_radiobutton->get_active();
}

void ChooseMonitorWindow::on_temperature_radiobutton_toggled()
{
  temperature_options->property_sensitive()
    = temperature_radiobutton->get_active();
}

void ChooseMonitorWindow::on_fan_speed_radiobutton_toggled()
{
  fan_speed_options->property_sensitive()
    = fan_speed_radiobutton->get_active();
}

void ChooseMonitorWindow::on_help_button_clicked()
{
  // FIXME: do something
}

bool ChooseMonitorWindow::on_closed(GdkEventAny *)
{
  window->hide();
  return false;
}
