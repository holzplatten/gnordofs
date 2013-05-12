#ifndef _BLOCK_H_
#define _BLOCK_H_

#include <superblock.h>

struct block
{
  unsigned char data[1024];
};

typedef struct block block_t;

int free_block_list_init(int fd, const superblock_t * const sb);

void print_free_block_list(int fd, const superblock_t * const sb);

#endif
