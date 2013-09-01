/* Implementation of textual monitoring view.
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

#include <algorithm>
#include <cassert>

#include <gtkmm/label.h>
#include <pangomm/fontdescription.h>
#include <pangomm/attributes.h>
#include <pangomm/attrlist.h>

#include "text-view.hpp"
#include "monitor.hpp"
#include "applet.hpp"
#include "ucompose.hpp"
#include "i18n.hpp"

class Text
{
public:
  Text(Monitor *monitor);

  void add_to_table(Gtk::Table &table, int col, int row);
  void update(const Glib::ustring &font);

  Monitor *monitor;
  
private:
  std::auto_ptr<Gtk::Label> label;
};

Text::Text(Monitor *mon)
  : monitor(mon)
{
}

void Text::add_to_table(Gtk::Table &table, int col, int row)
{
  label.reset(new Gtk::Label);
  table.attach(*label, col, col + 1, row, row + 1,
	       Gtk::EXPAND | Gtk::FILL | Gtk::SHRINK, Gtk::SHRINK);
  label->set_alignment(0, 0.5);
}

void Text::update(const Glib::ustring &font)
{
  assert(label.get());

  Pango::AttrList attrlist;
  
  if (!font.empty()) {
    Pango::AttrFontDesc attr =
      Pango::Attribute::create_attr_font_desc(Pango::FontDescription(font));
    
    attrlist.insert(attr);
  }

  label->property_attributes() = attrlist;
  
  monitor->measure();
  label->set_text(monitor->format_value(monitor->value()));

  label->show();
}


TextView::TextView()
  : View(false)
{
}


void TextView::do_display()
{
  background_box.add(table);
  applet->get_container().add(background_box);

  table.show();
  background_box.show();
}

void TextView::do_update()
{
  // first update font
  Glib::RefPtr<Gnome::Conf::Client> &client = applet->get_gconf_client();
  Glib::ustring dir = applet->get_gconf_dir();

  // FIXME: use schemas?
  if (client->get(dir + "/viewer/font").get_type() == Gnome::Conf::VALUE_STRING)
    font = client->get_string(dir + "/viewer/font");
  else {
    font = "";
    client->set(dir + "/viewer/font", font);
  }

  // then update
  for (text_iterator i = texts.begin(), end = texts.end(); i != end; ++i)
    (*i)->update(font);
}

void TextView::do_attach(Monitor *monitor)
{
  texts.push_back(new Text(monitor));

  texts.back()->add_to_table(table, 1, texts.size());
  
  // FIXME: consider space restraints
#if 0
  int row = 1;
  for (text_iterator i = texts.begin(), end = texts.end(); i != end; ++i) {
    (*i)->add_to_table(table, 1, row++);
  }
#endif
}

void TextView::do_detach(Monitor *monitor)
{
  for (text_iterator i = texts.begin(), end = texts.end(); i != end; ++i)
    if ((*i)->monitor == monitor) {
      delete *i;
      texts.erase(i);
      return;
    }

  g_assert_not_reached();
}

void TextView::do_set_background(unsigned int color)
{
  Gdk::Color c;
  c.set_rgb(((color >> 24) & 0xff) * 256,
	    ((color >> 16) & 0xff) * 256,
	    ((color >>  8) & 0xff) * 256);
  
  background_box.modify_bg(Gtk::STATE_NORMAL, c);
  background_box.modify_bg(Gtk::STATE_ACTIVE, c);
  background_box.modify_bg(Gtk::STATE_PRELIGHT, c);
  background_box.modify_bg(Gtk::STATE_SELECTED, c);
  background_box.modify_bg(Gtk::STATE_INSENSITIVE, c);
}

void TextView::do_unset_background()
{
  // FIXME: convert to C++ code in gtkmm 2.4
  gtk_widget_modify_bg(background_box.Gtk::Widget::gobj(), GTK_STATE_NORMAL, 0);
  gtk_widget_modify_bg(background_box.Gtk::Widget::gobj(), GTK_STATE_ACTIVE, 0);
  gtk_widget_modify_bg(background_box.Gtk::Widget::gobj(), GTK_STATE_PRELIGHT, 0);
  gtk_widget_modify_bg(background_box.Gtk::Widget::gobj(), GTK_STATE_SELECTED, 0);
  gtk_widget_modify_bg(background_box.Gtk::Widget::gobj(), GTK_STATE_INSENSITIVE, 0);
}

int TextView::get_height() const
{
  // FIXME: determine from panel size
  return 44;
}
