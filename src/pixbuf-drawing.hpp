/* Pixbuf-drawing helpers.
 *
 * Copyright (c) 2002, 03, 04 Ole Laursen.
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

#ifndef PIXBUF_DRAWING_HPP
#define PIXBUF_DRAWING_HPP

#include <iterator>

#include <gdkmm/pixbuf.h>
#include <glib/gtypes.h>

// scale pixbuf alpha values by scale / 256 (where scale <= 256)
void scale_alpha(const Glib::RefPtr<Gdk::Pixbuf> &pixbuf, int scale);

// shift hue of the pixels by the given shift amount
// which is in { 0, 1, ..., 6 * 256 - 1 }
void shift_hue(const Glib::RefPtr<Gdk::Pixbuf> &pixbuf, int shift);


class Pixel
{
public:
  Pixel()
  {}
  
  explicit Pixel(unsigned char *pixel)
    : data(pixel)
  {}

  // to facilitate -> on PixelIterators
  Pixel *operator->()
  {
    return this;
  }
  
  unsigned char &red()
  {
    return *data;
  }

  unsigned char const &red() const
  {
    return *data;
  }

  unsigned char &green()
  {
    return *(data + 1);
  }

  unsigned char const &green() const
  {
    return *(data + 1);
  }

  unsigned char &blue()
  {
    return *(data + 2);
  }

  unsigned char const &blue() const
  {
    return *(data + 2);
  }

  unsigned char &alpha()
  {
    return *(data + 3);
  }
  
  unsigned char const &alpha() const
  {
    return *(data + 3);
  }
  
private:
  unsigned char * data;
};

class PixelPosition
{
public:
  PixelPosition()
  {}

  PixelPosition(unsigned char *pixel, int rs, int chnls)
    : data(pixel), rowstride(rs), channels(chnls)
  {}

  Pixel pixel()
  {
    return Pixel(data);
  }

  PixelPosition &left(int n = 1)
  {
    data -= n * channels;
    return *this;
  }
  
  PixelPosition &right(int n = 1)
  {
    data += n * channels;
    return *this;
  }

  PixelPosition &up(int n = 1)
  {
    data -= n * rowstride;
    return *this;
  }
  
  PixelPosition &down(int n = 1)
  {
    data += n * rowstride;
    return *this;
  }
  
private:
  unsigned char *data;
  int rowstride, channels;
};

inline PixelPosition get_position(const Glib::RefPtr<Gdk::Pixbuf> &pixbuf,
				  int x, int y)
{
  unsigned char *data = pixbuf->get_pixels();
  int rowstride = pixbuf->get_rowstride();
  int channels = pixbuf->get_n_channels();
  
  data += rowstride * y + channels * x;

  return PixelPosition(data, rowstride, channels);
}


class PixelIterator {
public:
  typedef std::random_access_iterator_tag iterator_category;
  typedef Pixel value_type;
  typedef Pixel pointer;
  typedef Pixel reference;
  typedef std::ptrdiff_t difference_type;

  PixelIterator()
  {}
  
  PixelIterator(const Glib::RefPtr<Gdk::Pixbuf> &p,
		unsigned int x, unsigned int y)
    : xpos(x), width(p->get_width()), channels(p->get_n_channels()),
      padding(p->get_rowstride() - width * channels) 
  {
    data = p->get_pixels() + p->get_rowstride() * y + channels * x;
  }

  bool operator==(const PixelIterator& other) const
  {
    return data == other.data;
  }
  
  bool operator!=(const PixelIterator& other) const
  {
    return !(*this == other);
  }
  
  reference operator*() const
  {
    return Pixel(data);
  }

  pointer operator->() const
  {
    return operator*();
  }

  PixelIterator &operator++()
  {
    if (xpos == width) {
      data += padding;
      xpos = 0;
    }
    data += channels;
    ++xpos;
    return *this;
  }
  
  PixelIterator operator++(int)
  { 
    PixelIterator tmp = *this;
    ++*this;
    return tmp;
  }
  
  PixelIterator &operator--()
  { 
    if (xpos == 0) {
      data -= padding;
      xpos = width;

    }
    data -= channels;
    --xpos;
    return *this;
  }
  
  PixelIterator operator--(int)
  { 
    PixelIterator tmp = *this;
    --*this;
    return tmp;
  }

private:
  unsigned char *data;
  int xpos;
  
  int width, channels, padding;
};


inline PixelIterator begin(const Glib::RefPtr<Gdk::Pixbuf> &pixbuf)
{
  return PixelIterator(pixbuf, 0, 0);
}

inline PixelIterator end(const Glib::RefPtr<Gdk::Pixbuf> &pixbuf)
{
  return PixelIterator(pixbuf, 0, pixbuf->get_height());
}

#endif
