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


#ifndef __BLOCK_H__
#define __BLOCK_H__

#include <superblock.h>

#define BLOCK_SIZE 4096

struct block
{
  unsigned char data[BLOCK_SIZE];
};

typedef struct block block_t;

long allocblk(int dev, superblock_t * const sb);
block_t * getblk(int dev, superblock_t *sb, long n);
int writeblk(int dev, superblock_t *sb, long n, block_t *datablock);
int freeblk(int dev, superblock_t * const sb, long block);

int free_block_list_init(int fd, const superblock_t * const sb);

void print_free_block_list(int fd, const superblock_t * const sb);

#endif
