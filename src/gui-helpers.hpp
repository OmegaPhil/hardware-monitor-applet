/* Helper functions.
 *
 * Copyright (c) 2003, 04 Ole Laursen.
 * Copyright (c) 2013 OmegaPhil (OmegaPhil@startmail.com)
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

#ifndef GUI_HELPERS_HPP
#define GUI_HELPERS_HPP

#include <config.h>

#include <glibmm/ustring.h>
#include <libglademm/xml.h>

#include "helpers.hpp"

// helper for loading a Glade XML file
inline Glib::RefPtr<Gnome::Glade::Xml> get_glade_xml(Glib::ustring root)
{
  try {
    return Gnome::Glade::Xml::create(HARDWARE_MONITOR_GLADEDIR
             "ui.glade", root);
  }
  catch (Gnome::Glade::XmlError &error) {
    fatal_error(error.what());
    return Glib::RefPtr<Gnome::Glade::Xml>();
  }
}

#endif
