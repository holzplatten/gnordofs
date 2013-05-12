#ifndef _INODE_H_
#define _INODE_H_

#include <block.h>

typedef enum {
  I_FREE = 0,
  I_FILE,
  I_DIR
} imode_t;

struct inode {
  imode_t mode;
  int size;
  
  block_t direct_blocks[10];
  //  datablock_t *simple_indirect_blocks[64];
  //  datablock_t **double_indirect_blocks[16];
  //  datablock_t ***triple_indirect_blocks[4];
};

typedef struct inode inode_t;

#endif
