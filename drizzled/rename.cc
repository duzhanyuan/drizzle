/* Copyright (C) 2000-2006 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/*
  Atomic rename of table;  RENAME TABLE t1 to t2, tmp to t1 [,...]
*/
#include <drizzled/server_includes.h>
#include <drizzled/error.h>
#include <drizzled/table_list.h>
#include <drizzled/session.h>
#include <drizzled/lock.h>
#include "drizzled/rename.h"

static TableList *rename_tables(Session *session, TableList *table_list,
                                bool skip_error);

static TableList *reverse_table_list(TableList *table_list);

/*
  Every second entry in the table_list is the original name and every
  second entry is the new name.
*/
bool drizzle_rename_tables(Session *session, TableList *table_list)
{
  bool error= true;
  TableList *ren_table= NULL;

  /*
    Avoid problems with a rename on a table that we have locked or
    if the user is trying to to do this in a transcation context
  */
  if (session->inTransaction())
  {
    my_message(ER_LOCK_OR_ACTIVE_TRANSACTION, ER(ER_LOCK_OR_ACTIVE_TRANSACTION), MYF(0));
    return true;
  }

  if (wait_if_global_read_lock(session,0,1))
    return true;

  pthread_mutex_lock(&LOCK_open); /* Rename table lock for exclusive access */
  if (lock_table_names_exclusively(session, table_list))
  {
    pthread_mutex_unlock(&LOCK_open);
    goto err;
  }

  error= false;
  if ((ren_table=rename_tables(session,table_list,0)))
  {
    /* Rename didn't succeed;  rename back the tables in reverse order */
    TableList *table;

    /* Reverse the table list */
    table_list= reverse_table_list(table_list);

    /* Find the last renamed table */
    for (table= table_list;
	 table->next_local != ren_table ;
	 table= table->next_local->next_local) ;
    table= table->next_local->next_local;		// Skip error table
    /* Revert to old names */
    rename_tables(session, table, 1);

    /* Revert the table list (for prepared statements) */
    table_list= reverse_table_list(table_list);

    error= true;
  }
  /*
    An exclusive lock on table names is satisfactory to ensure
    no other thread accesses this table.
    We still should unlock LOCK_open as early as possible, to provide
    higher concurrency - query_cache_invalidate can take minutes to
    complete.
  */
  pthread_mutex_unlock(&LOCK_open);

  /* Lets hope this doesn't fail as the result will be messy */
  if (!error)
  {
    write_bin_log(session, true, session->query, session->query_length);
    session->my_ok();
  }

  pthread_mutex_lock(&LOCK_open); /* unlock all tables held */
  unlock_table_names(table_list, NULL);
  pthread_mutex_unlock(&LOCK_open);

err:
  start_waiting_global_read_lock(session);

  return error;
}


/*
  reverse table list

  SYNOPSIS
    reverse_table_list()
    table_list pointer to table _list

  RETURN
    pointer to new (reversed) list
*/
static TableList *reverse_table_list(TableList *table_list)
{
  TableList *prev= NULL;

  while (table_list)
  {
    TableList *next= table_list->next_local;
    table_list->next_local= prev;
    prev= table_list;
    table_list= next;
  }
  return (prev);
}


/*
  Rename a single table or a view

  SYNPOSIS
    do_rename()
      session               Thread handle
      ren_table         A table/view to be renamed
      new_db            The database to which the table to be moved to
      new_table_name    The new table/view name
      skip_error        Whether to skip error

  DESCRIPTION
    Rename a single table or a view.

  RETURN
    false     Ok
    true      rename failed
*/

static bool
do_rename(Session *session, TableList *ren_table, const char *new_db, const char *new_table_name, bool skip_error)
{
  bool rc= true;
  const char *new_alias, *old_alias;

  {
    old_alias= ren_table->table_name;
    new_alias= new_table_name;
  }

  StorageEngine *engine= NULL;
  drizzled::message::Table table_proto;
  char path[FN_REFLEN];
  size_t length;

  length= build_table_filename(path, sizeof(path),
                               ren_table->db, old_alias, false);

  if (StorageEngine::getTableProto(path, &table_proto)!= EEXIST)
  {
    my_error(ER_NO_SUCH_TABLE, MYF(0), ren_table->db, old_alias);
    return true;
  }

  LEX_STRING engine_name= { (char*)table_proto.engine().name().c_str(),
                            strlen(table_proto.engine().name().c_str()) };
  engine= ha_resolve_by_name(session, &engine_name);

  length= build_table_filename(path, sizeof(path),
                               new_db, new_alias, false);

  if (StorageEngine::getTableProto(path, NULL)!=ENOENT)
  {
    my_error(ER_TABLE_EXISTS_ERROR, MYF(0), new_alias);
    return(1);			// This can't be skipped
  }

  rc= mysql_rename_table(engine,
                         ren_table->db, old_alias,
                         new_db, new_alias, 0);
  if (rc && !skip_error)
    return true;

  return false;

}
/*
  Rename all tables in list; Return pointer to wrong entry if something goes
  wrong.  Note that the table_list may be empty!
*/

/*
  Rename tables/views in the list

  SYNPOSIS
    rename_tables()
      session               Thread handle
      table_list        List of tables to rename
      skip_error        Whether to skip errors

  DESCRIPTION
    Take a table/view name from and odd list element and rename it to a
    the name taken from list element+1. Note that the table_list may be
    empty.

  RETURN
    false     Ok
    true      rename failed
*/

static TableList *
rename_tables(Session *session, TableList *table_list, bool skip_error)
{
  TableList *ren_table, *new_table;

  for (ren_table= table_list; ren_table; ren_table= new_table->next_local)
  {
    new_table= ren_table->next_local;
    if (do_rename(session, ren_table, new_table->db, new_table->table_name, skip_error))
      return(ren_table);
  }
  return(0);
}
