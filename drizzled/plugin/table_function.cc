/* -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Copyright (C) 2010 Monty Taylor
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

#include "config.h"

#include <drizzled/plugin/table_function.h>
#include <drizzled/table_function_container.h>
#include <drizzled/gettext.h>
#include "drizzled/plugin/registry.h"
#include "drizzled/global_charset_info.h"
#include "drizzled/session.h"
#include "drizzled/current_session.h"

#include <vector>

using namespace std;

namespace drizzled
{

static TableFunctionContainer table_functions;

void plugin::TableFunction::init()
{
  drizzled::message::Table::StorageEngine *engine;
  drizzled::message::Table::TableOptions *table_options;

  proto.set_name(getTableLabel());
  proto.set_schema(identifier.getSchemaName());
  proto.set_type(drizzled::message::Table::FUNCTION);
  proto.set_creation_timestamp(0);
  proto.set_update_timestamp(0);

  table_options= proto.mutable_options();
  table_options->set_collation_id(default_charset_info->number);
  table_options->set_collation(default_charset_info->name);

  engine= proto.mutable_engine();
  engine->set_name("FunctionEngine");
}

bool plugin::TableFunction::addPlugin(plugin::TableFunction *tool)
{
  assert(tool != NULL);
  table_functions.addFunction(tool); 
  return false;
}

plugin::TableFunction *plugin::TableFunction::getFunction(const string &arg)
{
  return table_functions.getFunction(arg);
}

void plugin::TableFunction::getNames(const string &arg,
                                     set<std::string> &set_of_names)
{
  table_functions.getNames(arg, set_of_names);
}

plugin::TableFunction::Generator *plugin::TableFunction::generator(Field **arg)
{
  return new Generator(arg);
}

void plugin::TableFunction::add_field(const char *label,
                                      uint32_t field_length)
{
  add_field(label, TableFunction::STRING, field_length);
}

void plugin::TableFunction::add_field(const char *label,
                              TableFunction::ColumnType type,
                              bool is_default_null)
{
  add_field(label, type, 5, is_default_null);
}

void plugin::TableFunction::add_field(const char *label,
                              TableFunction::ColumnType type,
                              uint32_t field_length,
                              bool is_default_null)
{
  drizzled::message::Table::Field *field;
  drizzled::message::Table::Field::FieldOptions *field_options;
  drizzled::message::Table::Field::FieldConstraints *field_constraints;

  field= proto.add_field();
  field->set_name(label);

  field_options= field->mutable_options();
  field_constraints= field->mutable_constraints();
  field_options->set_default_null(is_default_null);
  field_constraints->set_is_nullable(is_default_null);

  switch (type) 
  {
  default:
  case TableFunction::BOOLEAN: // Currently BOOLEAN always has a value
    field_length= 5;
    field_options->set_default_null(false);
    field_constraints->set_is_nullable(false);
  case TableFunction::STRING:
  {
    drizzled::message::Table::Field::StringFieldOptions *string_field_options;
    field->set_type(drizzled::message::Table::Field::VARCHAR);

    string_field_options= field->mutable_string_options();
    string_field_options->set_length(field_length);
  }
    break;
  case TableFunction::VARBINARY:
  {
    drizzled::message::Table::Field::StringFieldOptions *string_field_options;
    field->set_type(drizzled::message::Table::Field::VARCHAR);

    string_field_options= field->mutable_string_options();
    string_field_options->set_length(field_length);
    string_field_options->set_collation(my_charset_bin.csname);
    string_field_options->set_collation_id(my_charset_bin.number);
  }
    break;
  case TableFunction::NUMBER: // Currently NUMBER always has a value
    field->set_type(drizzled::message::Table::Field::BIGINT);
    field_options->set_default_null(false);
    field_constraints->set_is_nullable(false);
    break;
  }
}

plugin::TableFunction::Generator::Generator(Field **arg) :
  columns(arg)
{
  scs= system_charset_info;
}

bool plugin::TableFunction::Generator::sub_populate(uint32_t field_size)
{
  bool ret;
  uint64_t difference;

  columns_iterator= columns;
  ret= populate();
  difference= columns_iterator - columns;

  if (ret == true)
  {
    assert(difference == field_size);
  }

  return ret;
}

void plugin::TableFunction::Generator::push(uint64_t arg)
{
  (*columns_iterator)->store(static_cast<int64_t>(arg), true);
  (*columns_iterator)->set_notnull();
  columns_iterator++;
}

void plugin::TableFunction::Generator::push(int64_t arg)
{
  (*columns_iterator)->store(arg, false);
  (*columns_iterator)->set_notnull();
  columns_iterator++;
}

void plugin::TableFunction::Generator::push(const char *arg, uint32_t length)
{
  assert(columns_iterator);
  assert(*columns_iterator);
  assert(arg);
  length= length ? length : strlen(arg);

  (*columns_iterator)->store(arg, length, scs);
  (*columns_iterator)->set_notnull();
  columns_iterator++;
}

void plugin::TableFunction::Generator::push()
{
  assert((*columns_iterator)->type()  == DRIZZLE_TYPE_VARCHAR);
  (*columns_iterator)->set_null();
  columns_iterator++;
}

void plugin::TableFunction::Generator::push(const std::string& arg)
{
  push(arg.c_str(), arg.length());
}

void plugin::TableFunction::Generator::push(bool arg)
{
  if (arg)
  {
    (*columns_iterator)->store("TRUE", 4, scs);
  }
  else
  {
    (*columns_iterator)->store("FALSE", 5, scs);
  }

  columns_iterator++;
}

bool plugin::TableFunction::Generator::isWild(const std::string &predicate)
{
  Session *session= current_session;

  if (not session->lex->wild)
    return false;

  bool match= wild_case_compare(system_charset_info,
                                predicate.c_str(),
                                session->lex->wild->ptr());

  return match;
}

} /* namespace drizzled */