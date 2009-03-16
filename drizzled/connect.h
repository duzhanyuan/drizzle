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

/**
 * @file
 *
 * Contains function declarations that deal with connecting to the server,
 * creating a new connection, logging in, etc
 */
#ifndef DRIZZLE_SERVER_CONNECT_H
#define DRIZZLE_SERVER_CONNECT_H

extern "C"
pthread_handler_t handle_one_connection(void *arg);
bool init_new_connection_handler_thread();
bool setup_connection_thread_globals(Session *session);
void prepare_new_connection_state(Session* session);
void end_connection(Session *session);

#endif /* DRIZZLE_SERVER_CONNECT_H */
