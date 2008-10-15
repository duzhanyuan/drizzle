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

#ifndef DRIZZLED_FUNCTIONS_RAND_H
#define DRIZZLED_FUNCTIONS_RAND_H

#include <drizzled/functions/func.h> 

class Item_func_rand :public Item_real_func
{
  struct rand_struct *rand;
public:
  Item_func_rand(Item *a) :Item_real_func(a), rand(0) {}
  Item_func_rand()        :Item_real_func() {}
  double val_real();
  const char *func_name() const { return "rand"; }
  bool const_item() const { return 0; }
  void update_used_tables();
  bool fix_fields(THD *thd, Item **ref);
  bool check_vcol_func_processor(unsigned char *int_arg __attribute__((unused)))
  { return true; }
private:
  void seed_random (Item * val);
};

#endif /* DRIZZLED_FUNCTIONS_RAND_H */
