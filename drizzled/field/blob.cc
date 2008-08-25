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

#ifdef USE_PRAGMA_IMPLEMENTATION
#pragma implementation				// gcc: Class implementation
#endif

#include <drizzled/server_includes.h>
#include <drizzled/field/blob.h>

uint32_t
blob_pack_length_to_max_length(uint arg)
{
  return (1LL << min(arg, 4U) * 8) - 1LL;
}


/****************************************************************************
** blob type
** A blob is saved as a length and a pointer. The length is stored in the
** packlength slot and may be from 1-4.
****************************************************************************/

Field_blob::Field_blob(uchar *ptr_arg, uchar *null_ptr_arg, uchar null_bit_arg,
		       enum utype unireg_check_arg, const char *field_name_arg,
                       TABLE_SHARE *share, uint blob_pack_length,
		       const CHARSET_INFO * const cs)
  :Field_longstr(ptr_arg, blob_pack_length_to_max_length(blob_pack_length),
                 null_ptr_arg, null_bit_arg, unireg_check_arg, field_name_arg,
                 cs),
   packlength(blob_pack_length)
{
  flags|= BLOB_FLAG;
  share->blob_fields++;
  /* TODO: why do not fill table->s->blob_field array here? */
}


void Field_blob::store_length(uchar *i_ptr,
                              uint i_packlength,
                              uint32_t i_number,
                              bool low_byte_first __attribute__((unused)))
{
  switch (i_packlength) {
  case 1:
    i_ptr[0]= (uchar) i_number;
    break;
  case 2:
#ifdef WORDS_BIGENDIAN
    if (low_byte_first)
    {
      int2store(i_ptr,(unsigned short) i_number);
    }
    else
#endif
      shortstore(i_ptr,(unsigned short) i_number);
    break;
  case 3:
    int3store(i_ptr,i_number);
    break;
  case 4:
#ifdef WORDS_BIGENDIAN
    if (low_byte_first)
    {
      int4store(i_ptr,i_number);
    }
    else
#endif
      longstore(i_ptr,i_number);
  }
}


uint32_t Field_blob::get_length(const uchar *pos,
                              uint packlength_arg,
                              bool low_byte_first __attribute__((unused)))
{
  switch (packlength_arg) {
  case 1:
    return (uint32_t) pos[0];
  case 2:
    {
      uint16_t tmp;
#ifdef WORDS_BIGENDIAN
      if (low_byte_first)
	tmp=sint2korr(pos);
      else
#endif
	shortget(tmp,pos);
      return (uint32_t) tmp;
    }
  case 3:
    return (uint32_t) uint3korr(pos);
  case 4:
    {
      uint32_t tmp;
#ifdef WORDS_BIGENDIAN
      if (low_byte_first)
	tmp=uint4korr(pos);
      else
#endif
	longget(tmp,pos);
      return (uint32_t) tmp;
    }
  }
  return 0;					// Impossible
}


/**
  Put a blob length field into a record buffer.

  Depending on the maximum length of a blob, its length field is
  put into 1 to 4 bytes. This is a property of the blob object,
  described by 'packlength'.

  @param pos                 Pointer into the record buffer.
  @param length              The length value to put.
*/

void Field_blob::put_length(uchar *pos, uint32_t length)
{
  switch (packlength) {
  case 1:
    *pos= (char) length;
    break;
  case 2:
    int2store(pos, length);
    break;
  case 3:
    int3store(pos, length);
    break;
  case 4:
    int4store(pos, length);
    break;
  }
}


int Field_blob::store(const char *from,uint length, const CHARSET_INFO * const cs)
{
  uint copy_length, new_length;
  const char *well_formed_error_pos;
  const char *cannot_convert_error_pos;
  const char *from_end_pos, *tmp;
  char buff[STRING_BUFFER_USUAL_SIZE];
  String tmpstr(buff,sizeof(buff), &my_charset_bin);

  if (!length)
  {
    memset(ptr, 0, Field_blob::pack_length());
    return 0;
  }

  if (from == value.ptr())
  {
    uint32_t dummy_offset;
    if (!String::needs_conversion(length, cs, field_charset, &dummy_offset))
    {
      Field_blob::store_length(length);
      memcpy(ptr+packlength, &from, sizeof(char*));
      return 0;
    }
    if (tmpstr.copy(from, length, cs))
      goto oom_error;
    from= tmpstr.ptr();
  }

  new_length= min(max_data_length(), field_charset->mbmaxlen * length);
  if (value.alloc(new_length))
    goto oom_error;


  if (f_is_hex_escape(flags))
  {
    copy_length= my_copy_with_hex_escaping(field_charset,
                                           (char*) value.ptr(), new_length,
                                            from, length);
    Field_blob::store_length(copy_length);
    tmp= value.ptr();
    memcpy(ptr + packlength, &tmp, sizeof(char*));
    return 0;
  }
  /*
    "length" is OK as "nchars" argument to well_formed_copy_nchars as this
    is never used to limit the length of the data. The cut of long data
    is done with the new_length value.
  */
  copy_length= well_formed_copy_nchars(field_charset,
                                       (char*) value.ptr(), new_length,
                                       cs, from, length,
                                       length,
                                       &well_formed_error_pos,
                                       &cannot_convert_error_pos,
                                       &from_end_pos);

  Field_blob::store_length(copy_length);
  tmp= value.ptr();
  memcpy(ptr+packlength, &tmp, sizeof(char*));

  if (check_string_copy_error(this, well_formed_error_pos,
                              cannot_convert_error_pos, from + length, cs))
    return 2;

  return report_if_important_data(from_end_pos, from + length);

oom_error:
  /* Fatal OOM error */
  memset(ptr, 0, Field_blob::pack_length());
  return -1; 
}


int Field_blob::store(double nr)
{
  const CHARSET_INFO * const cs=charset();
  value.set_real(nr, NOT_FIXED_DEC, cs);
  return Field_blob::store(value.ptr(),(uint) value.length(), cs);
}


int Field_blob::store(int64_t nr, bool unsigned_val)
{
  const CHARSET_INFO * const cs=charset();
  value.set_int(nr, unsigned_val, cs);
  return Field_blob::store(value.ptr(), (uint) value.length(), cs);
}


double Field_blob::val_real(void)
{
  int not_used;
  char *end_not_used, *blob;
  uint32_t length;
  const CHARSET_INFO *cs;

  memcpy(&blob,ptr+packlength,sizeof(char*));
  if (!blob)
    return 0.0;
  length= get_length(ptr);
  cs= charset();
  return my_strntod(cs, blob, length, &end_not_used, &not_used);
}


int64_t Field_blob::val_int(void)
{
  int not_used;
  char *blob;
  memcpy(&blob,ptr+packlength,sizeof(char*));
  if (!blob)
    return 0;
  uint32_t length=get_length(ptr);
  return my_strntoll(charset(),blob,length,10,NULL,&not_used);
}

String *Field_blob::val_str(String *val_buffer __attribute__((unused)),
			    String *val_ptr)
{
  char *blob;
  memcpy(&blob,ptr+packlength,sizeof(char*));
  if (!blob)
    val_ptr->set("",0,charset());	// A bit safer than ->length(0)
  else
    val_ptr->set((const char*) blob,get_length(ptr),charset());
  return val_ptr;
}


my_decimal *Field_blob::val_decimal(my_decimal *decimal_value)
{
  const char *blob;
  size_t length;
  memcpy(&blob, ptr+packlength, sizeof(const uchar*));
  if (!blob)
  {
    blob= "";
    length= 0;
  }
  else
    length= get_length(ptr);

  str2my_decimal(E_DEC_FATAL_ERROR, blob, length, charset(),
                 decimal_value);
  return decimal_value;
}


int Field_blob::cmp(const uchar *a,uint32_t a_length, const uchar *b,
		    uint32_t b_length)
{
  return field_charset->coll->strnncollsp(field_charset, 
                                          a, a_length, b, b_length,
                                          0);
}


int Field_blob::cmp_max(const uchar *a_ptr, const uchar *b_ptr,
                        uint max_length)
{
  uchar *blob1,*blob2;
  memcpy(&blob1,a_ptr+packlength,sizeof(char*));
  memcpy(&blob2,b_ptr+packlength,sizeof(char*));
  uint a_len= get_length(a_ptr), b_len= get_length(b_ptr);
  set_if_smaller(a_len, max_length);
  set_if_smaller(b_len, max_length);
  return Field_blob::cmp(blob1,a_len,blob2,b_len);
}


int Field_blob::cmp_binary(const uchar *a_ptr, const uchar *b_ptr,
			   uint32_t max_length)
{
  char *a,*b;
  uint diff;
  uint32_t a_length,b_length;
  memcpy(&a,a_ptr+packlength,sizeof(char*));
  memcpy(&b,b_ptr+packlength,sizeof(char*));
  a_length=get_length(a_ptr);
  if (a_length > max_length)
    a_length=max_length;
  b_length=get_length(b_ptr);
  if (b_length > max_length)
    b_length=max_length;
  diff=memcmp(a,b,min(a_length,b_length));
  return diff ? diff : (int) (a_length - b_length);
}


/* The following is used only when comparing a key */

uint Field_blob::get_key_image(uchar *buff,
                               uint length,
                               imagetype type_arg __attribute__((unused)))
{
  uint32_t blob_length= get_length(ptr);
  uchar *blob;

  get_ptr(&blob);
  uint local_char_length= length / field_charset->mbmaxlen;
  local_char_length= my_charpos(field_charset, blob, blob + blob_length,
                          local_char_length);
  set_if_smaller(blob_length, local_char_length);

  if ((uint32_t) length > blob_length)
  {
    /*
      Must clear this as we do a memcmp in opt_range.cc to detect
      identical keys
    */
    memset(buff+HA_KEY_BLOB_LENGTH+blob_length, 0, (length-blob_length));
    length=(uint) blob_length;
  }
  int2store(buff,length);
  memcpy(buff+HA_KEY_BLOB_LENGTH, blob, length);
  return HA_KEY_BLOB_LENGTH+length;
}


void Field_blob::set_key_image(const uchar *buff,uint length)
{
  length= uint2korr(buff);
  (void) Field_blob::store((const char*) buff+HA_KEY_BLOB_LENGTH, length,
                           field_charset);
}


int Field_blob::key_cmp(const uchar *key_ptr, uint max_key_length)
{
  uchar *blob1;
  uint blob_length=get_length(ptr);
  memcpy(&blob1,ptr+packlength,sizeof(char*));
  const CHARSET_INFO * const cs= charset();
  uint local_char_length= max_key_length / cs->mbmaxlen;
  local_char_length= my_charpos(cs, blob1, blob1+blob_length,
                                local_char_length);
  set_if_smaller(blob_length, local_char_length);
  return Field_blob::cmp(blob1, blob_length,
			 key_ptr+HA_KEY_BLOB_LENGTH,
			 uint2korr(key_ptr));
}

int Field_blob::key_cmp(const uchar *a,const uchar *b)
{
  return Field_blob::cmp(a+HA_KEY_BLOB_LENGTH, uint2korr(a),
			 b+HA_KEY_BLOB_LENGTH, uint2korr(b));
}


/**
   Save the field metadata for blob fields.

   Saves the pack length in the first byte of the field metadata array
   at index of *metadata_ptr.

   @param   metadata_ptr   First byte of field metadata

   @returns number of bytes written to metadata_ptr
*/
int Field_blob::do_save_field_metadata(uchar *metadata_ptr)
{
  *metadata_ptr= pack_length_no_ptr();
  return 1;
}


uint32_t Field_blob::sort_length() const
{
  return (uint32_t) (current_thd->variables.max_sort_length + 
                   (field_charset == &my_charset_bin ? 0 : packlength));
}


void Field_blob::sort_string(uchar *to,uint length)
{
  uchar *blob;
  uint blob_length=get_length();

  if (!blob_length)
    memset(to, 0, length);
  else
  {
    if (field_charset == &my_charset_bin)
    {
      uchar *pos;

      /*
        Store length of blob last in blob to shorter blobs before longer blobs
      */
      length-= packlength;
      pos= to+length;

      switch (packlength) {
      case 1:
        *pos= (char) blob_length;
        break;
      case 2:
        mi_int2store(pos, blob_length);
        break;
      case 3:
        mi_int3store(pos, blob_length);
        break;
      case 4:
        mi_int4store(pos, blob_length);
        break;
      }
    }
    memcpy(&blob,ptr+packlength,sizeof(char*));
    
    blob_length=my_strnxfrm(field_charset,
                            to, length, blob, blob_length);
    assert(blob_length == length);
  }
}


void Field_blob::sql_type(String &res) const
{
  if (charset() == &my_charset_bin)
    res.set_ascii(STRING_WITH_LEN("blob"));
  else
    res.set_ascii(STRING_WITH_LEN("text"));
}

uchar *Field_blob::pack(uchar *to, const uchar *from,
                        uint max_length, bool low_byte_first)
{
  uchar *save= ptr;
  ptr= (uchar*) from;
  uint32_t length=get_length();			// Length of from string

  /*
    Store max length, which will occupy packlength bytes. If the max
    length given is smaller than the actual length of the blob, we
    just store the initial bytes of the blob.
  */
  store_length(to, packlength, min(length, max_length), low_byte_first);

  /*
    Store the actual blob data, which will occupy 'length' bytes.
   */
  if (length > 0)
  {
    get_ptr((uchar**) &from);
    memcpy(to+packlength, from,length);
  }
  ptr=save;					// Restore org row pointer
  return(to+packlength+length);
}


/**
   Unpack a blob field from row data.

   This method is used to unpack a blob field from a master whose size of 
   the field is less than that of the slave. Note: This method is included
   to satisfy inheritance rules, but is not needed for blob fields. It
   simply is used as a pass-through to the original unpack() method for
   blob fields.

   @param   to         Destination of the data
   @param   from       Source of the data
   @param   param_data @c true if base types should be stored in little-
                       endian format, @c false if native format should
                       be used.

   @return  New pointer into memory based on from + length of the data
*/
const uchar *Field_blob::unpack(uchar *to __attribute__((unused)),
                                const uchar *from,
                                uint param_data,
                                bool low_byte_first)
{
  uint const master_packlength=
    param_data > 0 ? param_data & 0xFF : packlength;
  uint32_t const length= get_length(from, master_packlength, low_byte_first);
  bitmap_set_bit(table->write_set, field_index);
  store(reinterpret_cast<const char*>(from) + master_packlength,
        length, field_charset);
  return(from + master_packlength + length);
}

/* Keys for blobs are like keys on varchars */

int Field_blob::pack_cmp(const uchar *a, const uchar *b, uint key_length_arg,
                         bool insert_or_update)
{
  uint a_length, b_length;
  if (key_length_arg > 255)
  {
    a_length=uint2korr(a); a+=2;
    b_length=uint2korr(b); b+=2;
  }
  else
  {
    a_length= (uint) *a++;
    b_length= (uint) *b++;
  }
  return field_charset->coll->strnncollsp(field_charset,
                                          a, a_length,
                                          b, b_length,
                                          insert_or_update);
}


int Field_blob::pack_cmp(const uchar *b, uint key_length_arg,
                         bool insert_or_update)
{
  uchar *a;
  uint a_length, b_length;
  memcpy(&a,ptr+packlength,sizeof(char*));
  if (!a)
    return key_length_arg > 0 ? -1 : 0;

  a_length= get_length(ptr);
  if (key_length_arg > 255)
  {
    b_length= uint2korr(b); b+=2;
  }
  else
    b_length= (uint) *b++;
  return field_charset->coll->strnncollsp(field_charset,
                                          a, a_length,
                                          b, b_length,
                                          insert_or_update);
}

/** Create a packed key that will be used for storage from a MySQL row. */

uchar *
Field_blob::pack_key(uchar *to, const uchar *from, uint max_length,
                     bool low_byte_first __attribute__((unused)))
{
  uchar *save= ptr;
  ptr= (uchar*) from;
  uint32_t length=get_length();        // Length of from string
  uint local_char_length= ((field_charset->mbmaxlen > 1) ?
                           max_length/field_charset->mbmaxlen : max_length);
  if (length)
    get_ptr((uchar**) &from);
  if (length > local_char_length)
    local_char_length= my_charpos(field_charset, from, from+length,
                                  local_char_length);
  set_if_smaller(length, local_char_length);
  *to++= (uchar) length;
  if (max_length > 255)				// 2 byte length
    *to++= (uchar) (length >> 8);
  memcpy(to, from, length);
  ptr=save;					// Restore org row pointer
  return to+length;
}


/**
  Unpack a blob key into a record buffer.

  A blob key has a maximum size of 64K-1.
  In its packed form, the length field is one or two bytes long,
  depending on 'max_length'.
  Depending on the maximum length of a blob, its length field is
  put into 1 to 4 bytes. This is a property of the blob object,
  described by 'packlength'.
  Blobs are internally stored apart from the record buffer, which
  contains a pointer to the blob buffer.


  @param to                          Pointer into the record buffer.
  @param from                        Pointer to the packed key.
  @param max_length                  Key length limit from key description.

  @return
    Pointer into 'from' past the last byte copied from packed key.
*/

const uchar *
Field_blob::unpack_key(uchar *to, const uchar *from, uint max_length,
                       bool low_byte_first __attribute__((unused)))
{
  /* get length of the blob key */
  uint32_t length= *from++;
  if (max_length > 255)
    length+= *from++ << 8;

  /* put the length into the record buffer */
  put_length(to, length);

  /* put the address of the blob buffer or NULL */
  if (length)
    memcpy(to + packlength, &from, sizeof(from));
  else
    memset(to + packlength, 0, sizeof(from));

  /* point to first byte of next field in 'from' */
  return from + length;
}


/** Create a packed key that will be used for storage from a MySQL key. */

uchar *
Field_blob::pack_key_from_key_image(uchar *to, const uchar *from, uint max_length,
                                    bool low_byte_first __attribute__((unused)))
{
  uint length=uint2korr(from);
  if (length > max_length)
    length=max_length;
  *to++= (char) (length & 255);
  if (max_length > 255)
    *to++= (char) (length >> 8);
  if (length)
    memcpy(to, from+HA_KEY_BLOB_LENGTH, length);
  return to+length;
}


uint Field_blob::packed_col_length(const uchar *data_ptr, uint length)
{
  if (length > 255)
    return uint2korr(data_ptr)+2;
  return (uint) *data_ptr + 1;
}


uint Field_blob::max_packed_col_length(uint max_length)
{
  return (max_length > 255 ? 2 : 1)+max_length;
}


uint Field_blob::is_equal(Create_field *new_field)
{
  if (compare_str_field_flags(new_field, flags))
    return 0;

  return ((new_field->sql_type == get_blob_type_from_length(max_data_length()))
          && new_field->charset == field_charset &&
          ((Field_blob *)new_field->field)->max_data_length() ==
          max_data_length());
}


/**
  maximum possible display length for blob.

  @return
    length
*/

uint32_t Field_blob::max_display_length()
{
  switch (packlength)
  {
  case 1:
    return 255 * field_charset->mbmaxlen;
  case 2:
    return 65535 * field_charset->mbmaxlen;
  case 3:
    return 16777215 * field_charset->mbmaxlen;
  case 4:
    return (uint32_t) 4294967295U;
  default:
    assert(0); // we should never go here
    return 0;
  }
}

