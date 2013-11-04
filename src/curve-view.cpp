/* Implementation of the curve view.
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

#include <algorithm>		// for max/min[_element]()

#include <libgnomecanvasmm/line.h>
#include <libgnomecanvasmm/point.h>

#include "curve-view.hpp"
#include "applet.hpp"
#include "monitor.hpp"
#include "value-history.hpp"


//
// class Curve - represents a line curve
//

class Curve
{
public:
  Curve(Monitor *monitor, unsigned int color);

  void update(unsigned int max_samples); // gather info from monitor
  void draw(Gnome::Canvas::Canvas &canvas, // redraw curve on canvas
	    int width, int height);

  Monitor *monitor;
  
private:
  std::auto_ptr<Gnome::Canvas::Line> line;

  ValueHistory value_history;
  int remaining_draws;
  unsigned int color;
};

Curve::Curve(Monitor *m, unsigned int c)
  : monitor(m), value_history(m), remaining_draws(0), color(c)
{}

void Curve::update(unsigned int max_samples)
{
  bool new_value;
  value_history.update(max_samples, new_value);

  if (new_value)
    remaining_draws = CanvasView::draw_iterations;
}

void Curve::draw(Gnome::Canvas::Canvas &canvas, int width, int height)
{
  if (remaining_draws <= 0)
    return;

  --remaining_draws;
  
  double time_offset = double(remaining_draws) / CanvasView::draw_iterations;

  ValueHistory::iterator vi = value_history.values.begin(),
    vend = value_history.values.end();

  // only one point is pointless
  if (std::distance(vi, vend) < 2) 
    return;

  // make sure line is initialised
  if (line.get() == 0) {
    line.reset(new Gnome::Canvas::Line(*canvas.root()));
    line->property_smooth() = true;
    line->property_join_style() = Gdk::JOIN_ROUND;
  }

  // Get drawing attributes with defaults
  double const line_width = 1.5;

  line->property_fill_color_rgba() = color;
  line->property_width_units() = line_width;
  
  double max = monitor->max();
  if (max <= 0)
    max = 0.0000001;

  Gnome::Canvas::Points points;
  points.reserve(value_history.values.size());

  // start from right
  double x = width + CurveView::pixels_per_sample * time_offset;
	
  do {
    double y = line_width/2 + (1 - (*vi / max)) * (height - line_width/2);
    if (y < 0)
      y = 0;

    points.push_back(Gnome::Art::Point(x, y));
    x -= CurveView::pixels_per_sample;
  } while (++vi != vend);

  line->property_points() = points;

  // Debug code
  //std::cout << "In CurveView::draw!\n" << color << "\n";
}


//
// class CurveView
//

int const CurveView::pixels_per_sample = 2;

CurveView::CurveView()
  : CanvasView(true)
{
}

CurveView::~CurveView()
{
  for (curve_iterator i = curves.begin(), end = curves.end(); i != end; ++i)
    delete *i;
}

void CurveView::do_update()
{
  CanvasView::do_update();
  
  // then loop through each curve
  for (curve_iterator i = curves.begin(), end = curves.end(); i != end; ++i)
    // two extra because two points are end-points
    (*i)->update(width() / pixels_per_sample + 2);
}

void CurveView::do_attach(Monitor *monitor)
{
  unsigned int color = 0;
  bool color_missing = true;

  // Obtaining color
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

  /* Saving color if it was not recorded. XFCE4 configuration is done in
   * read and write stages, so this needs to be separated */
  if (color_missing)
  {
    // Setting color
    color = applet->get_fg_color();

    // Search for a writeable settings file, create one if it doesnt exist
    file = xfce_panel_plugin_save_location(applet->panel_applet, true);

    if (file)
    {
      // Opening setting file
      XfceRc* settings = xfce_rc_simple_open(file, false);
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
        "order to set color in Curve::do_attach call!\n");
    }
  }

  // Instantiating curve with determined color
  curves.push_back(new Curve(monitor, color));
}

void CurveView::do_detach(Monitor *monitor)
{
  for (curve_iterator i = curves.begin(), end = curves.end(); i != end; ++i)
    if ((*i)->monitor == monitor) {
      delete *i;
      curves.erase(i);
      return;
    }

  g_assert_not_reached();
}

void CurveView::do_draw_loop()
{
  for (curve_iterator i = curves.begin(), end = curves.end(); i != end; ++i)
    (*i)->draw(*canvas, width(), height());
}
