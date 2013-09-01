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


#ifndef __FS_H__
#define __FS_H__

#include <fcntl.h>

#include <inode.h>
#include <superblock.h>

int do_read(int dev, superblock_t *sb, inode_t *inode,
            char *buffer, int n);
int do_lseek(int dev __attribute__((unused)),
             superblock_t *sb __attribute__((unused)),
             inode_t *inode, off_t offset, int whence);
int do_write(int dev, superblock_t *sb, inode_t *inode,
             const char * const buffer, int n);

#endif
