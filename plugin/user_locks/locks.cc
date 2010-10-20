/* - mode: c; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Copyright (C) 2010 Brian Aker
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

#include "config.h"
#include "plugin/user_locks/module.h"

#include <boost/thread/locks.hpp>

#include <string>

namespace user_locks {

bool Locks::lock(drizzled::session_id_t id_arg, const user_locks::Key &arg, int64_t wait_for)
{
  boost::system_time timeout= boost::get_system_time() + boost::posix_time::seconds(wait_for);
  boost::unique_lock<boost::mutex> scope(mutex);
  
  LockMap::iterator iter;
  while ((iter= lock_map.find(arg)) != lock_map.end())
  {
    if (id_arg == (*iter).second->id)
    {
      // We own the lock, so we just exit.
      return true;
    }
    bool success= cond.timed_wait(scope, timeout);

    if (not success)
      return false;
  }

  if (iter == lock_map.end())
  {
    std::pair<LockMap::iterator, bool> is_locked;

    is_locked= lock_map.insert(std::make_pair(arg, new lock_st(id_arg)));
    return is_locked.second;
  }

  return false;
}

bool Locks::lock(drizzled::session_id_t id_arg, const user_locks::Key &arg)
{
  std::pair<LockMap::iterator, bool> is_locked;
  boost::unique_lock<boost::mutex> scope(mutex);
  is_locked= lock_map.insert(std::make_pair(arg, new lock_st(id_arg)));

  return is_locked.second;
}

bool Locks::lock(drizzled::session_id_t id_arg, const user_locks::Keys &arg)
{
  boost::unique_lock<boost::mutex> scope(mutex);

  for (user_locks::Keys::const_iterator iter= arg.begin(); iter != arg.end(); iter++)
  {
    LockMap::iterator record= lock_map.find(*iter);

    if (record != lock_map.end())
    {
      if (id_arg != (*record).second->id)
        return false;
    }
  }

  for (Keys::iterator iter= arg.begin(); iter != arg.end(); iter++)
  {
    std::pair<LockMap::iterator, bool> is_locked;
    //is_locked can fail in cases where we already own the lock.
    is_locked= lock_map.insert(std::make_pair(*iter, new lock_st(id_arg)));
  }

  return true;
}

bool Locks::isUsed(const user_locks::Key &arg, drizzled::session_id_t &id_arg)
{
  boost::unique_lock<boost::mutex> scope(mutex);
  
  LockMap::iterator iter= lock_map.find(arg);
  
  if ( iter == lock_map.end())
    return false;

  id_arg= (*iter).second->id;

  return true;
}

bool Locks::isFree(const user_locks::Key &arg)
{
  boost::unique_lock<boost::mutex> scope(mutex);

  LockMap::iterator iter= lock_map.find(arg);
  
  return iter != lock_map.end();
}

void Locks::Copy(LockMap &lock_map_arg)
{
  lock_map_arg= lock_map;
}

boost::tribool Locks::release(const user_locks::Key &arg, drizzled::session_id_t &id_arg)
{
  size_t elements= 0;
  boost::unique_lock<boost::mutex> scope(mutex);
  LockMap::iterator iter= lock_map.find(arg);

  // Nothing is found
  if ( iter == lock_map.end())
    return boost::indeterminate;

  if ((*iter).second->id == id_arg)
    elements= lock_map.erase(arg);

  if (elements)
  {
    cond.notify_one();
    return true;
  }

  return false;
}

} /* namespace user_locks */
