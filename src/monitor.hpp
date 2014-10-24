/* Interface base class for the monitors.
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

#ifndef MONITOR_HPP
#define MONITOR_HPP

#include <string>
#include <list>
#include <glibmm/ustring.h>

extern "C"
{
#include <libxfce4util/libxfce4util.h>
}

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

  // Update the measured value from device
  void measure()
  {
    measured_value = do_measure();
    if (measured_value < 0)  // Safety check
      measured_value = 0;
  }
  
  // Fetch the currently measured value
  double value()
  {
    return measured_value;
  }

  void set_settings_dir(const Glib::ustring &new_dir)
  {
    settings_dir = new_dir;
  }
  
  Glib::ustring get_settings_dir()
  {
    return settings_dir;
  }
  
  // The max value that the monitor may attain
  virtual double max() = 0;

  // Indicate whether the monitor's max is fixed or not
  virtual bool fixed_max() = 0;

  /* Convert float to string which represents an actual number with the
   * appropriate unit */
  virtual Glib::ustring format_value(double val) = 0;

  // Return a descriptive name
  virtual Glib::ustring get_name() = 0;

  // Return a short name
  virtual Glib::ustring get_short_name() = 0;

  // The interval between updates in milliseconds
  virtual int update_interval() = 0;

  // Save information about the monitor
  virtual void save(XfceRc *settings) = 0;

  // Load any internal monitor state
  virtual void load(XfceRc *settings)
  {
  }

  /* If other is watching the same thing as this monitor, it might be
   * a good idea to sync maxima with it */
  virtual void possibly_add_sync_with(Monitor *other)
  {
  }

  // Remove a synchronisation
  virtual void remove_sync_with(Monitor *other)
  {
  }
  
protected:
  double measured_value;
  
private:

  // Perform actual measurement, for derived classes
  virtual double do_measure() = 0;

  Glib::ustring settings_dir;
};


//
// Helpers implemented in monitor-impls.cpp
//

typedef std::list<Monitor *> monitor_seq;
typedef monitor_seq::iterator monitor_iter;

monitor_seq load_monitors(XfceRc* settings);

#endif
