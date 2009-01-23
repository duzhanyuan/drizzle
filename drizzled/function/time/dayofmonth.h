/* -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Copyright (C) 2008 Sun Microsystems
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef DRIZZLED_FUNCTION_TIME_DAYOFMONTH_H
#define DRIZZLED_FUNCTION_TIME_DAYOFMONTH_H

#include <drizzled/function/func.h>
#include <drizzled/function/math/int.h>


class Item_func_dayofmonth :public Item_int_func
{
public:
  Item_func_dayofmonth(Item *a) :Item_int_func(a) {}
  int64_t val_int();
  const char *func_name() const { return "dayofmonth"; }
  void fix_length_and_dec()
  {
    decimals=0;
    max_length=2*MY_CHARSET_BIN_MB_MAXLEN;
    maybe_null=1;
  }
  bool check_vcol_func_processor(unsigned char *)
  { return false; }
};

#endif /* DRIZZLED_FUNCTION_TIME_DAYOFMONTH_H */
