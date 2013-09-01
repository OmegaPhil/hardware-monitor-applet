/* Interface base class for the monitors.
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

#ifndef MONITOR_HPP
#define MONITOR_HPP

#include <string>
#include <list>
#include <glibmm/ustring.h>
#include <gconfmm/client.h>

#include "helpers.hpp"

class Monitor: noncopyable
{
public:
  Monitor()
    : measured_value(0)
  {
  }
  
  virtual ~Monitor()
  {}

  // update the measured value from device
  void measure()
  {
    measured_value = do_measure();
    if (measured_value < 0) // safety check
      measured_value = 0;
  }
  
  // fetch the currently measured value
  double value()
  {
    return measured_value;
  }

  void set_gconf_dir(const Glib::ustring &new_dir)
  {
    gconf_dir = new_dir;
  }
  
  Glib::ustring get_gconf_dir()
  {
    return gconf_dir;
  }
  
  // the max value that the monitor may attain
  virtual double max() = 0;

  // convert float to string which represents an actual number with the
  // appropriate unit
  virtual Glib::ustring format_value(double val) = 0;

  // return a descriptive name
  virtual Glib::ustring get_name() = 0;

  // return a short name
  virtual Glib::ustring get_short_name() = 0;

  // the interval between updates in milliseconds
  virtual int update_interval() = 0;

  // save information about the monitor
  virtual void save(const Glib::RefPtr<Gnome::Conf::Client> &client) = 0;

  // load any internal monitor state
  virtual void load(const Glib::RefPtr<Gnome::Conf::Client> &client)
  {
  }

  // if other is watching the same thing as this monitor, it might be
  // a good idea to sync maxima with it
  virtual void possibly_add_sync_with(Monitor *other)
  {
  }

  // remove a synchronisation
  virtual void remove_sync_with(Monitor *other)
  {
  }
  
protected:
  double measured_value;
  
private:
  // perform actual measurement, for derived classes
  virtual double do_measure() = 0;

  Glib::ustring gconf_dir;
};


//
// helpers implemented in monitor-impls.cpp
//

typedef std::list<Monitor *> monitor_seq;
typedef monitor_seq::iterator monitor_iter;

monitor_seq load_monitors(const Glib::RefPtr<Gnome::Conf::Client> &client,
			  const Glib::ustring &dir);

#endif
