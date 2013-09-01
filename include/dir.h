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


#ifndef __DIR_H__
#define __DIR_H__

#include <inode.h>
#include <superblock.h>

typedef struct dir_entry {
  int inode;
  unsigned char name[252];
} dir_entry_t;

#define DIR_MAX_ENTRIES 4*BLOCKS_PER_INODE

int add_dir_entry(int dev, superblock_t *, inode_t *,
                  inode_t * entry_inode,
                  const char * const entry_name);
int del_dir_entry_by_name(int dev, superblock_t *, inode_t *,
                          const char * const entry_name);
dir_entry_t * get_dir_entry(int dev, superblock_t *,
                            inode_t *, int n);
dir_entry_t * get_dir_entry_by_name(int dev, superblock_t *,
                                    inode_t *, char *name);

#endif
