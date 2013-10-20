/* Implementation of the non-abstract parts of canvas view.
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

#include <config.h>

#include <libgnomecanvasmm/pixbuf.h>

#include "canvas-view.hpp"
#include "applet.hpp"


int const CanvasView::draw_interval = 100;
int const CanvasView::draw_iterations = 10;

CanvasView::CanvasView(bool keeps_history)
  : View(keeps_history)
{
}

CanvasView::~CanvasView()
{
  draw_timer.disconnect();	// FIXME: is this enough to prevent crash?
}

void CanvasView::do_display()
{
  // canvas creation magic
  canvas.reset(new Gnome::Canvas::CanvasAA);
  applet->get_container().add(*canvas);

  draw_timer = Glib::signal_timeout()
    .connect(sigc::mem_fun(*this, &CanvasView::draw_loop), draw_interval);
  
  do_update();
  canvas->show();
}

void CanvasView::do_update()
{
  /* Obtaining size
   * Keeping with the default settings group for viewer settings
   * Search for settings file */
  gchar* file = xfce_panel_plugin_lookup_rc_file(applet->panel_applet);

  if (file)
  {
    // One exists - loading readonly settings
    XfceRc* settings = xfce_rc_simple_open(file, true);
    g_free(file);

    // Loading size
    bool size_missing = false;
    int size = 60;
    if (xfce_rc_has_entry(settings, "viewer_size"))
    {
      size = xfce_rc_read_int_entry(settings, "viewer_size", 60);
    }
    else
      size_missing = true;

    // Close settings file
    xfce_rc_close(settings);

    /* Viewer size not recorded - setting default then updating config. XFCE4
     * configuration is done in read and write stages, so this needs to
     * be separated */
    if (size_missing)
    {
      // Search for a writeable settings file, create one if it doesnt exist
      file = xfce_panel_plugin_save_location(applet->panel_applet, true);
	
      if (file)
      {
        // Opening setting file
        settings = xfce_rc_simple_open(file, false);
        g_free(file);

        // Saving viewer size
        xfce_rc_write_int_entry(settings, "viewer_size", size);
        
        // Close settings file
        xfce_rc_close(settings);
      }
      else
      {
        // Unable to obtain writeable config file - informing user
        std::cerr << _("Unable to obtain writeable config file path in "
          "order to update viewer size in CanvasView::do_update call!\n");
      }
    }
  }
  else
  {
    // Unable to obtain read only config file - informing user
    std::cerr << _("Unable to obtain read-only config file path in order"
      " to load viewer size in CanvasView::do_update call!\n");
  }

  // Ensure the canvas is shown
  resize_canvas();
}

void CanvasView::do_set_background(unsigned int color)
{
  Gdk::Color c;
  c.set_rgb(((color >> 24) & 0xff) * 256,
	    ((color >> 16) & 0xff) * 256,
	    ((color >>  8) & 0xff) * 256);
  
  canvas->modify_bg(Gtk::STATE_NORMAL, c);
  canvas->modify_bg(Gtk::STATE_ACTIVE, c);
  canvas->modify_bg(Gtk::STATE_PRELIGHT, c);
  canvas->modify_bg(Gtk::STATE_SELECTED, c);
  canvas->modify_bg(Gtk::STATE_INSENSITIVE, c);
}

void CanvasView::do_unset_background()
{
  // FIXME: convert to C++ code in gtkmm 2.4
  gtk_widget_modify_bg(canvas->Gtk::Widget::gobj(), GTK_STATE_NORMAL, 0);
  gtk_widget_modify_bg(canvas->Gtk::Widget::gobj(), GTK_STATE_ACTIVE, 0);
  gtk_widget_modify_bg(canvas->Gtk::Widget::gobj(), GTK_STATE_PRELIGHT, 0);
  gtk_widget_modify_bg(canvas->Gtk::Widget::gobj(), GTK_STATE_SELECTED, 0);
  gtk_widget_modify_bg(canvas->Gtk::Widget::gobj(), GTK_STATE_INSENSITIVE, 0);
}

int CanvasView::width() const
{
  if (applet->horizontal())
    return size;
  else
    return applet->get_size();
}

int CanvasView::height() const
{
  if (applet->horizontal())
    return applet->get_size();
  else
    return size;
}

void CanvasView::resize_canvas()
{
  int w = width(), h = height();

  double x1, y1, x2, y2;
  canvas->get_scroll_region(x1, y1, x2, y2);
  
  if (x1 != 0 || y1 != 0 || x2 != w || y2 != h) {
    canvas->set_scroll_region(0, 0, w, h);
    canvas->set_size_request(w, h);
  }
}

bool CanvasView::draw_loop()
{
  do_draw_loop();
  return true;
}

