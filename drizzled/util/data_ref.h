/* Drizzle
 * Copyright (C) 2011 Olaf van der Spek
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstddef>
#include <cstring>
#include <ostream>

template <class T>
class data_ref_basic
{
public:
  data_ref_basic()
  {
  }

  template <class U>
  data_ref_basic(const U& c)
  {
    if (c.begin() == c.end())
      clear();
    else
      assign(&*c.begin(), &*c.end());
  }

  data_ref_basic(const void* b, const void* e)
  {
    assign(b, e);
  }

  data_ref_basic(const void* b, size_t sz)
  {
    assign(b, sz);
  }

  explicit data_ref_basic(const char* b)
  {
    assign(b, strlen(b));
  }

  const data_ref_basic& clear()
  {
    begin_ = NULL;
    end_ = NULL;
    return *this;
  }

  void assign(const void* b, const void* e)
  {
    begin_ = reinterpret_cast<T>(b);
    end_ = reinterpret_cast<T>(e);
  }

  void assign(const void* b, size_t sz)
  {
    begin_ = reinterpret_cast<T>(b);
    end_ = begin_ + sz;
  }

  T begin() const
  {
    return begin_;
  }

  T end() const
  {
    return end_;
  }

  T data() const
  {
    return begin();
  }

  size_t size() const
  {
    return end() - begin();
  }

  bool empty() const
  {
    return begin() == end();
  }

  friend std::ostream& operator<<(std::ostream& os, data_ref_basic<T> v)
  {
    os.write(v.data(), v.size());
    return os;
  }
private:
  T begin_;
  T end_;
};

typedef data_ref_basic<const unsigned char*> data_ref;
typedef data_ref_basic<const char*> str_ref;