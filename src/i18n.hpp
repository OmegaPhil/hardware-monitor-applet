/* Internationalisation functions.
 *
 * Copyright (c) 2003 Ole Laursen.
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

#ifndef HARDWARE_MONITOR_I18N_HPP
#define HARDWARE_MONITOR_I18N_HPP

#ifndef GETTEXT_PACKAGE
#error "config.h must be included prior to i18n.hpp"
#endif

#include <libintl.h>
#include <locale.h>

#ifdef _
#undef _
#endif

#ifdef N_
#undef N_
#endif

//#define _(x) dgettext (GETTEXT_PACKAGE, x)
#define _(x) gettext (x)
#define N_(x) x

#endif
