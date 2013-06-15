#ifndef __DIR_H__
#define __DIR_H__

#include <inode.h>
#include <superblock.h>

typedef struct dir_entry {
  int inode;
  unsigned char name[252];
} dir_entry_t;

#define DIR_MAX_ENTRIES 256

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
