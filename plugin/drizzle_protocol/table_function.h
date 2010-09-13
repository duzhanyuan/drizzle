/* - mode: c; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Copyright (C) 2010 Andrew Hutchings
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
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

#ifndef PLUGIN_DRIZZLE_PROTOCOL_TABLE_FUNCTION_H
#define PLUGIN_DRIZZLE_PROTOCOL_TABLE_FUNCTION_H

#include "drizzled/plugin/table_function.h"
namespace drizzle_protocol
{


class DrizzleProtocolStatus : public drizzled::plugin::TableFunction
{
public:
  DrizzleProtocolStatus() :
    drizzled::plugin::TableFunction("DATA_DICTIONARY","DRIZZLE_PROTOCOL_STATUS")
  {
    add_field("VARIABLE_NAME");
    add_field("VARIABLE_VALUE");
  }

  class Generator : public drizzled::plugin::TableFunction::Generator
  {
    drizzled::drizzle_show_var *status_var_ptr;

  public:
    Generator(drizzled::Field **fields);

    bool populate();
  };

  Generator *generator(drizzled::Field **arg)
  {
    return new Generator(arg);
  }
};

}

#endif /* PLUGIN_DRIZZLE_PROTOCOL_TABLE_FUNCTION_H */