/* - mode: c; c-basic-offset: 2; indent-tabs-mode: nil; -*-
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

/**
 * @file 
 *   Character Set I_S table methods.
 */

#include "drizzled/server_includes.h"
#include "drizzled/session.h"
#include "drizzled/show.h"

#include "helper_methods.h"
#include "collation.h"

#include <vector>

using namespace drizzled;
using namespace std;

/*
 * Vectors of columns for the collation I_S table.
 */
static vector<const plugin::ColumnInfo *> *columns= NULL;

/*
 * Methods for the collation I_S table.
 */
static plugin::InfoSchemaMethods *methods= NULL;

/*
 * collation I_S table.
 */
static plugin::InfoSchemaTable *coll_table= NULL;

/**
 * Populate the vectors of columns for the I_S table.
 *
 * @return a pointer to a std::vector of Columns.
 */
vector<const plugin::ColumnInfo *> *CollationIS::createColumns()
{
  if (columns == NULL)
  {
    columns= new vector<const plugin::ColumnInfo *>;
  }
  else
  {
    clearColumns(*columns);
  }

  /*
   * Create each column for the COLLATION table.
   */
  columns->push_back(new plugin::ColumnInfo("COLLATION_NAME",
                                            64,
                                            DRIZZLE_TYPE_VARCHAR,
                                            0,
                                            0,
                                            "Collation",
                                            SKIP_OPEN_TABLE));

  columns->push_back(new plugin::ColumnInfo("CHARACTER_SET_NAME",
                                            64,
                                            DRIZZLE_TYPE_VARCHAR,
                                            0,
                                            0,
                                            "Default collation",
                                            SKIP_OPEN_TABLE));

  columns->push_back(new plugin::ColumnInfo("DESCRIPTION",
                                            60,
                                            DRIZZLE_TYPE_VARCHAR,
                                            0,
                                            0,
                                            "Charset",
                                            SKIP_OPEN_TABLE));

  columns->push_back(new plugin::ColumnInfo("ID",
                                            MY_INT32_NUM_DECIMAL_DIGITS,
                                            DRIZZLE_TYPE_LONGLONG,
                                            0,
                                            0,
                                            "Id",
                                            SKIP_OPEN_TABLE));

  columns->push_back(new plugin::ColumnInfo("IS_DEFAULT",
                                            3,
                                            DRIZZLE_TYPE_VARCHAR,
                                            0,
                                            0,
                                            "Default",
                                            SKIP_OPEN_TABLE));

  columns->push_back(new plugin::ColumnInfo("IS_COMPILED",
                                            3,
                                            DRIZZLE_TYPE_VARCHAR,
                                            0,
                                            0,
                                            "Compiled",
                                            SKIP_OPEN_TABLE));

  columns->push_back(new plugin::ColumnInfo("SORTLEN",
                                            3,
                                            DRIZZLE_TYPE_LONGLONG,
                                            0,
                                            0,
                                            "Sortlen",
                                            SKIP_OPEN_TABLE));

  return columns;
}

/**
 * Initialize the I_S table.
 *
 * @return a pointer to an I_S table
 */
plugin::InfoSchemaTable *CollationIS::getTable()
{
  columns= createColumns();

  if (methods == NULL)
  {
    methods= new CollationISMethods();
  }

  if (coll_table == NULL)
  {
    coll_table= new(nothrow) plugin::InfoSchemaTable("COLLATIONS",
                                                     *columns,
                                                     -1, -1, false, false, 0,
                                                     methods);
  }

  return coll_table;
}

/**
 * Delete memory allocated for the table, columns and methods.
 */
void CollationIS::cleanup()
{
  clearColumns(*columns);
  delete coll_table;
  delete methods;
  delete columns;
}

int CollationISMethods::fillTable(Session *session, TableList *tables)
{
  CHARSET_INFO **cs;
  const char *wild= session->lex->wild ? session->lex->wild->ptr() : NULL;
  Table *table= tables->table;
  const CHARSET_INFO * const scs= system_charset_info;
  for (cs= all_charsets ; cs < all_charsets+255 ; cs++ )
  {
    CHARSET_INFO **cl;
    const CHARSET_INFO *tmp_cs= cs[0];
    if (! tmp_cs || ! (tmp_cs->state & MY_CS_AVAILABLE) ||
         (tmp_cs->state & MY_CS_HIDDEN) ||
        !(tmp_cs->state & MY_CS_PRIMARY))
      continue;
    for (cl= all_charsets; cl < all_charsets+255 ;cl ++)
    {
      const CHARSET_INFO *tmp_cl= cl[0];
      if (! tmp_cl || ! (tmp_cl->state & MY_CS_AVAILABLE) ||
          !my_charset_same(tmp_cs, tmp_cl))
        continue;
      if (! (wild && wild[0] &&
          wild_case_compare(scs, tmp_cl->name,wild)))
      {
        const char *tmp_buff;
        table->restoreRecordAsDefault();
        table->field[0]->store(tmp_cl->name, strlen(tmp_cl->name), scs);
        table->field[1]->store(tmp_cl->csname , strlen(tmp_cl->csname), scs);
        table->field[2]->store((int64_t) tmp_cl->number, true);
        tmp_buff= (tmp_cl->state & MY_CS_PRIMARY) ? "Yes" : "";
        table->field[3]->store(tmp_buff, strlen(tmp_buff), scs);
        tmp_buff= (tmp_cl->state & MY_CS_COMPILED)? "Yes" : "";
        table->field[4]->store(tmp_buff, strlen(tmp_buff), scs);
        table->field[5]->store((int64_t) tmp_cl->strxfrm_multiply, true);
        if (schema_table_store_record(session, table))
          return 1;
      }
    }
  }
  return 0;
}