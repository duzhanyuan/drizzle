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

#include <drizzled/server_includes.h>
#include <drizzled/show.h>
#include <drizzled/lock.h>
#include <drizzled/session.h>
#include <drizzled/statement/alter_table.h>

using namespace drizzled;

bool statement::AlterTable::execute()
{
  TableList *first_table= (TableList *) session->lex->select_lex.table_list.first;
  TableList *all_tables= session->lex->query_tables;
  assert(first_table == all_tables && first_table != 0);
  Select_Lex *select_lex= &session->lex->select_lex;
  bool need_start_waiting= false;

  /*
     Code in mysql_alter_table() may modify its HA_CREATE_INFO argument,
     so we have to use a copy of this structure to make execution
     prepared statement- safe. A shallow copy is enough as no memory
     referenced from this structure will be modified.
   */
  HA_CREATE_INFO create_info(session->lex->create_info);
  Alter_info alter_info(session->lex->alter_info, session->mem_root);

  if (session->is_fatal_error) /* out of memory creating a copy of alter_info */
  {
    return true;
  }

  /* Must be set in the parser */
  assert(select_lex->db);

  { // Rename of table
    TableList tmp_table;
    memset(&tmp_table, 0, sizeof(tmp_table));
    tmp_table.table_name= session->lex->name.str;
    tmp_table.db= select_lex->db;
  }

  /* ALTER TABLE ends previous transaction */
  if (! session->endActiveTransaction())
  {
    return true;
  }

  if (! (need_start_waiting= ! wait_if_global_read_lock(session, 0, 1)))
  {
    return true;
  }

  bool res= mysql_alter_table(session, 
                              select_lex->db, 
                              session->lex->name.str,
                              &create_info,
                              session->lex->create_table_proto,
                              first_table,
                              &alter_info,
                              select_lex->order_list.elements,
                              (order_st *) select_lex->order_list.first,
                              session->lex->ignore);
  /*
     Release the protection against the global read lock and wake
     everyone, who might want to set a global read lock.
   */
  start_waiting_global_read_lock(session);
  return res;
}
