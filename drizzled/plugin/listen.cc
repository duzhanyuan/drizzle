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

#include <drizzled/server_includes.h>
#include <drizzled/gettext.h>
#include <drizzled/error.h>
#include <drizzled/plugin/listen.h>
#include <drizzled/plugin/listen.h>
#include <drizzled/plugin/null_client.h>

#include <poll.h>

using namespace std;

namespace drizzled
{

vector<plugin::Listen *> listen_list;
vector<plugin::Listen *> listen_fd_list;
vector<pollfd> fd_list;
uint32_t fd_count= 0;
int wakeup_pipe[2];

bool plugin::Listen::addPlugin(plugin::Listen *listen_obj)
{
  listen_list.push_back(listen_obj);
  return false;
}

void plugin::Listen::removePlugin(plugin::Listen *listen_obj)
{
  listen_list.erase(remove(listen_list.begin(),
                           listen_list.end(),
                           listen_obj),
                    listen_list.end());
}

bool plugin::Listen::setup(void)
{
  vector<plugin::Listen *>::iterator it;

  for (it= listen_list.begin(); it < listen_list.end(); ++it)
  {
    vector<int> fds;
    vector<int>::iterator fd;

    if ((*it)->getFileDescriptors(fds))
      return true;

    fd_list.resize(fd_count + fds.size() + 1);

    for (fd= fds.begin(); fd < fds.end(); ++fd)
    {
      fd_list[fd_count].fd= *fd;
      fd_list[fd_count].events= POLLIN | POLLERR;
      listen_fd_list.push_back(*it);
      fd_count++;
    }
  }

  if (fd_count == 0)
  {
    errmsg_printf(ERRMSG_LVL_ERROR,
                  _("No sockets could be bound for listening"));
    return true;
  }

  /*
    We need a pipe to wakeup the listening thread since some operating systems
    are stupid. *cough* OSX *cough*
  */
  if (pipe(wakeup_pipe) == -1)
  {
    errmsg_printf(ERRMSG_LVL_ERROR, _("pipe() failed with errno %d"), errno);
    return true;
  }

  fd_list.resize(fd_count + 1);

  fd_list[fd_count].fd= wakeup_pipe[0];
  fd_list[fd_count].events= POLLIN | POLLERR;
  fd_count++;

  return false;
}

plugin::Client *plugin::Listen::getClient(void)
{
  int ready;
  uint32_t x;
  plugin::Client *client;

  while (1)
  {
    ready= poll(&fd_list[0], fd_count, -1);
    if (ready == -1)
    {
      if (errno != EINTR)
      {
        errmsg_printf(ERRMSG_LVL_ERROR, _("poll() failed with errno %d"),
                      errno);
      }

      continue;
    }
    else if (ready == 0)
      continue;

    for (x= 0; x < fd_count; x++)
    {
      if (fd_list[x].revents != POLLIN)
        continue;

      /* Check to see if the wakeup_pipe was written to. */
      if (x == fd_count - 1)
      {
        /* Close all file descriptors now. */
        for (x= 0; x < fd_count; x++)
        {
          (void) ::shutdown(fd_list[x].fd, SHUT_RDWR);
          (void) close(fd_list[x].fd);
          fd_list[x].fd= -1;
        }

        /* wakeup_pipe[0] was closed in the for loop above. */
        (void) close(wakeup_pipe[1]);

        return NULL;
      }

      if (!(client= listen_fd_list[x]->getClient(fd_list[x].fd)))
        continue;

      return client;
    }
  }
}

plugin::Client *plugin::Listen::getNullClient(void)
{
  return new plugin::NullClient();
}

void plugin::Listen::shutdown(void)
{
  ssize_t ret= write(wakeup_pipe[1], "\0", 1);
  assert(ret == 1);
}

} /* namespace drizzled */