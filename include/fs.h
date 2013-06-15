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
