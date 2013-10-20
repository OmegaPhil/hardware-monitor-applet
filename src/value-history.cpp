/* Implementation of value history class.
 *
 * Copyright (c) 2004 Ole Laursen.
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

#include "value-history.hpp"
#include "monitor.hpp"
#include "applet.hpp"


ValueHistory::ValueHistory(Monitor *mon)
  : monitor(mon)
{
  wait_iterations = monitor->update_interval() / Applet::update_interval;
  waits_remaining = 0;
}

void ValueHistory::update(unsigned int max_samples, bool &new_value)
{
  --waits_remaining;
    
  if (waits_remaining <= 0) {
    new_value = true;
    monitor->measure();
    values.push_front(monitor->value());
    waits_remaining = wait_iterations;
  }
  else
    new_value = false;
  
  // get rid of extra samples (there may be more than one if user changes
  // configuration)
  while (values.size() > max_samples)
    values.pop_back();
}
