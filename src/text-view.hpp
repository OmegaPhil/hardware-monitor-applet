/* A textual monitoring view.
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

#ifndef TEXT_VIEW_HPP
#define TEXT_VIEW_HPP

#include <memory>

#include <gtkmm/eventbox.h>
#include <gtkmm/table.h>

#include "view.hpp"

class Text;

class TextView: public View
{
public:
  TextView();
  
private:
  virtual void do_display();
  virtual void do_update();
  virtual void do_attach(Monitor *monitor);
  virtual void do_detach(Monitor *monitor);
  virtual void do_set_background(unsigned int color);
  virtual void do_unset_background();

  int get_height() const;

  Gtk::EventBox background_box;
  Gtk::Table table;
  
  Glib::ustring font;

  typedef std::list<Text *> text_sequence;
  typedef text_sequence::iterator text_iterator;
  text_sequence texts;
};


#endif
