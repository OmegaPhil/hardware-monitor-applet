/* A view which displays a (time, value) curve plot.
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

#ifndef CURVE_VIEW_HPP
#define CURVE_VIEW_HPP

#include <list>
#include <vector>
#include <memory>

#include <libgnomecanvasmm/canvas.h>
#include <glibmm/ustring.h>

#include "canvas-view.hpp"


class Curve;

class CurveView: public CanvasView
{
public:
  CurveView();
  ~CurveView();
  
  static int const pixels_per_sample;

private:
  virtual void do_update();
  virtual void do_attach(Monitor *monitor);
  virtual void do_detach(Monitor *monitor);
  virtual void do_draw_loop();

  // must be destroyed before the canvas
  typedef std::list<Curve *> curve_sequence;
  typedef curve_sequence::iterator curve_iterator;
  curve_sequence curves;
};

#endif
