/* Copyright (C) 2009 PrimeBase Technologies GmbH
 *
 * PrimeBase XT
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * 2009-09-07	Paul McCullagh
 *
 * H&G2JCtL
 */

#ifndef __backup_xt_h__
#define __backup_xt_h__

#include "xt_defs.h"

#ifdef MYSQL_SUPPORTS_BACKUP

Backup_result_t pbxt_backup_engine(handlerton *self, Backup_engine* &be);

#endif
#endif
