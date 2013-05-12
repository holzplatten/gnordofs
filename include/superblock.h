#ifndef _SUPERBLOCK_H_
#define _SUPERBLOCK_H_

/* 
 * Estructura del superbloque en disco:
 *  - datablock_count (4 bytes)
 * 
 *  - free_datablocks (4 bytes)
 *  - free_datablock_list (4 bytes * FREE_DATABLOCK_LIST_SIZE)
 *  - free_datablock_index (2 bytes)
 * 
 *  - inode_list_size (4 bytes)
 *  - free_inode_count (4 bytes)
 *  - free_inode_list (4 bytes * FREE_INODE_LIST_SIZE)
 *  - free_inode_index (2 bytes)
 * 
 *  - block_zone_base (4 bytes)
 *  - inode_zone_base (4 bytes)
 * 
 */

#define MAGIC_NUMBER 0xCACA

#define FREE_INODE_LIST_SIZE 16
#define FREE_BLOCK_LIST_SIZE 64

#define SUPERBLOCK_PERSISTENT_DATA                                      \
  unsigned short magic;                                                 \
  /* Número de bloques de datos (Tamaño) */                             \
  unsigned long block_count;                                            \
  /* */                                                                 \
  unsigned long free_blocks;                                            \
  unsigned long free_block_list[FREE_BLOCK_LIST_SIZE];                  \
  unsigned short free_block_index;                                      \
  /* Número de inodos */                                                \
  unsigned long inode_count;                                            \
  /* */                                                                 \
  unsigned long free_inodes;                                            \
  unsigned long free_inode_list[FREE_INODE_LIST_SIZE];                  \
  unsigned short free_inode_index;                                      \
                                                                        \
  unsigned long inode_zone_base;                                        \
  unsigned long block_zone_base; unsigned short magic2;
  
struct superblock
{
  SUPERBLOCK_PERSISTENT_DATA
  
  char lock;	/* ¿Mejor usar un pthread_spinlock_t? */
  char modified;
};

typedef struct superblock superblock_t;

struct persistent_superblock
{
  SUPERBLOCK_PERSISTENT_DATA
};

int superblock_write(int fd, const superblock_t * const sb);
superblock_t * superblock_read(int fd);
superblock_t * superblock_init(unsigned long size);

void superblock_print_dump(const superblock_t * const sb);

#endif
