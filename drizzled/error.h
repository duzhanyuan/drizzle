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

#ifndef DRIZZLED_ERROR_H
#define DRIZZLED_ERROR_H

#include <string>
#include "drizzled/definitions.h"

namespace drizzled
{

/* Max width of screen (for error messages) */
#define SC_MAXWIDTH 256
#define ERRMSGSIZE	(SC_MAXWIDTH)	/* Max length of a error message */
#define NRERRBUFFS	(2)	/* Buffers for parameters */
#define MY_FILE_ERROR	((size_t) -1)
#define ME_FATALERROR   1024    /* Fatal statement error */

typedef void (*error_handler_func)(uint32_t my_err,
                                   const char *str,
                                   myf MyFlags);
extern error_handler_func error_handler_hook;

// TODO: kill this method. Too much to do with this branch.
// This is called through the ER(x) macro.
const char * error_message(unsigned int err_index);

// Adds the message to the global error dictionary.
void add_error_message(uint32_t error_code, const std::string &error_name,
                       const std::string& message);
#define DRIZZLE_ADD_ERROR_MESSAGE(code, msg) add_error_message(code, STRINGIFY_ARG(code), msg)

void my_error(int nr, myf MyFlags, ...);
void my_message(uint32_t my_err, const char *str, myf MyFlags);
void my_printf_error(uint32_t my_err, const char *format,
                     myf MyFlags, ...)
                     __attribute__((format(printf, 2, 4)));

} /* namespace drizzled */

#endif /* DRIZZLED_ERROR_H */
