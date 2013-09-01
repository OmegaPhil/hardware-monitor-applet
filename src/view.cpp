/* Implementation of view base class.
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

#include "view.hpp"

View::View(bool kh)
  : keeps_history(kh)
{
}

View::View::~View()
{
}

void View::display(Applet &a)
{
  applet = &a;
  do_display();
}

void View::update()
{
  do_update();
}

void View::attach(Monitor *monitor)
{
  do_attach(monitor);
}

void View::detach(Monitor *monitor)
{
  do_detach(monitor);
}

void View::set_background(unsigned int color)
{
  do_set_background(color);
}

void View::unset_background()
{
  do_unset_background();
}
