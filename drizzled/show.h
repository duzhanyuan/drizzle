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

/**
 * @file
 *
 * Contains function declarations that deal with the SHOW commands.  These
 * will eventually go away, but for now we split these definitions out into
 * their own header file for easier maintenance
 */
#ifndef DRIZZLED_SHOW_H
#define DRIZZLED_SHOW_H

#include <vector>

#include "drizzled/sql_list.h"
#include "drizzled/lex_string.h"
#include "drizzled/sql_parse.h"
#include "drizzled/plugin.h"

namespace drizzled
{

/* Forward declarations */
class String;
class Join;
class Session;
struct st_ha_create_information;
typedef st_ha_create_information HA_CREATE_INFO;
struct TableList;

class Table;
typedef class Item COND;

extern struct system_status_var global_status_var;

int store_create_info(TableList *table_list, String *packet, bool is_if_not_exists);

int wild_case_compare(const CHARSET_INFO * const cs, 
                      const char *str,const char *wildstr);

bool drizzled_show_create(Session *session, TableList *table_list, bool is_if_not_exists);
bool mysqld_show_create_db(Session &session, SchemaIdentifier &, bool if_not_exists);

bool mysqld_show_column_types(Session *session);

void init_status_vars();
void free_status_vars();
void reset_status_vars();

int get_quote_char_for_identifier();

} /* namespace drizzled */

#endif /* DRIZZLED_SHOW_H */
