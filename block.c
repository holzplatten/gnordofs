#include <fcntl.h>
#include <stdio.h>

#include <block.h>
#include <inode.h>
#include <superblock.h>

/* 
   alloc
   free

   getblk
   brelse
   bread
   bwrite
*/

/*-
  *      Routine:       free_block_list_init
  *
  *      Purpose:
  *              Inicializa una lista de inodos.
  *      Conditions:
  *              fd debe corresponder a un fichero abierto para escritura.
  *              sb debe apuntar a un superblock_t inicializado.
  *      Returns:
  *              0 on success.
  *              -1 on error.
  *
  */
int free_block_list_init(int fd, const superblock_t * const sb)
{
  unsigned int i, acc;
  unsigned int offset, block_count;
  unsigned int sublist[FREE_BLOCK_LIST_SIZE];

  offset = sb->block_zone_base + sb->free_block_list[0] * sizeof(struct block);

  acc = sb->free_block_list[0];
  block_count = sb->block_count;
  while(acc < block_count)
    {
      for (i=1; i<=FREE_BLOCK_LIST_SIZE; ++i)
        {
          ++acc;
          sublist[FREE_BLOCK_LIST_SIZE-i] = acc;
        }

      lseek(fd, offset , SEEK_SET);
      
      /* printf("ESCRIBIENDO BLOQUE %u (%x): ", acc, offset); */
      /* printf(" -> %u: ", acc); */
      /* for (i=0; i<FREE_BLOCK_LIST_SIZE/8-1; ++i) */
      /*   { */
      /*     printf("%u,", sublist[i]); */
      /*   } */
      /* printf("%u...\n", sublist[i]); */

      if (write(fd, &sublist, sizeof(sublist)) < sizeof(sublist))
        return -1;

      offset += FREE_BLOCK_LIST_SIZE * sizeof(struct block);
    }

  return 0;
}




/*-
  *      Routine:       print_free_block_list
  *
  *      Purpose:
  *              Muestra en pantalla un volcado de la lista de nodos libres.
  *      Conditions:
  *              fd debe corresponder a un fichero abierto para lectura y
  *              que corresponda a un sistema de archivos inicializado.
  *              sb debe apuntar al superblock_t correspondiente al sistema
  *              de archivos de fd.
  *      Returns:
  *              none
  *
  */
void print_free_block_list(int fd, const superblock_t * const sb)
{
  unsigned int i;
  unsigned int offset, block_count, next;
  unsigned int block_zone_base;
  unsigned int sublist[FREE_BLOCK_LIST_SIZE];

  block_zone_base = sb->block_zone_base;

  printf("Superblock free list: ");
  for (i=0; i<FREE_BLOCK_LIST_SIZE/8-1; ++i)
    {
      printf("%d,", sb->free_block_list[i]);
    }
  printf("%d...\n", sb->free_block_list[i]);

  block_count = sb->block_count - FREE_BLOCK_LIST_SIZE;
  next = sb->free_block_list[0];

  offset = block_zone_base + next * sizeof(struct block);
  while (block_count > 0)
    {
      printf("LEYENDO BLOQUE %u (0x%x): ", next, offset );

      lseek(fd, offset, SEEK_SET);
      if (read(fd, &sublist, sizeof(sublist)) < sizeof(sublist))
        {
          printf("########## Error! #########\n");
          return;
        }

      for (i=0; i<FREE_BLOCK_LIST_SIZE/8-1; ++i)
        {
          printf("%u,", sublist[i]);
        }
      printf("%u...\n", sublist[i]);

      next = sublist[0];
      offset = block_zone_base + next * sizeof(struct block);
      block_count -= FREE_BLOCK_LIST_SIZE;
    }

  printf("End of list\n");
}
