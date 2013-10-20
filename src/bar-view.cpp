/* Implementation of the bar view.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
 * Copyright (c) 2013 OmegaPhil (OmegaPhil+hardware.monitor@gmail.com)
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

#include <vector>
#include <cmath>		// for ceil/floor
#include <algorithm>		// for max/min

#include <libgnomecanvasmm/rect.h>

#include "bar-view.hpp"
#include "applet.hpp"
#include "monitor.hpp"


//
// class Bar - represents a single bar graph
//

class Bar
{
public:
  Bar(Monitor *monitor, bool horizontal = false);
  ~Bar();

  void update();
  void draw(Gnome::Canvas::Canvas &canvas,
	    Applet *applet, int width, int height, int no, int total,
	    double time_offset); 

  Monitor *monitor;
  
private:
  typedef std::vector<Gnome::Canvas::Rect *> box_sequence;
  box_sequence boxes;

  double old_value, new_value;
  bool horizontal;
};

Bar::Bar(Monitor *m, bool horizontal)
  : monitor(m), old_value(0), new_value(0)
{
  this->horizontal = horizontal;
}

Bar::~Bar()
{
  for (box_sequence::iterator i = boxes.begin(), end = boxes.end(); i != end;
       ++i)
    delete *i;
}

void Bar::update()
{
  monitor->measure();
  old_value = new_value;
  new_value = monitor->value();
}

unsigned int outlineified(unsigned int color)
{
  int
    r = (color >> 24) & 0xff,
    g = (color >> 16) & 0xff,
    b = (color >> 8) & 0xff;

  if ((r + g + b) / 3 < 50) {
    // enlight instead of darken
    r = std::min(255, int(r * 1.2));
    g = std::min(255, int(g * 1.2));
    b = std::min(255, int(b * 1.2));
  }
  else {
    r = std::max(0, int(r * 0.8));
    g = std::max(0, int(g * 0.8));
    b = std::max(0, int(b * 0.8));
  }
  
  return (r << 24) | (g << 16) | (b << 8) | (color & 0xff);
}

void Bar::draw(Gnome::Canvas::Canvas &canvas,
	       Applet *applet, int width, int height, int no, int total,
	       double time_offset)
{
  // get drawing attributes
  unsigned int fill_color;

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
    bool color_missing = false;
    xfce_rc_set_group(settings, dir.c_str());
    if (xfce_rc_has_entry(settings, "color"))
    {
      fill_color = xfce_rc_read_int_entry(settings, "color",
                                          applet->get_fg_color());
    }
    else
      color_missing = true;

    // Close settings file
    xfce_rc_close(settings);

    /* Color not recorded - setting default then updating config. XFCE4
     * configuration is done in read and write stages, so this needs to
     * be separated */
    if (color_missing)
    {
      fill_color = applet->get_fg_color();

      // Search for a writeable settings file, create one if it doesnt exist
      file = xfce_panel_plugin_save_location(applet->panel_applet, true);

      if (file)
      {
        // Opening setting file
        settings = xfce_rc_simple_open(file, false);
        g_free(file);

        // Saving color
        xfce_rc_set_group(settings, dir.c_str());
        xfce_rc_write_int_entry(settings, "color", int(fill_color));

        // Close settings file
        xfce_rc_close(settings);
      }
      else
      {
        // Unable to obtain writeable config file - informing user
        std::cerr << _("Unable to obtain writeable config file path in "
          "order to update color in Bar::draw call!\n");
      }
    }
  }
  else
  {
    // Unable to obtain read only config file - informing user
    std::cerr << _("Unable to obtain read-only config file path in order"
      " to load color in Bar::draw call!\n");
  }
  
  unsigned int outline_color = outlineified(fill_color);
  
  // calculate parameters
  int box_size;
  // use min_spacing at least, except for last box which doesn't need spacing
  int total_no_boxes;
  double box_spacing;

  if (this->horizontal) {
    box_size = 3;
    int const min_spacing = 2;
    total_no_boxes = (width + min_spacing) / (box_size + min_spacing);
    box_spacing = (double(width) - box_size * total_no_boxes) / (total_no_boxes - 1);
  }
  else {
    // Assume that a vertical view has limited space, thus the number of boxes
    // is hardcoded
    total_no_boxes = 5;
    box_spacing = 2;
    int const total_no_spacings = total_no_boxes - 1;
    box_size = int(double(height - (total_no_spacings * box_spacing)) / total_no_boxes);
  }
  
  
  // don't attain new value immediately
  double value = old_value * (1 - time_offset) + new_value * time_offset;

  double max = monitor->max();
  if (max <= 0)
    max = 0.0000001;

  double box_frac = total_no_boxes * value / max;
  if (box_frac > total_no_boxes)
    box_frac = total_no_boxes;
  unsigned int no_boxes = int(std::ceil(box_frac));
  double alpha = box_frac - std::floor(box_frac);

  if (alpha == 0)		// x.0 should give an opaque last box
    alpha = 1;
  
  // trim/expand boxes list
  while (boxes.size() < no_boxes)
    boxes.push_back(new Gnome::Canvas::Rect(*canvas.root()));
  while (boxes.size() > no_boxes) {
    delete boxes.back();
    boxes.pop_back();
  }

  double coord = this->horizontal ? 0 : height;
  // update parameters, starting from left
  for (box_sequence::iterator i = boxes.begin(), end = boxes.end(); i != end;
       ++i) {
    Gnome::Canvas::Rect &rect = **i;
    rect.property_fill_color_rgba() = fill_color;
    rect.property_outline_color_rgba() = outline_color;
    
    if (this->horizontal) {
      rect.property_x1() = coord;
      rect.property_x2() = coord + box_size;
      rect.property_y1() = double(height) * no / total + 1;
      rect.property_y2() = double(height) * (no + 1) / total - 1;
      
      coord += box_size + box_spacing;
    }
    else {
	    rect.property_x1() = double(width) * no / total + 1;
	    rect.property_x2() = double(width) * (no + 1) / total - 1;
	    rect.property_y1() = coord;
	    rect.property_y2() = coord - box_size;
      
      coord -= (box_size + box_spacing);
    }
  }

  // tint last box
  if (!boxes.empty()) {
    Gnome::Canvas::Rect &last = *boxes.back();
    last.property_fill_color_rgba()
      = (fill_color & 0xffffff00) |
      static_cast<unsigned int>((fill_color & 0xff) * alpha);
    last.property_outline_color_rgba()
      = (outline_color & 0xffffff00) |
      static_cast<unsigned int>((outline_color & 0xff) * alpha);
  }
}


//
// class BarView
//

BarView::BarView(bool _horizontal)
  : CanvasView(false), draws_since_update(0)
{
  horizontal = _horizontal;
}

BarView::~BarView()
{
  for (bar_iterator i = bars.begin(), end = bars.end(); i != end; ++i)
    delete *i;
}

void BarView::do_update()
{
  CanvasView::do_update();

  draws_since_update = 0;
  
  for (bar_iterator i = bars.begin(), end = bars.end(); i != end; ++i)
    (*i)->update();
}

void BarView::do_attach(Monitor *monitor)
{
  bars.push_back(new Bar(monitor, this->horizontal));
}

void BarView::do_detach(Monitor *monitor)
{
  for (bar_iterator i = bars.begin(), end = bars.end(); i != end; ++i)
    if ((*i)->monitor == monitor) {
      delete *i;
      bars.erase(i);
      return;
    }

  g_assert_not_reached();
}

void BarView::do_draw_loop()
{
 double time_offset = double(draws_since_update) / CanvasView::draw_iterations;
 
  int total = bars.size();
  int no = 0;
  
  for (bar_iterator i = bars.begin(), end = bars.end(); i != end; ++i)
    (*i)->draw(*canvas, applet, width(), height(), no++, total, time_offset);

  ++draws_since_update;
}

bool BarView::is_horizontal() {
  return this->horizontal;
}
