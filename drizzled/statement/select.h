/* -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Copyright (C) 2009 Sun Microsystems
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

#ifndef DRIZZLED_STATEMENT_SELECT_H
#define DRIZZLED_STATEMENT_SELECT_H

#include <drizzled/statement.h>

namespace drizzled
{
class Session;

namespace statement
{

class Select : public Statement
{
  /* These will move out once we have args for table functions */
  std::string show_schema;
  std::string show_table;

public:
  Select(Session *in_session)
    :
      Statement(in_session)
  {}

  void setShowPredicate(const std::string &schema_arg, const std::string &table_arg)
  {
    show_schema= schema_arg;
    show_table= table_arg;
  }

  const std::string getShowSchema()
  {
    return show_schema;
  }

  const std::string getShowTable()
  {
    return show_table;
  }

  bool execute();
};

} /* namespace statement */
} /* namespace drizzled */

#endif /* DRIZZLED_STATEMENT_SELECT_H */
