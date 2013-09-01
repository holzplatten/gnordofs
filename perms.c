/* -*- mode: C -*- Time-stamp: "2013-09-01 15:04:48 holzplatten"
 *
 *       File:         perms.c
 *       Author:       Pedro J. Ruiz Lopez (holzplatten@es.gnu.org)
 *       Date:         Mon Jun 17 01:15:33 2013
 *
 *       Rutinas de manejo de permisos.
 *
 */

/*
  Copyright (C) 2013 Pedro J. Ruiz López <holzplatten@es.gnu.org>

  This file is part of GnordoFS.

  GnordoFS is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  GnordoFS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with GnordoFS.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <errno.h>
#include <fcntl.h>
#include <fuse.h>

#include <inode.h>
#include <perms.h>

int
can_read_p(const inode_t * const inode)
{
  struct fuse_context * ctxt;

  ctxt = fuse_get_context();
  if (ctxt->uid == 0)
    return 1;

  return inode->perms & (S_IRUSR | S_IRGRP | S_IROTH);
}

int
can_write_p(const inode_t * const inode)
{
  struct fuse_context * ctxt;

  ctxt = fuse_get_context();
  if (ctxt->uid == 0)
    return 1;

  return inode->perms & (S_IWUSR | S_IWGRP | S_IWOTH);
}

int
can_exec_p(const inode_t * const inode)
{
  struct fuse_context * ctxt;

  ctxt = fuse_get_context();
  if (ctxt->uid == 0)
    return 1;

  return inode->perms & (S_IXUSR | S_IXGRP | S_IXOTH);
}
