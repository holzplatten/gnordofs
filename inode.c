#include <fcntl.h>

#include <inode.h>
#include <misc.h>
#include <superblock.h>

/*
  ialloc
  ifree

  iget
  iput
  bmap
 */

/*-
  *      Routine:       inode_list_init
  *
  *      Purpose:
  *              Inicializa la lista de inodos.
  *      Conditions:
  *              fd debe corresponder a un fichero abierto para escritura.
  *              sb debe apuntar a un superblock_t inicializado.
  *      Returns:
  *              0 on success.
  *              -1 on error.
  *
  */
int inode_list_init(int fd, const superblock_t * const sb)
{
  unsigned int i, last_inode;
  inode_t idummy;
  
  lseek(fd, sb->inode_zone_base, SEEK_SET);
  
  idummy.mode = I_FREE;
  last_inode = sb->inode_count;
  for (i=0; i < last_inode; i++)
    {
      if (write(fd, &idummy, sizeof(struct inode)) < sizeof(struct inode))
        return -1;
    }

  return 0;
}
