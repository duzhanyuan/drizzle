/* Copyright (C) 2000-2003 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#ifndef DRIZZLED_PARSER_H
#define DRIZZLED_PARSER_H

#include <drizzled/foreign_key.h>
#include <drizzled/lex_symbol.h>
#include <drizzled/function/locate.h>
#include <drizzled/function/str/char.h>
#include <drizzled/function/str/collation.h>
#include <drizzled/function/str/insert.h>
#include <drizzled/function/str/left.h>
#include <drizzled/function/str/repeat.h>
#include <drizzled/function/str/replace.h>
#include <drizzled/function/str/right.h>
#include <drizzled/function/str/set_collation.h>
#include <drizzled/function/str/trim.h>
#include <drizzled/function/time/curdate.h>
#include <drizzled/function/time/date_add_interval.h>
#include <drizzled/function/time/dayofmonth.h>
#include <drizzled/function/time/extract.h>
#include <drizzled/function/time/hour.h>
#include <drizzled/function/time/microsecond.h>
#include <drizzled/function/time/minute.h>
#include <drizzled/function/time/month.h>
#include <drizzled/function/time/now.h>
#include <drizzled/function/time/quarter.h>
#include <drizzled/function/time/second.h>
#include <drizzled/function/time/sysdate_local.h>
#include <drizzled/function/time/timestamp_diff.h>
#include <drizzled/function/time/typecast.h>
#include <drizzled/function/time/year.h>

#include <drizzled/error.h>
#include <drizzled/nested_join.h>
#include <drizzled/sql_parse.h>
#include <drizzled/item/copy_string.h>
#include <drizzled/item/cmpfunc.h>
#include <drizzled/item/uint.h>
#include <drizzled/item/null.h>
#include <drizzled/session.h>
#include <drizzled/item/func.h>
#include <drizzled/sql_base.h>
#include <drizzled/item/create.h>
#include <drizzled/item/default_value.h>
#include <drizzled/item/insert_value.h>
#include <drizzled/lex_string.h>
#include <drizzled/function/get_system_var.h>
#include <drizzled/thr_lock.h>
#include <drizzled/message/table.pb.h>
#include <drizzled/message/schema.pb.h>
#include <drizzled/statement.h>
#include <drizzled/statement/alter_schema.h>
#include <drizzled/statement/alter_table.h>
#include <drizzled/statement/analyze.h>
#include <drizzled/statement/catalog.h>
#include <drizzled/statement/change_schema.h>
#include <drizzled/statement/check.h>
#include <drizzled/statement/commit.h>
#include <drizzled/statement/create_index.h>
#include <drizzled/statement/create_schema.h>
#include <drizzled/statement/create_table.h>
#include <drizzled/statement/create_table/like.h>
#include <drizzled/statement/create_table/select.h>
#include <drizzled/statement/delete.h>
#include <drizzled/statement/drop_index.h>
#include <drizzled/statement/drop_schema.h>
#include <drizzled/statement/drop_table.h>
#include <drizzled/statement/empty_query.h>
#include <drizzled/statement/execute.h>
#include <drizzled/statement/flush.h>
#include <drizzled/statement/insert.h>
#include <drizzled/statement/insert_select.h>
#include <drizzled/statement/kill.h>
#include <drizzled/statement/load.h>
#include <drizzled/statement/release_savepoint.h>
#include <drizzled/statement/rename_table.h>
#include <drizzled/statement/replace.h>
#include <drizzled/statement/replace_select.h>
#include <drizzled/statement/rollback.h>
#include <drizzled/statement/rollback_to_savepoint.h>
#include <drizzled/statement/savepoint.h>
#include <drizzled/statement/select.h>
#include <drizzled/statement/show.h>
#include <drizzled/statement/set_option.h>
#include <drizzled/statement/show_errors.h>
#include <drizzled/statement/show_warnings.h>
#include <drizzled/statement/start_transaction.h>
#include <drizzled/statement/truncate.h>
#include <drizzled/statement/unlock_tables.h>
#include <drizzled/statement/update.h>
#include <drizzled/db.h>
#include "drizzled/global_charset_info.h"
#include "drizzled/pthread_globals.h"
#include "drizzled/charset.h"
#include "drizzled/internal/m_string.h"

#include "drizzled/item/boolean.h"

namespace drizzled {

class Session;
class Table_ident;
class Item;
class Item_num;

namespace item
{
class Boolean;
class True;
class False;
}

namespace parser {

Item* handle_sql2003_note184_exception(Session *session, Item* left, bool equal, Item *expr);
bool add_select_to_union_list(Session *session, LEX *lex, bool is_union_distinct);
bool setup_select_in_parentheses(Session *session, LEX *lex);
Item* reserved_keyword_function(Session *session, const std::string &name, List<Item> *item_list);
void my_parse_error(Lex_input_stream *lip);
void my_parse_error(const char *message);
bool check_reserved_words(LEX_STRING *name);
void errorOn(const char *s);
bool buildOrderBy(Session *session);
void buildEngineOption(Session *session, const char *key, const LEX_STRING &value);
void buildEngineOption(Session *session, const char *key, uint64_t value);
void buildSchemaOption(Session *session, const char *key, const LEX_STRING &value);
void buildSchemaOption(Session *session, const char *key, uint64_t value);

} // namespace parser
} // namespace drizzled

#endif /* DRIZZLED_PARSER_H */
