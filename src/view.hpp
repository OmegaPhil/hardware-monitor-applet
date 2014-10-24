/* Abstract base class for all views. Also contains some data.
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

#ifndef VIEW_HPP
#define VIEW_HPP

#include <config.h>

#include <string>
#include <list>

#include <glibmm/ustring.h>

#include "helpers.hpp"

class Applet;
class Monitor;

// something that can show something in an applet widget
class View: noncopyable
{
public:
  View(bool keeps_history);
  virtual ~View();

  void display(Applet &applet);
  void update();
  void attach(Monitor *monitor);
  void detach(Monitor *monitor);

  void set_background(unsigned int color);
  void unset_background();

  bool const keeps_history;

protected:
  Applet *applet;   // store pointer for reference

private:
  // for derived classes to override
  virtual void do_display() = 0;
  virtual void do_update() = 0;
  virtual void do_attach(Monitor *monitor) = 0;
  virtual void do_detach(Monitor *monitor) = 0;

  virtual void do_set_background(unsigned int color) = 0;
  virtual void do_unset_background() = 0;
};

#endif
