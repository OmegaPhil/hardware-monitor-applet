/* A value history class for measuring and keeping track of monitor values.
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

#ifndef VALUE_HISTORY_HPP
#define VALUE_HISTORY_HPP

#include <deque>

class Monitor;

class ValueHistory 
{
public:
  ValueHistory(Monitor *monitor);

  // perform a measurement if needed, new_value is set to true if it was
  void update(unsigned int max_samples, bool &new_value);
  
  // the past values
  typedef std::deque<double> sequence;
  typedef sequence::iterator iterator;
  sequence values;

private:
  Monitor *monitor;
  int wait_iterations, waits_remaining;
};


#endif
