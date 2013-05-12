/* -*- mode: C -*- Time-stamp: "2013-05-08 00:03:47 holzplatten"
   *
   *       File:         superblock.c
   *       Author:       Pedro J. Ruiz Lopez (holzplatten@es.gnu.org)
   *       Date:         Sat May  4 11:25:26 2013
   *
   *       Funciones de manejo de superbloque.
   *
   */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <inode.h>
#include <misc.h>
#include <superblock.h>




/*-
  *      Routine:       superblock_write
  *
  *      Purpose:
  *              Escribe a disco un superbloque.
  *      Conditions:
  *              fd debe corresponder a un fichero abierto para escritura.
  *              sb debe apuntar a un superblock_t válido.
  *      Returns:
  *              0 on success.
  *              -1 on error.
  *
  */
int
superblock_write(int fd, const superblock_t * const sb)
{
  int size;

  size = sizeof(struct persistent_superblock);

  if (lseek(fd, 0, SEEK_SET) < 0)
    return -1;

  if (write(fd, sb, size) < size)
    return -1;
  
  return 0;
}




/*-
  *      Routine:       superblock_read
  *
  *      Purpose:
  *              Lee un superbloque de disco.
  *      Conditions:
  *              fd debe corresponder a un fichero abierto para lectura.
  *      Returns:
  *              Un puntero a un superblock_t.
  *              NULL on error.
  *
  */
superblock_t *
superblock_read(int fd)
{
  int size;
  superblock_t *sb;
  
  size = sizeof(struct persistent_superblock);
  sb = (superblock_t *) malloc(sizeof(superblock_t));

  if (!sb)
    return NULL;
  
  if (lseek(fd, 0, SEEK_SET) < 0)
    {
      free(sb);
      return NULL;
    }

  if (read(fd, sb, size) < size)
    {
      free(sb);
      return NULL;
    }
  
  if (sb->magic != MAGIC_NUMBER)
    {
      free(sb);
      return NULL;
    }

  sb->lock = 0;
  sb->modified = 0;
  
  return sb;
}




/*-
  *      Routine:       superblock_init
  *
  *      Purpose:
  *              Genera un nuevo superbloque para un sistema
  *              de archivos gnordofs de tamaño size.
  *      Conditions:
  *              size debe ser mayor que [FIXME] sizeof(struct persistent_superblock) + n_inodos*inodesize + n_inodos*blocksize
  *      Returns:
  *              Un puntero a un superblock_t.
  *              NULL on error.
  *
  */
superblock_t *
superblock_init(unsigned long size)
{
  superblock_t *sb;
  unsigned long block_count, inode_count;
  int i;

  sb = (superblock_t *) malloc(sizeof(superblock_t));
  if (!sb)
    return NULL;

  inode_count = calculate_inode_count(size);
  
  // Coger tamaño. Restar superbloque e inodos.
  size -= sizeof(struct persistent_superblock);
  size -= inode_count * sizeof(inode_t);
  block_count = size / sizeof(block_t);

  /* Se garantiza que el número de bloques será múltiplo entero del tamaño de
     la lista de bloques libres almacenada en el superblock.
  */
  block_count -= block_count % FREE_BLOCK_LIST_SIZE;

  sb->magic2 = sb->magic = MAGIC_NUMBER;

  sb->block_count = block_count;
  sb->free_blocks = block_count;
  for (i=1;
       i <= FREE_BLOCK_LIST_SIZE;
       i++)
    {
      sb->free_block_list[FREE_BLOCK_LIST_SIZE-i] = i-1;
    }
  sb->free_block_index = FREE_BLOCK_LIST_SIZE -1;

  sb->inode_count = inode_count;
  sb->free_inodes = inode_count;
  for (i=1;
       i <= FREE_INODE_LIST_SIZE;
       i++)
    {
      sb->free_inode_list[FREE_INODE_LIST_SIZE-i] = i-1;
    }
  sb->free_inode_index = FREE_INODE_LIST_SIZE -1;

  sb->inode_zone_base = sizeof(struct persistent_superblock);
  sb->block_zone_base = sizeof(struct persistent_superblock)
                            + inode_count * sizeof(inode_t);

  sb->lock = 0;
  sb->modified = 0;

  return sb;
}




/*-
  *      Routine:      superblock_print
  *
  *      Purpose:
  *              Muestra en pantalla un volcado del superbloque.
  *      Conditions:
  *              sb debe apuntar a un superbloque válido.
  *      Returns:
  *              none
  *
  */
void
superblock_print_dump(const superblock_t * const sb)
{
  int i;

  printf("> block_count = %u\n", sb->block_count);
  printf(">\n> free_blocks = %u\n", sb->free_blocks);

  printf("> free_block_list = {");
  for (i=0; i<FREE_BLOCK_LIST_SIZE-1; i++)
    printf("%u,", sb->free_block_list[i]);
  printf("%u}\n", sb->free_block_list[i]);

  printf("> free_block_index = %u\n", sb->free_block_index);
  printf(">\n> inode_count = %u\n", sb->inode_count);
  printf("> free_inodes = %u\n", sb->free_inodes);

  printf("> free_inode_list = {");
  for (i=0; i<FREE_INODE_LIST_SIZE-1; i++)
    printf("%u,", sb->free_inode_list[i]);
  printf("%u}\n", sb->free_inode_list[i]);

  printf("> free_inode_index = %u\n", sb->free_inode_index);
  printf(">\n> inode_zone_base = %u\n", sb->inode_zone_base);
  printf("> block_zone_base = %u\n", sb->block_zone_base);

  printf(">\n> (in core) lock = %s\n", sb->lock ? "Locked" : "Unlocked");
  printf("> (in core) modified = %s\n", sb->modified ? "YES" : "NO");
}
