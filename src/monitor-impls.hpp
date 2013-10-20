/* Implementation of the various system statistics.
 *
 * Copyright (c) 2003, 04, 05 Ole Laursen.
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

#ifndef MONITOR_IMPLS_HPP
#define MONITOR_IMPLS_HPP

#include <config.h>

#include <string>
#include <glib/gtypes.h>

#if HAVE_LIBSENSORS
#include <sensors/sensors.h>
#endif

#include "monitor.hpp"

//
// concrete Monitor classes
//

class CpuUsageMonitor: public Monitor
{
public:
  CpuUsageMonitor();		// monitor all cpus
  CpuUsageMonitor(int cpu_no);	// monitor only cpu no.

  virtual double max();
  virtual Glib::ustring format_value(double val);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual int update_interval();
  virtual void save(XfceRc *settings);

  static int const max_no_cpus;
  
private:
  virtual double do_measure();
  
  static int const all_cpus = -1;
  int cpu_no;
  
  // we need to save these values to compute the difference next time the
  // monitor is updated
  guint64 total_time, nice_time, idle_time, iowait_time;
};


class SwapUsageMonitor: public Monitor
{
public:
  SwapUsageMonitor();
  
  virtual double max();
  virtual Glib::ustring format_value(double val);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual int update_interval();
  virtual void save(XfceRc *settings);

private:
  virtual double do_measure();
  
  guint64 max_value;		// maximum available swap
};
	
	
class LoadAverageMonitor: public Monitor
{
public:
  LoadAverageMonitor();
		
  virtual double max();
  virtual Glib::ustring format_value(double val);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual int update_interval();
  virtual void save(XfceRc *settings);
  virtual void load(XfceRc *settings);
		
private:
  virtual double do_measure();
  
  double max_value;		// currently monitored max number of processes
};
	
	
class MemoryUsageMonitor: public Monitor
{
public:
  MemoryUsageMonitor();
  
  virtual double max();
  virtual Glib::ustring format_value(double val);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual int update_interval();
  virtual void save(XfceRc *settings);

private:
  virtual double do_measure();
  
  guint64 max_value;		// maximum available physical RAM
};

	
class DiskUsageMonitor: public Monitor
{
public:
  DiskUsageMonitor(const std::string &mount_dir, bool show_free);
		
  virtual double max();
  virtual Glib::ustring format_value(double val);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual int update_interval();
  virtual void save(XfceRc *settings);

private:
  virtual double do_measure();
  
  guint64 max_value;		// maximum available disk blocks

  std::string mount_dir;
  bool show_free;
};

class NetworkLoadMonitor: public Monitor
{
public:
  enum Direction {
    all_data, incoming_data, outgoing_data
  };
  
  NetworkLoadMonitor(const Glib::ustring &interface, int interface_no,
		     Direction direction);
		
  virtual double max();
  virtual Glib::ustring format_value(double val);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual int update_interval();
  virtual void save(XfceRc *settings);
  virtual void load(XfceRc *settings);
  virtual void possibly_add_sync_with(Monitor *other);
  virtual void remove_sync_with(Monitor *other);

private:
  virtual double do_measure();
  
  guint64 max_value;		// maximum measured capacity of line
  long int time_difference;	// no. of msecs. between the last two calls
		
  guint64 byte_count;		// number of bytes at last call
  long int time_stamp_secs, time_stamp_usecs; // time stamp for last call

  Glib::ustring interface;	// e.g. "eth"
  int interface_no;		// e.g. 0
  Direction direction;

  typedef std::list<NetworkLoadMonitor *> nlm_seq;
  nlm_seq sync_monitors;
};


class TemperatureMonitor: public Monitor
{
public:
  TemperatureMonitor(int no);	// no. in the temperature features
		
  virtual double max();
  virtual Glib::ustring format_value(double val);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual int update_interval();
  virtual void save(XfceRc *settings);
  virtual void load(XfceRc *settings);

private:
  virtual double do_measure();
  
  double max_value;
  int chip_no, feature_no, sensors_no;
  std::string description;
};


class FanSpeedMonitor: public Monitor
{
public:
  FanSpeedMonitor(int no);	// no. in the fan features
		
  virtual double max();
  virtual Glib::ustring format_value(double val);
  virtual Glib::ustring get_name();
  virtual Glib::ustring get_short_name();
  virtual int update_interval();
  virtual void save(XfceRc *settings);
  virtual void load(XfceRc *settings);

private:
  virtual double do_measure();
  
  double max_value;
  int chip_no, feature_no, sensors_no;
  std::string description;
};


// a singleton for initializing the sensors library
class Sensors: noncopyable
{
public:
  static Sensors &instance();

  static double const invalid_max;
  
  struct FeatureInfo
  {
    int chip_no, feature_no;
    std::string description;	// description from sensors.conf
    double max;
  };
  typedef std::vector<FeatureInfo> FeatureInfoSequence;
  FeatureInfoSequence get_temperature_features();
  FeatureInfoSequence get_fan_features();

  // return value for feature, or 0 if not available
  double get_value(int chip_no, int feature_no);

private:
  Sensors();
  ~Sensors();

  // get a list of available features that contains base (e.g. "temp")
  FeatureInfoSequence get_features(std::string base);

#if HAVE_LIBSENSORS
  std::vector<sensors_chip_name> chips;
#endif
};


#endif
