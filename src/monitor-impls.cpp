/* The various system statistics - adapters of the libgtop interface.
 *
 * Copyright (c) 2003, 04, 05 Ole Laursen.
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

#include <string>
#include <iostream>
#include <iomanip>
#include <ostream>
#include <sys/time.h>       // for high-precision timing for the network load
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cassert>
#include <cstdlib>

#include <glibtop.h>
#include <glibtop/cpu.h>
#include <glibtop/mem.h>
#include <glibtop/swap.h>
#include <glibtop/loadavg.h>
#include <glibtop/fsusage.h>
#include <glibtop/netload.h>


#include "monitor-impls.hpp"
#include "ucompose.hpp"
#include "i18n.hpp"


/* Decay factor for maximum values (log_0.999(0.9) = 105 iterations
 * before reduced 10%). This is now no longer used for CurveView - the
 * actual max value across the ValueHistories is used */
double const max_decay = 0.999;


//
// functions from monitor.hpp
//

std::list<Monitor *>
load_monitors(XfceRc* settings)
{
  std::list<Monitor *> monitors;

  // Checking if settings currently exist
  if (settings)
  {
    // They do - fetching list of monitors
    gchar** settings_monitors = xfce_rc_get_groups(settings);

    // They do - looping for all monitors
    for (int i = 0; settings_monitors[i] != NULL; ++i)
    {
      // Skipping default group
      if (g_strcmp0(settings_monitors[i], "[NULL]") == 0)
        continue;

      // Setting the correct group prior to loading settings
      xfce_rc_set_group(settings, settings_monitors[i]);

      // Obtaining monitor type
      Glib::ustring type = xfce_rc_read_entry(settings, "type", "");

      if (type == "cpu_usage")
      {
        // Obtaining cpu_no
        int cpu_no = xfce_rc_read_int_entry(settings, "cpu_no", -1);

        // Creating CPU usage monitor with provided number if valid
        if (cpu_no == -1)
          monitors.push_back(new CpuUsageMonitor);
        else
          monitors.push_back(new CpuUsageMonitor(cpu_no));
      }
      else if (type == "memory_usage")
        monitors.push_back(new MemoryUsageMonitor);
      else if (type == "swap_usage")
        monitors.push_back(new SwapUsageMonitor);
      else if (type == "load_average")
        monitors.push_back(new LoadAverageMonitor);
      else if (type == "disk_usage")
      {
        // Obtaining volume mount directory
        Glib::ustring mount_dir = xfce_rc_read_entry(settings,
          "mount_dir", "/");

        // Obtaining whether to show free space or not
        bool show_free = xfce_rc_read_bool_entry(settings, "show_free",
          false);

        // Creating disk usage monitor
        monitors.push_back(new DiskUsageMonitor(mount_dir, show_free));
      }
      else if (type == "network_load")
      {
        // Fetching interface type
        Glib::ustring inter = xfce_rc_read_entry(settings, "interface",
          "eth");

        // Fetching interface number
        int inter_no = xfce_rc_read_int_entry(settings, "interface_no",
          0);

        // Fetching interface 'direction' setting
        int inter_direction = xfce_rc_read_int_entry(settings,
          "interface_direction", NetworkLoadMonitor::all_data);

        // Converting direction setting into dedicated type
        NetworkLoadMonitor::Direction dir;

        if (inter_direction == NetworkLoadMonitor::incoming_data)
          dir = NetworkLoadMonitor::incoming_data;
        else if (inter_direction == NetworkLoadMonitor::outgoing_data)
          dir = NetworkLoadMonitor::outgoing_data;
        else
          dir = NetworkLoadMonitor::all_data;

        // Creating network load monitor
        monitors.push_back(new NetworkLoadMonitor(inter, inter_no, dir));
      }
      else if (type == "temperature")
      {
        // Fetching temperature number
        int temperature_no = xfce_rc_read_int_entry(settings,
          "temperature_no", 0);

        // Creating temperature monitor
        monitors.push_back(new TemperatureMonitor(temperature_no));
      }
      else if (type == "fan_speed")
      {
        // Fetching fan number
        int fan_no = xfce_rc_read_int_entry(settings, "fan_no", 0);

        // Creating fan monitor
        monitors.push_back(new FanSpeedMonitor(fan_no));
      }

      // Saving the monitor's settings root
      monitors.back()->set_settings_dir(settings_monitors[i]);
    }

    // Clearing up
    g_strfreev(settings_monitors);
  }

  // Always start with a CpuUsageMonitor - FIXME: use schema?
  if (monitors.empty())
    monitors.push_back(new CpuUsageMonitor);

  return monitors;
}


//
// helpers
//

// for setting precision
struct Precision
{
  int n;
};

template <typename T>
T &operator <<(T& os, const Precision &p)
{
#if __GNUC__ < 3
  os << std::setprecision(p.n) << std::setiosflags(std::ios::fixed);
#else
  os << std::setprecision(p.n) << std::setiosflags(std::ios_base::fixed);
#endif
  return os;
}

Precision precision(int n)
{
  Precision p;
  p.n = n;
  return p;
}

// for getting max no. of decimal digits
Precision decimal_digits(double val, int n)
{
  Precision p;

  if (val == 0)
    p.n = 1;
  else {
    while (val > 1 && n > 0) {
      val /= 10;
      --n;
    }

    p.n = n;
  }

  return p;
}


//
// class CpuUsageMonitor
//

int const CpuUsageMonitor::max_no_cpus = GLIBTOP_NCPU;

CpuUsageMonitor::CpuUsageMonitor()
  : cpu_no(all_cpus), total_time(0), nice_time(0), idle_time(0), iowait_time(0)
{}

CpuUsageMonitor::CpuUsageMonitor(int cpu)
  : cpu_no(cpu), total_time(0), nice_time(0), idle_time(0), iowait_time(0)
{
  if (cpu_no < 0 || cpu_no >= max_no_cpus)
    cpu_no = all_cpus;
}

double CpuUsageMonitor::do_measure()
{
  glibtop_cpu cpu;

  glibtop_get_cpu(&cpu);

  guint64 t, n, i, io;

  if (cpu_no == all_cpus) {
    t = cpu.total;
    n = cpu.nice;
    i = cpu.idle;
    io = cpu.iowait;
  }
  else {
    t = cpu.xcpu_total[cpu_no];
    n = cpu.xcpu_nice[cpu_no];
    i = cpu.xcpu_idle[cpu_no];
    io = cpu.xcpu_iowait[cpu_no];
  }

  // calculate ticks since last call
  guint64
    dtotal = t - total_time,
    dnice = n - nice_time,
    didle = i - idle_time,
    diowait = io - iowait_time;

  // and save the new values
  total_time = t;
  nice_time = n;
  idle_time = i;
  iowait_time = io;

  // don't count in dnice to avoid always showing 100% with SETI@home and
  // similar applications running
  double res = double(dtotal - dnice - didle - diowait) / dtotal;

  if (res > 0)
    return res;
  else
    return 0;
}

double CpuUsageMonitor::max()
{
  return 1;
}

bool CpuUsageMonitor::fixed_max()
{
  return true;
}

Glib::ustring CpuUsageMonitor::format_value(double val)
{
  return String::ucompose(_("%1%%"), precision(1), 100 * val);
}

Glib::ustring CpuUsageMonitor::get_name()
{
  if (cpu_no == all_cpus)
    return _("All processors");
  else
    return String::ucompose(_("Processor no. %1"), cpu_no + 1);
}

Glib::ustring CpuUsageMonitor::get_short_name()
{
  if (cpu_no == all_cpus)
    // must be short
    return _("CPU");
  else
    // note to translators: %1 is the cpu no, e.g. "CPU 1"
    return String::ucompose(_("CPU %1"), cpu_no + 1);
}

int CpuUsageMonitor::update_interval()
{
  return 1000;
}

void CpuUsageMonitor::save(XfceRc *settings)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings, dir.c_str());
  xfce_rc_write_entry(settings, "type", "cpu_usage");
  xfce_rc_write_int_entry(settings, "cpu_no", cpu_no);
}


//
// class SwapUsageMonitor
//

SwapUsageMonitor::SwapUsageMonitor()
  : max_value(0)
{
}

double SwapUsageMonitor::do_measure()
{
  glibtop_swap swp;

  glibtop_get_swap(&swp);

  max_value = swp.total;

  if (swp.total > 0)
    return swp.used;
  else
    return 0;
}

double SwapUsageMonitor::max()
{
  return max_value;
}

bool SwapUsageMonitor::fixed_max()
{
  return false;
}

Glib::ustring SwapUsageMonitor::format_value(double val)
{
  val /= 1000000;

  return String::ucompose(_("%1 Mb"), decimal_digits(val, 3), val);
}

Glib::ustring SwapUsageMonitor::get_name()
{
  return _("Disk-based memory");
}

Glib::ustring SwapUsageMonitor::get_short_name()
{
  // must be short
  return _("Swap");
}

int SwapUsageMonitor::update_interval()
{
  return 10 * 1000;
}

void SwapUsageMonitor::save(XfceRc *settings)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings, dir.c_str());
  xfce_rc_write_entry(settings, "type", "swap_usage");
}


//
// class LoadAverageMonitor
//

LoadAverageMonitor::LoadAverageMonitor()
  : max_value(1.0)
{
}

double LoadAverageMonitor::do_measure()
{
  glibtop_loadavg loadavg;

  glibtop_get_loadavg (&loadavg);

  double val = loadavg.loadavg[0];

  max_value *= max_decay; // reduce gradually

  if (max_value < 1)    // make sure we don't get below 1
    max_value = 1;

  if (val > max_value)
    max_value = val * 1.05;

  if (max_value > 0)
    return val;
  else
    return 0;
}

double LoadAverageMonitor::max()
{
  return max_value;
}

bool LoadAverageMonitor::fixed_max()
{
  return false;
}

Glib::ustring LoadAverageMonitor::format_value(double val)
{
  return String::ucompose("%1", precision(1), val);
}

Glib::ustring LoadAverageMonitor::get_name()
{
  return _("Load average");
}

Glib::ustring LoadAverageMonitor::get_short_name()
{
  // note to translators: short for "load average" - it has nothing to do with
  // loading data
  return _("Load");
}

int LoadAverageMonitor::update_interval()
{
  return 30 * 1000;
}

void LoadAverageMonitor::save(XfceRc *settings)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings, dir.c_str());
  xfce_rc_write_entry(settings, "type", "load_average");

  // No support for floats - stringifying
  Glib::ustring setting = String::ucompose("%1", max_value);
  xfce_rc_write_entry(settings, "max", setting.c_str());
}

void LoadAverageMonitor::load(XfceRc *settings)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Loading settings - no support for floats, unstringifying
  xfce_rc_set_group(settings, dir.c_str());
  Glib::ustring type = xfce_rc_read_entry(settings, "type", "");
  if (type == "load_average")
    max_value = atof(xfce_rc_read_entry(settings, "max", "5"));
}


//
// class MemoryUsageMonitor
//

MemoryUsageMonitor::MemoryUsageMonitor()
  : max_value(0)
{
}

double MemoryUsageMonitor::do_measure()
{
  glibtop_mem mem;

  glibtop_get_mem (&mem);

  max_value = mem.total;

  if (mem.total > 0)
    return mem.used - (mem.buffer + mem.cached);
  else
    return 0;
}

double MemoryUsageMonitor::max()
{
  return max_value;
}

bool MemoryUsageMonitor::fixed_max()
{
  return false;
}

Glib::ustring MemoryUsageMonitor::format_value(double val)
{
  val /= 1000000;

  return String::ucompose(_("%1 Mb"), decimal_digits(val, 3), val);
}

Glib::ustring MemoryUsageMonitor::get_name()
{
  return _("Memory");
}

Glib::ustring MemoryUsageMonitor::get_short_name()
{
  // short for memory
  return _("Mem.");
}

int MemoryUsageMonitor::update_interval()
{
  return 10 * 1000;
}

void MemoryUsageMonitor::save(XfceRc *settings)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings, dir.c_str());
  xfce_rc_write_entry(settings, "type", "memory_usage");
}


//
// class DiskUsageMonitor
//

DiskUsageMonitor::DiskUsageMonitor(const std::string &dir, bool free)
  : max_value(0), mount_dir(dir), show_free(free)
{
}

double DiskUsageMonitor::do_measure()
{
  glibtop_fsusage fsusage;

  glibtop_get_fsusage(&fsusage, mount_dir.c_str());

  max_value = fsusage.blocks * fsusage.block_size;

  double v = 0;

  if (show_free) {
    if (fsusage.bavail > 0)
      v = fsusage.bavail * fsusage.block_size;
  }
  else {
    if (fsusage.blocks > 0)
      v = (fsusage.blocks - fsusage.bfree) * fsusage.block_size;
  }

  return v;
}

double DiskUsageMonitor::max()
{
  return max_value;
}

bool DiskUsageMonitor::fixed_max()
{
  return false;
}

Glib::ustring DiskUsageMonitor::format_value(double val)
{
  if (val >= 1000 * 1000 * 1000) {
    val /= 1000 * 1000 * 1000;
    return String::ucompose(_("%1 GB"), decimal_digits(val, 3), val);
  }
  else if (val >= 1000 * 1000) {
    val /= 1000 * 1000;
    return String::ucompose(_("%1 MB"), decimal_digits(val, 3), val);
  }
  else if (val >= 1000) {
    val /= 1000;
    return String::ucompose(_("%1 kB"), decimal_digits(val, 3), val);
  }
  else
    return String::ucompose(_("%1 B"), decimal_digits(val, 3), val);
}

Glib::ustring DiskUsageMonitor::get_name()
{
  return String::ucompose(_("Disk (%1)"), mount_dir);
}


Glib::ustring DiskUsageMonitor::get_short_name()
{
  return String::ucompose("%1", mount_dir);
}

int DiskUsageMonitor::update_interval()
{
  return 60 * 1000;
}

void DiskUsageMonitor::save(XfceRc *settings)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings, dir.c_str());
  xfce_rc_write_entry(settings, "type", "disk_usage");
  xfce_rc_write_entry(settings, "mount_dir", mount_dir.c_str());
  xfce_rc_write_bool_entry(settings, "show_free", show_free);
}


//
// class NetworkLoadMonitor
//

NetworkLoadMonitor::NetworkLoadMonitor(const Glib::ustring &inter, int inter_no,
               Direction dir)
  : max_value(1), byte_count(0), time_stamp_secs(0), time_stamp_usecs(0),
    interface(inter), interface_no(inter_no), direction(dir)
{
}

double NetworkLoadMonitor::do_measure()
{
  glibtop_netload netload;

  glibtop_get_netload(&netload,
          String::ucompose("%1%2", interface, interface_no).c_str());
  guint64 val, measured_bytes;

  if (direction == all_data)
    measured_bytes = netload.bytes_total;
  else if (direction == incoming_data)
    measured_bytes = netload.bytes_in;
  else
    measured_bytes = netload.bytes_out;

  if (byte_count == 0) // No estimate initially
    val = 0;
  else if (measured_bytes < byte_count) // Interface was reset
    val = 0;
  else
    val = measured_bytes - byte_count;

  byte_count = measured_bytes;

  /* Note - max_value is no longer used to determine the graph max for
   * Curves - the actual maxima stored in the ValueHistories are used */
  if (val != 0)     // Reduce scale gradually
    max_value = guint64(max_value * max_decay);

  if (val > max_value)
    max_value = guint64(val * 1.05);

  for (nlm_seq::iterator i = sync_monitors.begin(), end = sync_monitors.end();
       i != end; ++i) {
    NetworkLoadMonitor &other = **i;
    if (other.max_value > max_value)
      max_value = other.max_value;
    else if (max_value > other.max_value)
      other.max_value = max_value;
  }

  // calculate difference in msecs
  struct timeval tv;
  if (gettimeofday(&tv, 0) == 0) {
    time_difference =
      (tv.tv_sec - time_stamp_secs) * 1000 +
      (tv.tv_usec - time_stamp_usecs) / 1000;
    time_stamp_secs = tv.tv_sec;
    time_stamp_usecs = tv.tv_usec;
  }

  // Debug code
  /*std::cout << "NetworkLoadMonitor::do_measure: val: " << val <<
    ", max_value: " << max_value << "\n";*/

  return val;
}

double NetworkLoadMonitor::max()
{
  return max_value;
}

bool NetworkLoadMonitor::fixed_max()
{
  return false;
}

Glib::ustring NetworkLoadMonitor::format_value(double val)
{
  // 1000 ms = 1 s
  val = val / time_difference * 1000;

  if (val <= 0)     // fix weird problem with negative values
    val = 0;

  if (val >= 1000 * 1000 * 1000) {
    val /= 1000 * 1000 * 1000;
    return String::ucompose(_("%1 GB/s"), decimal_digits(val, 3), val);
  }
  else if (val >= 1000 * 1000) {
    val /= 1000 * 1000;
    return String::ucompose(_("%1 MB/s"), decimal_digits(val, 3), val);
  }
  else if (val >= 1000) {
    val /= 1000;
    return String::ucompose(_("%1 kB/s"), decimal_digits(val, 3), val);
  }
  else
    return String::ucompose(_("%1 B/s"), decimal_digits(val, 3), val);
}

Glib::ustring NetworkLoadMonitor::get_name()
{
  Glib::ustring str;

  if (interface == "eth" && interface_no == 0)
    str = _("Ethernet (first)");
  else if (interface == "eth" && interface_no == 1)
    str = _("Ethernet (second)");
  else if (interface == "eth" && interface_no == 2)
    str = _("Ethernet (third)");
  else if (interface == "ppp" && interface_no == 0)
    str = _("Modem");
  else if (interface == "slip" && interface_no == 0)
    str = _("Serial link");
  else if (interface == "wlan" && interface_no == 0)
    str = _("Wireless");
  else
    // unknown, someone must have been fiddling with the config file
    str = String::ucompose("%1%2", interface, interface_no);

  if (direction == incoming_data)
    // %1 is the network connection, e.g. "Ethernet (first)", in signifies
    // that this is incoming data
    str = String::ucompose(_("%1, in"), str);
  else if (direction == outgoing_data)
    // %1 is the network connection, e.g. "Ethernet (first)", out signifies
    // that this is outgoing data
    str = String::ucompose(_("%1, out"), str);

  return str;
}

Glib::ustring NetworkLoadMonitor::get_short_name()
{
  Glib::ustring str;

  if (interface == "eth")
    // short for an ethernet card
    str = String::ucompose(_("Eth. %1"), interface_no + 1);
  else if (interface == "ppp" && interface_no == 0)
    // short for modem
    str = _("Mod.");
  else if (interface == "slip" && interface_no == 0)
    // short for serial link
    str = _("Ser.");
  else if (interface == "wlan" && interface_no == 0)
    // short for wireless
    str = _("W.less.");
  else
    // unknown, someone must have been fiddling with the config file
    str = String::ucompose("%1%2", interface, interface_no);

  if (direction == incoming_data)
    str = String::ucompose(_("%1, in"), str);
  else if (direction == outgoing_data)
    str = String::ucompose(_("%1, out"), str);

  return str;
}

int NetworkLoadMonitor::update_interval()
{
  return 1000;
}

void NetworkLoadMonitor::save(XfceRc *settings)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings, dir.c_str());
  xfce_rc_write_entry(settings, "type", "network_load");
  xfce_rc_write_entry(settings, "interface", interface.c_str());
  xfce_rc_write_int_entry(settings, "interface_no", interface_no);
  xfce_rc_write_int_entry(settings, "interface_direction",
    int(direction));
  xfce_rc_write_int_entry(settings, "max", int(max_value));
}

void NetworkLoadMonitor::load(XfceRc *settings)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  /* Loading settings - ensuring the settings are for the particular
   * network monitor?? */
  xfce_rc_set_group(settings, dir.c_str());
  Glib::ustring type = xfce_rc_read_entry(settings, "type", ""),
    setting_interface = xfce_rc_read_entry(settings, "interface", "");
  if (type == "network_load" && setting_interface == interface
    && xfce_rc_read_int_entry(settings, "interface_no", 0) == interface_no
    && xfce_rc_read_int_entry(settings, "interface_direction",
      int(incoming_data)) == int(direction))
  {
    max_value = xfce_rc_read_int_entry(settings, "max", 0);
  }
}

void NetworkLoadMonitor::possibly_add_sync_with(Monitor *other)
{
  if (NetworkLoadMonitor *o = dynamic_cast<NetworkLoadMonitor *>(other))
    if (interface == o->interface && interface_no == o->interface_no
  && direction != o->direction)
      sync_monitors.push_back(o);
}

void NetworkLoadMonitor::remove_sync_with(Monitor *other)
{
  nlm_seq::iterator i
    = std::find(sync_monitors.begin(), sync_monitors.end(), other);

  if (i != sync_monitors.end())
    sync_monitors.erase(i);
}


//
// implementation of sensors wrapper
//

Sensors::Sensors()
{
#if HAVE_LIBSENSORS
  if (sensors_init(0) != 0)
    return;

  int i = 0;
  const sensors_chip_name *c;

  while ((c = sensors_get_detected_chips(0, &i)))
    chips.push_back(*c);
#endif
}

Sensors::~Sensors()
{
#if HAVE_LIBSENSORS
  chips.clear();

  sensors_cleanup();
#endif
}

Sensors &Sensors::instance()
{
  static Sensors s;

  return s;
}

Sensors::FeatureInfoSequence Sensors::get_features(std::string base)
{
  FeatureInfoSequence vec;

#if HAVE_LIBSENSORS
  const sensors_feature *feature;

  for (unsigned int i = 0; i < chips.size(); ++i) {
    sensors_chip_name *chip = &chips[i];
    int i1 = 0;

    while ((feature = sensors_get_features(chip, &i1))) {
      std::string name = feature->name;
      if (name.find(base) != std::string::npos) {
  FeatureInfo info;
  info.chip_no = i;
  info.feature_no = feature->number;
  info.max = invalid_max;

  char *desc = sensors_get_label(chip, feature);
  if (desc) {
    info.description = desc;
    std::free(desc);
  }

  vec.push_back(info);

        // now see if we can find a max
        const sensors_subfeature *subfeature;
        int i2 = 0;

        while ((subfeature = sensors_get_all_subfeatures(chip, feature, &i2))) {
          std::string subname = subfeature->name;
          // check whether this is a max value for the last feature
          if (subname.find(name) != std::string::npos
              && subname.find("_over") != std::string::npos) {
            double max;
            if (sensors_get_value(chip, subfeature->number, &max) == 0)
              vec.back().max = max;
            else
              vec.back().max = invalid_max;
          }
        }
      }
    }
  }
#endif

  return vec;
}

Sensors::FeatureInfoSequence Sensors::get_temperature_features()
{
  return get_features("temp");
}

Sensors::FeatureInfoSequence Sensors::get_fan_features()
{
  return get_features("fan");
}

double Sensors::get_value(int chip_no, int feature_no)
{
#if HAVE_LIBSENSORS
  if (chip_no < 0 || chip_no >= int(chips.size()))
    return 0;

  double res;

  if (sensors_get_value(&chips[chip_no], feature_no, &res) == 0)
    return res;
  else
    return 0;
#else
  return 0;
#endif
}



//
// class TemperatureMonitor
//

double const Sensors::invalid_max = -1000000;

TemperatureMonitor::TemperatureMonitor(int no)
  : sensors_no(no)
{
  Sensors::FeatureInfo info
    = Sensors::instance().get_temperature_features()[sensors_no];

  chip_no = info.chip_no;
  feature_no = info.feature_no;
  description = info.description;
  if (info.max != Sensors::invalid_max)
    max_value = info.max;
  else
    max_value = 40;        // set a reasonable default (40 Celcius degrees)
}

double TemperatureMonitor::do_measure()
{
  double val = Sensors::instance().get_value(chip_no, feature_no);

  if (val > max_value)
    max_value = val;

  return val;
}

double TemperatureMonitor::max()
{
  return max_value;
}

bool TemperatureMonitor::fixed_max()
{
  return false;
}


Glib::ustring TemperatureMonitor::format_value(double val)
{
  // %2 contains the degree sign (the following 'C' stands for Celsius)
  return String::ucompose(_("%1%2C"), decimal_digits(val, 3), val, "\xc2\xb0");
}

Glib::ustring TemperatureMonitor::get_name()
{
  if (!description.empty())
    // %2 is a descriptive string from sensors.conf
    return String::ucompose(_("Temperature %1: \"%2\""),
          sensors_no + 1, description);
  else
    return String::ucompose(_("Temperature %1"), sensors_no + 1);
}

Glib::ustring TemperatureMonitor::get_short_name()
{
  // short for "temperature", %1 is sensor no.
  return String::ucompose(_("Temp. %1"), sensors_no + 1);
}

int TemperatureMonitor::update_interval()
{
  return 20 * 1000;
}

void TemperatureMonitor::save(XfceRc *settings)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings, dir.c_str());
  xfce_rc_write_entry(settings, "type", "temperature");
  xfce_rc_write_int_entry(settings, "temperature_no", sensors_no);

  // No support for floats - stringifying
  Glib::ustring setting = String::ucompose("%1", max_value);
  xfce_rc_write_entry(settings, "max", setting.c_str());
}

void TemperatureMonitor::load(XfceRc *settings)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  /* Loading settings, making sure the right sensor is loaded. No support
   * for floats, unstringifying */
  xfce_rc_set_group(settings, dir.c_str());
  Glib::ustring type = xfce_rc_read_entry(settings, "type", "");
  if (type == "temperature" && xfce_rc_read_int_entry(settings,
    "temperature_no", 0) == sensors_no)
    max_value = atof(xfce_rc_read_entry(settings, "max", "40"));
}



//
// class FanSpeedMonitor
//

FanSpeedMonitor::FanSpeedMonitor(int no)
  : sensors_no(no)
{
  Sensors::FeatureInfo info
    = Sensors::instance().get_fan_features()[sensors_no];

  chip_no = info.chip_no;
  feature_no = info.feature_no;
  description = info.description;
  if (info.max != Sensors::invalid_max)
    max_value = info.max;
  else
    max_value = 1;    // 1 rpm isn't realistic, but whatever
}

double FanSpeedMonitor::do_measure()
{
  double val = Sensors::instance().get_value(chip_no, feature_no);

  if (val > max_value)
    max_value = val;

  return val;
}

double FanSpeedMonitor::max()
{
  return max_value;
}

bool FanSpeedMonitor::fixed_max()
{
  return false;
}

Glib::ustring FanSpeedMonitor::format_value(double val)
{
  // rpm is rotations per minute
  return String::ucompose(_("%1 rpm"), val, val);
}

Glib::ustring FanSpeedMonitor::get_name()
{
  if (!description.empty())
    // %2 is a descriptive string from sensors.conf
    return String::ucompose(_("Fan %1 speed: \"%2\""),
          sensors_no + 1, description);
  else
    return String::ucompose(_("Fan %1 speed"), sensors_no + 1);
}

Glib::ustring FanSpeedMonitor::get_short_name()
{
  return String::ucompose(_("Fan %1"), sensors_no + 1);
}

int FanSpeedMonitor::update_interval()
{
  return 20 * 1000;
}

void FanSpeedMonitor::save(XfceRc *settings)
{
    // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  // Saving settings
  xfce_rc_set_group(settings, dir.c_str());
  xfce_rc_write_entry(settings, "type", "fan_speed");
  xfce_rc_write_int_entry(settings, "fan_no", sensors_no);

  // No support for floats - stringifying
  Glib::ustring setting = String::ucompose("%1", max_value);
  xfce_rc_write_entry(settings, "max", setting.c_str());
}

void FanSpeedMonitor::load(XfceRc *settings)
{
  // Fetching assigned settings group
  Glib::ustring dir = get_settings_dir();

  /* Loading settings, making sure the right fan is loaded. No support
   * for floats, unstringifying */
  xfce_rc_set_group(settings, dir.c_str());
  Glib::ustring type = xfce_rc_read_entry(settings, "type", "");
  if (type == "fan_speed" && xfce_rc_read_int_entry(settings, "fan_no",
    0) == sensors_no)
    max_value = atof(xfce_rc_read_entry(settings, "max", "1"));
}
