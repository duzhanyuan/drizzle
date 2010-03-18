/* -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
 *  vim:expandtab:shiftwidth=2:tabstop=2:smarttab:
 *
 *  Copyright (C) 2008 Sun Microsystems
 *  Copyright (c) 2010 Jay Pipes <jaypipes@gmail.com>
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

#ifndef DRIZZLED_PLUGIN_XA_RESOURCE_MANAGER_H
#define DRIZZLED_PLUGIN_XA_RESOURCE_MANAGER_H

namespace drizzled
{

class XID;

namespace plugin
{

/**
 * An abstract interface class which exposes the participation
 * of implementing classes in distributed transactions in the XA protocol.
 */
class XaResourceManager
{
public:
  XaResourceManager() {}
  virtual ~XaResourceManager() {}

  int xaPrepare(Session *session, bool normal_transaction)
  {
    return doXaPrepare(session, normal_transaction);
  }

  int xaCommit(Session *session, bool normal_transaction)
  {
    return doXaCommit(session, normal_transaction);
  }

  int xaRollback(Session *session, bool normal_transaction)
  {
    return doXaRollback(session, normal_transaction);
  }

  int xaCommitXid(XID *xid)
  {
    return doXaCommitXid(xid);
  }

  int xaRollbackXid(XID *xid)
  {
    return doXaRollbackXid(xid);
  }

  int xaRecover(XID * append_to, size_t len)
  {
    return doXaRecover(append_to, len);
  }

  /** 
   * The below static class methods wrap the interaction
   * of the vector of registered XA storage engines.
   */
  static int commitOrRollbackXID(XID *xid, bool commit);
  static int recoverAllXids(HASH *commit_list);

  /* Class Methods for operating on plugin */
  static bool addPlugin(plugin::XaResourceManager *manager);
  static void removePlugin(plugin::XaResourceManager *manager);
private:
  /**
   * Does the COMMIT stage of the two-phase commit.
   */
  virtual int doXaCommit(Session *session, bool normal_transaction)= 0;
  /**
   * Does the ROLLBACK stage of the two-phase commit.
   */
  virtual int doXaRollback(Session *session, bool normal_transaction)= 0;
  /**
   * Does the PREPARE stage of the two-phase commit.
   */
  virtual int doXaPrepare(Session *session, bool normal_transaction)= 0;
  /**
   * Rolls back a transaction identified by a XID.
   */
  virtual int doXaRollbackXid(XID *xid)= 0;
  /**
   * Commits a transaction identified by a XID.
   */
  virtual int doXaCommitXid(XID *xid)= 0;
  /**
   * Notifies the transaction manager of any transactions
   * which had been marked prepared but not committed at
   * crash time or that have been heurtistically completed
   * by the storage engine.
   *
   * @param[out] Reference to a vector of XIDs to add to
   *
   * @retval
   *  Returns the number of transactions left to recover
   *  for this engine.
   */
  virtual int doXaRecover(XID * append_to, size_t len)= 0;
};

} /* namespace plugin */
} /* namespace drizzled */

#endif /* DRIZZLED_PLUGIN_XA_RESOURCE_MANAGER_H */
