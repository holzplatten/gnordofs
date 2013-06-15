#ifndef __BLOCK_H__
#define __BLOCK_H__

#include <superblock.h>

struct block
{
  unsigned char data[1024];
};

typedef struct block block_t;

int allocblk(int dev, superblock_t * const sb);
block_t * getblk(int dev, superblock_t *sb, int n);
int writeblk(int dev, superblock_t *sb, int n, block_t *datablock);


int free_block_list_init(int fd, const superblock_t * const sb);

void print_free_block_list(int fd, const superblock_t * const sb);

#endif
