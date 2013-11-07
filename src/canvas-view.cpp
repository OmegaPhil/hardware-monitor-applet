/* Implementation of the non-abstract parts of canvas view.
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

#include <config.h>

#include <iostream>

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
  draw_timer.disconnect();  // FIXME: is this enough to prevent crash?
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
  // Debug code
  //std::cout << "In CanvasView::do_update!\n";

  // Size is maintained in applet
  size = applet->get_viewer_size();

  /* Ensure that the widget's requested size is being honoured on every
   * call */
  applet->set_viewer_size(size);

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
  /* Remember that applet->get_size returns the thickness of the panel
   * (i.e. height in the normal orientation or width in the vertical
   * orientation) */

  // Debug code
  //std::cout << "CanvasView::width: " << ((applet->horizontal()) ? size : applet->get_size()) << "\n";

  if (applet->horizontal())
    return size;
  else
    return applet->get_size();
}

int CanvasView::height() const
{
  // Debug code
  //std::cout << "CanvasView::height: " << ((applet->horizontal()) ? applet->get_size() : size) << "\n";

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

  // Debug code
  //std::cout << "In CanvasView::resize_canvas!\n" << w << "|" << h << "\n";
}

bool CanvasView::draw_loop()
{
  do_draw_loop();
  return true;
}

