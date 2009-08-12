/* -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Copyright (C) 2009 Sun Microsystems
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

#ifndef DRIZZLED_OPTIMIZER_KEY_FIELD_H
#define DRIZZLED_OPTIMIZER_KEY_FIELD_H

#include <drizzled/server_includes.h>
#include <drizzled/sql_select.h>

#include <vector>

namespace drizzled
{
namespace optimizer
{

/**
 * Structure used when finding key fields
 */
typedef struct key_field_t 
{
  Field *field;
  Item *val; /**< May be empty if diff constant */
  uint32_t level;
  uint32_t optimize; /**< KEY_OPTIMIZE_* */
  bool eq_func;
  /**
    If true, the condition this struct represents will not be satisfied
    when val IS NULL.
  */
  bool null_rejecting;
  bool *cond_guard; /**< @see KeyUse::cond_guard */
} KEY_FIELD;

void add_key_fields(JOIN *join, 
                    KEY_FIELD **key_fields,
                    uint32_t *and_level,
                    COND *cond,
                    table_map usable_tables,
                    std::vector<SargableParam> &sargables);

void add_key_part(DYNAMIC_ARRAY *keyuse_array, KEY_FIELD *key_field);

/*
  Add to KEY_FIELD array all 'ref' access candidates within nested join.

    This function populates KEY_FIELD array with entries generated from the
    ON condition of the given nested join, and does the same for nested joins
    contained within this nested join.

  @param[in]      nested_join_table   Nested join pseudo-table to process
  @param[in,out]  end                 End of the key field array
  @param[in,out]  and_level           And-level
  @param[in,out]  sargables           std::vector of found sargable candidates


  @note
    We can add accesses to the tables that are direct children of this nested
    join (1), and are not inner tables w.r.t their neighbours (2).

    Example for #1 (outer brackets pair denotes nested join this function is
    invoked for):
    @code
     ... LEFT JOIN (t1 LEFT JOIN (t2 ... ) ) ON cond
    @endcode
    Example for #2:
    @code
     ... LEFT JOIN (t1 LEFT JOIN t2 ) ON cond
    @endcode
    In examples 1-2 for condition cond, we can add 'ref' access candidates to
    t1 only.
    Example #3:
    @code
     ... LEFT JOIN (t1, t2 LEFT JOIN t3 ON inner_cond) ON cond
    @endcode
    Here we can add 'ref' access candidates for t1 and t2, but not for t3.
*/
void add_key_fields_for_nj(JOIN *join,
                           TableList *nested_join_table,
                           KEY_FIELD **end,
                           uint32_t *and_level,
                           std::vector<SargableParam> &sargables);

/**
  Merge new key definitions to old ones, remove those not used in both.

  This is called for OR between different levels.

  To be able to do 'ref_or_null' we merge a comparison of a column
  and 'column IS NULL' to one test.  This is useful for sub select queries
  that are internally transformed to something like:.

  @code
  SELECT * FROM t1 WHERE t1.key=outer_ref_field or t1.key IS NULL
  @endcode

  KEY_FIELD::null_rejecting is processed as follows: @n
  result has null_rejecting=true if it is set for both ORed references.
  for example:
  -   (t2.key = t1.field OR t2.key  =  t1.field) -> null_rejecting=true
  -   (t2.key = t1.field OR t2.key <=> t1.field) -> null_rejecting=false

  @todo
    The result of this is that we're missing some 'ref' accesses.
    OptimizerTeam: Fix this
*/
KEY_FIELD *merge_key_fields(KEY_FIELD *start,
                            KEY_FIELD *new_fields,
                            KEY_FIELD *end, 
                            uint32_t and_level);

/**
  Add a possible key to array of possible keys if it's usable as a key

    @param key_fields      Pointer to add key, if usable
    @param and_level       And level, to be stored in KEY_FIELD
    @param cond            Condition predicate
    @param field           Field used in comparision
    @param eq_func         True if we used =, <=> or IS NULL
    @param value           Value used for comparison with field
    @param usable_tables   Tables which can be used for key optimization
    @param sargables       IN/OUT std::vector of found sargable candidates

  @note
    If we are doing a NOT NULL comparison on a NOT NULL field in a outer join
    table, we store this to be able to do not exists optimization later.

  @returns
    *key_fields is incremented if we stored a key in the array
*/
void add_key_field(KEY_FIELD **key_fields,
                   uint32_t and_level,
                   Item_func *cond,
                   Field *field,
                   bool eq_func,
                   Item **value,
                   uint32_t num_values,
                   table_map usable_tables,
                   std::vector<SargableParam> &sargables);

/**
  Add possible keys to array of possible keys originated from a simple
  predicate.

    @param  key_fields     Pointer to add key, if usable
    @param  and_level      And level, to be stored in KEY_FIELD
    @param  cond           Condition predicate
    @param  field          Field used in comparision
    @param  eq_func        True if we used =, <=> or IS NULL
    @param  value          Value used for comparison with field
                           Is NULL for BETWEEN and IN
    @param  usable_tables  Tables which can be used for key optimization
    @param  sargables      IN/OUT std::vector of found sargable candidates

  @note
    If field items f1 and f2 belong to the same multiple equality and
    a key is added for f1, the the same key is added for f2.

  @returns
    *key_fields is incremented if we stored a key in the array
*/
void add_key_equal_fields(KEY_FIELD **key_fields,
                          uint32_t and_level,
                          Item_func *cond,
                          Item_field *field_item,
                          bool eq_func,
                          Item **val,
                          uint32_t num_values,
                          table_map usable_tables,
                          std::vector<SargableParam> &sargables);

} /* end namespace optimizer */

} /* end namespace drizzled */

#endif /* DRIZZLED_OPTIMIZER_KEY_FIELD_H */
