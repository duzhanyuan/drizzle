/* Copyright (C) 2005 MySQL AB

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

#include "mysys/mysys_priv.h"
#include <mystrings/m_string.h>
#include <mysys/my_dir.h>

#include <stdio.h>
#include <algorithm>

using namespace std;

#define BUFF_SIZE 1024
#define RESERVE 1024                   /* Extend buffer with this extent */

#define NEWLINE "\n"
#define NEWLINE_LEN 1

static char *add_option(char *dst, const char *option_value,
			const char *option, int remove_option);


/*
  Add/remove option to the option file section.

  SYNOPSYS
    modify_defaults_file()
    file_location     The location of configuration file to edit
    option            The name of the option to look for (can be NULL)
    option value      The value of the option we would like to set (can be NULL)
    section_name      The name of the section (must be NOT NULL)
    remove_option     This defines what we want to remove:
                        - MY_REMOVE_NONE -- nothing to remove;
                        - MY_REMOVE_OPTION -- remove the specified option;
                        - MY_REMOVE_SECTION -- remove the specified section;
  IMPLEMENTATION
    We open the option file first, then read the file line-by-line,
    looking for the section we need. At the same time we put these lines
    into a buffer. Then we look for the option within this section and
    change/remove it. In the end we get a buffer with modified version of the
    file. Then we write it to the file, truncate it if needed and close it.
    Note that there is a small time gap, when the file is incomplete,
    and this theoretically might introduce a problem.

  RETURN
    0 - ok
    1 - some error has occured. Probably due to the lack of resourses
    2 - cannot open the file
*/

int modify_defaults_file(const char *file_location, const char *option,
                         const char *option_value,
                         const char *section_name, int remove_option)
{
  FILE *cnf_file;
  struct stat file_stat;
  char linebuff[BUFF_SIZE], *src_ptr, *dst_ptr, *file_buffer;
  size_t opt_len= 0, optval_len= 0, sect_len;
  uint32_t nr_newlines= 0;
  size_t buffer_size;
  bool in_section= false, opt_applied= 0;
  size_t reserve_extended;
  uint32_t new_opt_len;
  int reserve_occupied= 0;

  if (!(cnf_file= fopen(file_location, "r+")))
    return(2);

  if (fstat(fileno(cnf_file), &file_stat))
    goto malloc_err;

  if (option && option_value)
  {
    opt_len= strlen(option);
    optval_len= strlen(option_value);
  }

  new_opt_len= opt_len + 1 + optval_len + NEWLINE_LEN;

  /* calculate the size of the buffer we need */
  reserve_extended= (opt_len +
                     1 +                        /* For '=' char */
                     optval_len +               /* Option value len */
                     NEWLINE_LEN +              /* Space for newline */
                     RESERVE);                  /* Some additional space */

  buffer_size= (size_t)max((uint64_t)file_stat.st_size + 1, (uint64_t)SIZE_MAX);

  /*
    Reserve space to read the contents of the file and some more
    for the option we want to add.
  */
  if (!(file_buffer= (char*) malloc(max(buffer_size + reserve_extended,
                                         SIZE_MAX))))
    goto malloc_err;

  sect_len= strlen(section_name);

  for (dst_ptr= file_buffer; fgets(linebuff, BUFF_SIZE, cnf_file); )
  {
    /* Skip over whitespaces */
    for (src_ptr= linebuff; my_isspace(&my_charset_utf8_general_ci, *src_ptr);
	 src_ptr++)
    {}

    if (!*src_ptr) /* Empty line */
    {
      nr_newlines++;
      continue;
    }

    /* correct the option (if requested) */
    if (option && in_section && !strncmp(src_ptr, option, opt_len) &&
        (*(src_ptr + opt_len) == '=' ||
         my_isspace(&my_charset_utf8_general_ci, *(src_ptr + opt_len)) ||
         *(src_ptr + opt_len) == '\0'))
    {
      char *old_src_ptr= src_ptr;
      src_ptr= strchr(src_ptr+ opt_len, '\0');        /* Find the end of the line */

      /* could be negative */
      reserve_occupied+= (int) new_opt_len - (int) (src_ptr - old_src_ptr);
      if (reserve_occupied >= (int) reserve_extended)
      {
        reserve_extended= (uint32_t) reserve_occupied + RESERVE;
        if (!(file_buffer= (char*) realloc(file_buffer, buffer_size +
                                           reserve_extended)))
          goto malloc_err;
      }
      opt_applied= 1;
      dst_ptr= add_option(dst_ptr, option_value, option, remove_option);
    }
    else
    {
      /*
        If we are going to the new group and have an option to apply, do
        it now. If we are removing a single option or the whole section
        this will only trigger opt_applied flag.
      */

      if (in_section && !opt_applied && *src_ptr == '[')
      {
        dst_ptr= add_option(dst_ptr, option_value, option, remove_option);
        opt_applied= 1;           /* set the flag to do write() later */
        reserve_occupied= new_opt_len+ opt_len + 1 + NEWLINE_LEN;
      }

      for (; nr_newlines; nr_newlines--)
        dst_ptr= strcpy(dst_ptr, NEWLINE)+NEWLINE_LEN;

      /* Skip the section if MY_REMOVE_SECTION was given */
      if (!in_section || remove_option != MY_REMOVE_SECTION)
        dst_ptr= strcpy(dst_ptr, linebuff);
        dst_ptr+= strlen(linebuff);
    }
    /* Look for a section */
    if (*src_ptr == '[')
    {
      /* Copy the line to the buffer */
      if (!strncmp(++src_ptr, section_name, sect_len))
      {
        src_ptr+= sect_len;
        /* Skip over whitespaces. They are allowed after section name */
        for (; my_isspace(&my_charset_utf8_general_ci, *src_ptr); src_ptr++)
        {}

        if (*src_ptr != ']')
        {
          in_section= false;
          continue; /* Missing closing parenthesis. Assume this was no group */
        }

        if (remove_option == MY_REMOVE_SECTION)
          dst_ptr= dst_ptr - strlen(linebuff);

        in_section= true;
      }
      else
        in_section= false; /* mark that this section is of no interest to us */
    }
  }

  /*
    File ended. Apply an option or set opt_applied flag (in case of
    MY_REMOVE_SECTION) so that the changes are saved. Do not do anything
    if we are removing non-existent option.
  */

  if (!opt_applied && in_section && (remove_option != MY_REMOVE_OPTION))
  {
    /* New option still remains to apply at the end */
    if (!remove_option && *(dst_ptr - 1) != '\n')
      dst_ptr= strcpy(dst_ptr, NEWLINE)+NEWLINE_LEN;
    dst_ptr= add_option(dst_ptr, option_value, option, remove_option);
    opt_applied= 1;
  }
  for (; nr_newlines; nr_newlines--)
    dst_ptr= strcpy(dst_ptr, NEWLINE)+NEWLINE_LEN;

  if (opt_applied)
  {
    /* Don't write the file if there are no changes to be made */
    if (ftruncate(fileno(cnf_file), (size_t) (dst_ptr - file_buffer)) ||
        fseeko(cnf_file, 0, SEEK_SET) ||
        fwrite(file_buffer, 1, (size_t) (dst_ptr - file_buffer), cnf_file))
      goto err;
  }
  if (fclose(cnf_file))
    return(1);

  free(file_buffer);
  return(0);

err:
  free(file_buffer);
malloc_err:
  fclose(cnf_file);

  return 1; /* out of resources */
}


static char *add_option(char *dst, const char *option_value,
			const char *option, int remove_option)
{
  if (!remove_option)
  {
    dst= strcpy(dst, option);
    dst+= strlen(option);
    if (*option_value)
    {
      *dst++= '=';
      dst= strcpy(dst, option_value);
      dst+= strlen(option_value);
    }
    /* add a newline */
    dst= strcpy(dst, NEWLINE)+NEWLINE_LEN;
  }
  return dst;
}
