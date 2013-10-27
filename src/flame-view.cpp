/* Implementation of the flame view.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
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

#include <cstdlib>
#include <cmath>
#include <vector>

#include <libgnomecanvasmm/pixbuf.h>

#include "pixbuf-drawing.hpp"

#include "flame-view.hpp"
#include "applet.hpp"
#include "monitor.hpp"


//
// class Flame - represents a flame layer
//

class Flame
{
public:
  Flame(Monitor *monitor);

  void update(Gnome::Canvas::Canvas &canvas,
	      Applet *applet, int width, int height, int no, int total); 

  void burn();
  
  Monitor *monitor;
  
private:
  std::auto_ptr<Gnome::Canvas::Pixbuf> flame;

  double value, max;

  std::vector<unsigned char> fuel;
  int next_refuel;
  int cooling; 			// cooling factor

  void recompute_fuel();
};

Flame::Flame(Monitor *m)
  : monitor(m), value(0), next_refuel(0)
{}

void Flame::update(Gnome::Canvas::Canvas &canvas,
		   Applet *applet, int width, int height, int no, int total)
{
  // Get color with default
  unsigned int color = applet->get_fg_color();
  bool color_missing = true;

  // Fetching assigned settings group
  Glib::ustring dir = monitor->get_settings_dir();

  // Search for settings file
  gchar* file = xfce_panel_plugin_lookup_rc_file(applet->panel_applet);

  if (file)
  {
    // One exists - loading readonly settings
    XfceRc* settings = xfce_rc_simple_open(file, true);
    g_free(file);

    // Loading color
    xfce_rc_set_group(settings, dir.c_str());
    if (xfce_rc_has_entry(settings, "color"))
    {
      color = xfce_rc_read_int_entry(settings, "color",
        applet->get_fg_color());
      color_missing = false;
    }

    // Close settings file
    xfce_rc_close(settings);
  }

  /* Saving if color was not recorded. XFCE4 configuration is done in
   * read and write stages, so this needs to be separated */
  if (color_missing)
  {
    // Search for a writeable settings file, create one if it doesnt exist
    file = xfce_panel_plugin_save_location(applet->panel_applet, true);

    if (file)
    {
      // Opening setting file
      settings = xfce_rc_simple_open(file, false);
      g_free(file);

      // Saving color
      xfce_rc_set_group(settings, dir.c_str());
      xfce_rc_write_int_entry(settings, "color", int(color));

      // Close settings file
      xfce_rc_close(settings);
    }
    else
    {
      // Unable to obtain writeable config file - informing user
      std::cerr << _("Unable to obtain writeable config file path in "
        "order to update color in Flame::update call!\n");
    }
  }

  // Then make sure layer is correctly setup
  if (flame.get() == 0)
  {
    Glib::RefPtr<Gdk::Pixbuf> p =
      Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, width, height);
    p->fill(color & 0xFFFFFF00);
    
    flame.reset(new Gnome::Canvas::Pixbuf(*canvas.root(), 0, 0, p));
  }
  else
  {
    Glib::RefPtr<Gdk::Pixbuf> pixbuf = flame->property_pixbuf();

    // perhaps the dimensions have changed
    if (pixbuf->get_width() != width || pixbuf->get_height() != height)
    {
      Glib::RefPtr<Gdk::Pixbuf> new_pixbuf =
	Gdk::Pixbuf::create(Gdk::COLORSPACE_RGB, true, 8, width, height);
      new_pixbuf->fill(color & 0xFFFFFF00);
      
      flame->property_pixbuf() = new_pixbuf;
    }
    else
    {
      // perhaps the color has changed
      PixelIterator i = begin(pixbuf);
      unsigned char red = color >> 24,
        green = color >> 16,
        blue = color >> 8;
      
      if (i->red() != red || i->green() != green || i->blue() != blue)
      {
        for (PixelIterator e = end(pixbuf); i != e; ++i)
        {
          Pixel pixel = *i;
          pixel.red() = red;
          pixel.green() = green;
          pixel.blue() = blue;
        }
	
        flame->property_pixbuf() = pixbuf;
      }
    }
  }
  
  // Finally just extract values, we don't draw here
  monitor->measure();
  value = monitor->value();
  
  max = monitor->max();
  if (max <= 0)
    max = 0.0000001;

  cooling =
    int((std::pow(-1.0 / (0.30 - 1), 1.0 / height) - 1) * 256);

  if (width != int(fuel.size()))
    fuel.resize(width);
}

unsigned int random_between(unsigned int min, unsigned int max)
{
  return min + std::rand() % (max - min);
}

void Flame::recompute_fuel()
{
  int ratio = int(value / max * 255);

  if (ratio > 255)
    ratio = 255;
  
  if (next_refuel <= 0) {
    next_refuel = random_between(5, 20);

    // The fuel values are calculated from parabels with the branches
    // turning downwards, concatenated at the roots; span is the
    // distance between the roots and max is the value at the summit
    int span = 0, max = 0, i = 0;

    for (std::vector<unsigned char>::iterator x = fuel.begin(),
      end = fuel.end(); x != end; ++x)
    {
      if (i <= 0)
      {
        // new graph, new parameters
        i = span = random_between(6, 16);
        max = random_between(255 + ratio * 3, 255 * 2 + ratio * 6) / 8;
      }
      else
      {
        //    y = -(x - |r1-r2|/2)^2 + y_max
        *x = - (i - span / 2) * (i - span / 2) + max;
        --i;
      }
    }
  }
  else
    --next_refuel;
}

void Flame::burn()
{
  if (!flame.get())
    return;
  
  Glib::RefPtr<Gdk::Pixbuf> pixbuf = flame->property_pixbuf();

  int width = pixbuf->get_width();
  int height = pixbuf->get_height();

  recompute_fuel();
  
  // Process the lowest row
  PixelPosition lowest = get_position(pixbuf, 0, height - 1);
  
  for (int x = 0; x < width; ++x)
  {
    lowest.pixel().alpha() = (lowest.pixel().alpha() * 3 + fuel[x]) / 4;
    lowest.right();
  }
  
  // Then process the rest of the pixbuf
  for (int y = height - 2; y >= 0; --y)
  {
    // Setup positions
    PixelPosition pos = get_position(pixbuf, 0, y),
      right = get_position(pixbuf, 2, y),
      below = get_position(pixbuf, 1, y + 1);

    unsigned char left_alpha = pos.pixel().alpha();
    pos.right();

    // process row
    for (int x = 1; x < width - 1; ++x)
    {
      // this is int to ensure enough precision in sum below
      unsigned int pos_alpha = pos.pixel().alpha(),
        right_alpha = right.pixel().alpha(),
        below_alpha = below.pixel().alpha();

      int tmp =
  (left_alpha + 6 * pos_alpha + right_alpha + 8 * below_alpha) / 16;

      pos.pixel().alpha()
	= std::max(((256 + cooling) * tmp - cooling * 256) / 256, 0);
  
#if 0
      if (std::rand() / 4 == 0)
	pos.pixel().alpha()
	  = (pos.pixel().alpha() * y * 2
	     + random_between(0, 255) * (height - y)) / height / 3;
#endif

      left_alpha = pos_alpha;
      pos.right();
      right.right();
      below.right();
    }
  }
  
  flame->property_pixbuf() = pixbuf;
}


//
// class FlameView
//

FlameView::FlameView()
  : CanvasView(false)
{
}

FlameView::~FlameView()
{
  for (flame_iterator i = flames.begin(), end = flames.end(); i != end; ++i)
    delete *i;
}

void FlameView::do_update()
{
  CanvasView::do_update();
  
  int total = flames.size();
  int no = 0;
  
  for (flame_iterator i = flames.begin(), end = flames.end(); i != end; ++i) {
    Flame &flame = **i;
    flame.update(*canvas, applet, width(), height(), no++, total);
  }
}

void FlameView::do_attach(Monitor *monitor)
{
  flames.push_back(new Flame(monitor));
}

void FlameView::do_detach(Monitor *monitor)
{
  for (flame_iterator i = flames.begin(), end = flames.end(); i != end; ++i)
    if ((*i)->monitor == monitor) {
      delete *i;
      flames.erase(i);
      return;
    }

  g_assert_not_reached();
}

void FlameView::do_draw_loop()
{
  for (flame_iterator i = flames.begin(), end = flames.end(); i != end; ++i)
    (*i)->burn();
}
