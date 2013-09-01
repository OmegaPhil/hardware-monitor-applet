/* Implementation of the column view.
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

#include <cmath>

#include <libgnomecanvasmm/pixbuf.h>
#include <gconfmm/client.h>

#include "column-view.hpp"
#include "applet.hpp"
#include "monitor.hpp"
#include "value-history.hpp"

#include "pixbuf-drawing.hpp"


//
// class ColumnGraph - represents the columns in a column diagram
//

class ColumnGraph
{
public:
  ColumnGraph(Monitor *monitor);

  void update(unsigned int max_samples); // gather info from monitor
  void draw(Gnome::Canvas::Canvas &canvas, // redraw columns on canvas
	    Applet *applet, int width, int height); 

  Monitor *monitor;
  
private:
  // a pixbuf is used for the columns
  std::auto_ptr<Gnome::Canvas::Pixbuf> columns;

  ValueHistory value_history;
  int remaining_draws;
};

ColumnGraph::ColumnGraph(Monitor *m)
  : monitor(m), value_history(m), remaining_draws(0)
{
}

void ColumnGraph::update(unsigned int max_samples)
{
  bool new_value;
  value_history.update(max_samples, new_value);

  if (new_value)
    remaining_draws = CanvasView::draw_iterations;
}

void ColumnGraph::draw(Gnome::Canvas::Canvas &canvas,
		       Applet *applet, int width, int height)
{
  if (remaining_draws <= 0)
    return;

  --remaining_draws;
  
  double time_offset = double(remaining_draws) / CanvasView::draw_iterations;


  Glib::RefPtr<Gnome::Conf::Client> &client = applet->get_gconf_client();
    
  ValueHistory::iterator vi = value_history.values.begin(),
    vend = value_history.values.end();

  if (vi == vend)		// there must be at least one point
    return;

  
  // get colour
  unsigned int color;

  Gnome::Conf::Value v;
  
  v = client->get(monitor->get_gconf_dir() + "/color");
  if (v.get_type() == Gnome::Conf::VALUE_INT)
    color = v.get_int();
  else {
    color = applet->get_fg_color();
    client->set(monitor->get_gconf_dir() + "/color", int(color));
  }

  // make sure we got a pixbuf and that it has the right size
  Glib::RefPtr<Gdk::Pixbuf> pixbuf;

  if (columns.get() == 0)
    pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, width, height);
  else {
    pixbuf = columns->property_pixbuf();

    // but perhaps the dimensions have changed
    if (pixbuf->get_width() != width || pixbuf->get_height() != height)
      pixbuf = Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, width, height);
  }

  pixbuf->fill(color & 0xFFFFFF00);
  

  double max = monitor->max();
  if (max <= 0)
    max = 0.0000001;

  // start from right
  double l = width - ColumnView::pixels_per_sample
    + ColumnView::pixels_per_sample * time_offset;

  do {
    if (*vi >= 0) {
      // FIXME: the uppermost pixel should be scaled down too to avoid aliasing
      double r = l + ColumnView::pixels_per_sample;
      int
	t = int((1 - (*vi / max)) * (height - 1)),
	b = height - 1;

      if (t < 0)
	t = 0;
    
      for (int x = std::max(int(l), 0); x < std::min(r, double(width)); ++x) {
	PixelPosition pos = get_position(pixbuf, x, t);

	// anti-aliasing effect; if we are partly on a pixel, scale alpha down
	double scale = 1.0;
	if (x < l)
	  scale -= l - std::floor(l);
	if (x + 1 > r)
	  scale -= std::ceil(r) - r;

	int alpha = int((color & 0xFF) * scale);
      
	for (int y = t; y <= b; ++y, pos.down())
	  pos.pixel().alpha() = std::min(pos.pixel().alpha() + alpha, 255);
      }
    }
    
    l -= ColumnView::pixels_per_sample;
  } while (++vi != vend);
  
  // update columns
  if (columns.get() == 0)
    columns.reset(new Gnome::Canvas::Pixbuf(*canvas.root(), 0, 0, pixbuf));
  else
    columns->property_pixbuf() = pixbuf;
}


//
// class ColumnView
//

int const ColumnView::pixels_per_sample = 2;

ColumnView::ColumnView()
  : CanvasView(true)
{
}

ColumnView::~ColumnView()
{
  for (column_iterator i = columns.begin(), end = columns.end(); i != end; ++i)
    delete *i;
}

void ColumnView::do_update()
{
  CanvasView::do_update();
  
  // update each column graph
  for (column_iterator i = columns.begin(), end = columns.end(); i != end; ++i)
     // one extra because of animation
    (*i)->update(width() / pixels_per_sample + 1);
}

void ColumnView::do_attach(Monitor *monitor)
{
  columns.push_back(new ColumnGraph(monitor));
}

void ColumnView::do_detach(Monitor *monitor)
{
  for (column_iterator i = columns.begin(), end = columns.end(); i != end; ++i)
    if ((*i)->monitor == monitor) {
      delete *i;
      columns.erase(i);
      return;
    }

  g_assert_not_reached();
}

void ColumnView::do_draw_loop()
{
  for (column_iterator i = columns.begin(), end = columns.end(); i != end; ++i)
    (*i)->draw(*canvas, applet, width(), height());
}
