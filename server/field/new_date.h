/* - mode: c; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Copyright (C) 2008 MySQL
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

#ifndef DRIZZLE_SERVER_FIELD_NEW_DATE
#define DRIZZLE_SERVER_FIELD_NEW_DATE

#include "mysql_priv.h"

class Field_newdate :public Field_str {
public:
  Field_newdate(uchar *ptr_arg, uchar *null_ptr_arg, uchar null_bit_arg,
		enum utype unireg_check_arg, const char *field_name_arg,
		CHARSET_INFO *cs)
    :Field_str(ptr_arg, 10, null_ptr_arg, null_bit_arg,
	       unireg_check_arg, field_name_arg, cs)
    {}
  Field_newdate(bool maybe_null_arg, const char *field_name_arg,
                CHARSET_INFO *cs)
    :Field_str((uchar*) 0,10, maybe_null_arg ? (uchar*) "": 0,0,
               NONE, field_name_arg, cs) {}
  enum_field_types type() const { return MYSQL_TYPE_NEWDATE;}
  enum_field_types real_type() const { return MYSQL_TYPE_NEWDATE; }
  enum ha_base_keytype key_type() const { return HA_KEYTYPE_UINT24; }
  enum Item_result cmp_type () const { return INT_RESULT; }
  int  store(const char *to,uint length,CHARSET_INFO *charset);
  int  store(double nr);
  int  store(int64_t nr, bool unsigned_val);
  int store_time(MYSQL_TIME *ltime, timestamp_type type);
  int reset(void) { ptr[0]=ptr[1]=ptr[2]=0; return 0; }
  double val_real(void);
  int64_t val_int(void);
  String *val_str(String*,String *);
  bool send_binary(Protocol *protocol);
  int cmp(const uchar *,const uchar *);
  void sort_string(uchar *buff,uint length);
  uint32 pack_length() const { return 3; }
  void sql_type(String &str) const;
  bool can_be_compared_as_int64_t() const { return true; }
  bool zero_pack() const { return 1; }
  bool get_date(MYSQL_TIME *ltime,uint fuzzydate);
  bool get_time(MYSQL_TIME *ltime);
};

#endif

