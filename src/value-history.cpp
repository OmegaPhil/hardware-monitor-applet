/* Implementation of value history class.
 *
 * Copyright (c) 2004 Ole Laursen.
 * Copyright (c) 2013 OmegaPhil (OmegaPhil00@startmail.com)
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

#include <algorithm>
#include <iostream>

#include "value-history.hpp"
#include "monitor.hpp"
#include "applet.hpp"


ValueHistory::ValueHistory(Monitor *mon)
  : monitor(mon)
{
  wait_iterations = monitor->update_interval() / Applet::update_interval;
  waits_remaining = 0;
}

double ValueHistory::get_max_value()
{
  /* This is used so that the maximum displayed point on the graph is
   * always known */
  return max_value;
}

void ValueHistory::update(unsigned int max_samples, bool &new_value)
{
  --waits_remaining;
    
  if (waits_remaining <= 0) {
    new_value = true;
    monitor->measure();

    // Fetching new measurement
    double measurement = monitor->value();

    // Dealing with new max measurements
    if (measurement > max_value)
    {
      max_value = measurement;
      max_count = 1;
    }
    else if (measurement == max_value)
      ++max_count;

    // Saving data and resetting waits
    values.push_front(measurement);
    waits_remaining = wait_iterations;
  }
  else
    new_value = false;
  
  /* Get rid of extra samples (there may be more than one if user changes
   * configuration */
  while (values.size() > max_samples)
  {
    // Removing last value - saving to allow one pop_back at the top
    double last_value = values.back();
    values.pop_back();

    // Detecting dropping max values
    if (last_value == max_value)
    {
      --max_count;

      /* Determining the new maximum value and count if all of the
       * previous maxes have been dropped */
      if (max_count < 1)
      {

        // Debug code
        /*std::cout << "ValueHistory::update: Dropping samples, dropping "
        "max detected: " << max_value << ", count: " << max_count << "\n";*/

        max_value = *std::max_element(values.begin(), values.end());
        max_count = std::count(values.begin(), values.end(), max_value);

        // Debug code
        /*std::cout << "New max: " << max_value << ", new count: " <<
          max_count << "\n";*/
      }
    }
  }
}
