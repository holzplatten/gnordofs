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


#ifndef __INODE_H__
#define __INODE_H__

#include <block.h>

#define BLK_UNASSIGNED -1492
#define unassigned_p(x) ( (x) == BLK_UNASSIGNED )

typedef enum {
  I_FREE = 0,
  I_FILE,
  I_DIR
} itype_t;

#define N_DIRECT_BLOCKS 10
#define N_SINGLE_INDIRECT_BLOCKS (sizeof(struct block) / sizeof(long))

#define INODE_PERSISTENT_DATA                                      \
  itype_t type;                                                    \
  unsigned size;                                                   \
  unsigned link_counter;                                           \
                                                                   \
  time_t atime;                                                    \
  time_t ctime;                                                    \
  time_t mtime;                                                    \
                                                                   \
  unsigned owner;                                                  \
  unsigned group;                                                  \
                                                                   \
  unsigned perms;                                                  \
                                                                   \
  long direct_blocks[N_DIRECT_BLOCKS];                             \
  long single_indirect_blocks;

#define BLOCKS_PER_INODE (N_DIRECT_BLOCKS + 1*N_SINGLE_INDIRECT_BLOCKS)

struct inode {
  INODE_PERSISTENT_DATA
  
  unsigned n;
  unsigned offset_ptr;
};

typedef struct inode inode_t;

inode_t * namei(int fd, superblock_t * const sb, char * path);
inode_t * iget(int dev, const superblock_t * const sb, int n);
inode_t * ialloc(int dev, superblock_t * const sb);
int iput(int dev, const superblock_t * const sb, inode_t * inode);

long inode_getblk(int dev, superblock_t * const sb,
                  inode_t * inode, long blk);
long inode_allocblk(int dev, superblock_t * const sb,
                    inode_t * inode, long blk);
int inode_freeblk(int dev, superblock_t * const sb,
                  inode_t * inode, long blk);
int inode_truncate(int dev, superblock_t * const sb, inode_t *inode, int size);

int inode_list_init(int fd, const superblock_t * const sb);


#endif
