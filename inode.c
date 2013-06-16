/* -*- mode: C -*- Time-stamp: "2013-06-16 11:51:42 holzplatten"
 *
 *       File:         inode.c
 *       Author:       Pedro J. Ruiz Lopez (holzplatten@es.gnu.org)
 *       Date:         Sun Jun  1 19:29:47 2013
 *
 *       Funciones para manejo de inodos.
 *
 */

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include <dir.h>
#include <inode.h>
#include <misc.h>
#include <superblock.h>


/*-
 *      Routine:       namei
 *
 *      Purpose:
 *              Obtiene el inodo correspondiente a un path.
 *      Conditions:
 *              dev debe corresponder a un gnordofs válido.
 *              sb debe apuntar a un superbloque válido.
 *              El path debe ser válido.
 *      Returns:
 *              Un puntero al inodo ya bloqueado.
 *              NULL on error.
 *
 */
inode_t * namei(int dev, superblock_t * const sb, char * path)
{
  char *p;
  inode_t *inode;
  dir_entry_t *de;

  DEBUG_VERBOSE(">> namei(%s)", path);

  /* En FUSE, todas las rutas son absolutas, así que comenzamos cogiendo
     el inodo de / y actualizando path para que apunte al primer caracter
     de la ruta relativo a /.
  */
  inode = iget(dev, sb, sb->first_inode);

  /* Caso especial de haber pedido el /. */
  if (strcmp(path, "/") == 0)
    return inode;

  path++;
  p = strchr(path, '/');
  while (p)
    {
      /* Marcar fin de string en path. */
      *p = 0;

      /* Sacar dirent del subdirectorio objetivo. */
      de = get_dir_entry_by_name(dev, sb, inode, path);
      free(inode);
      if (!de || de->inode < 0)
        {
          free(de);
          return NULL;
        }
      /* Abrir inodo del subdirectorio objetivo. */
      inode = iget(dev, sb, de->inode);
      free(de);

      path = p+1;
      p = strchr(path, '/');
    }

  /* Sacar dirent del subdirectorio objetivo. */
  de = get_dir_entry_by_name(dev, sb, inode, path);
  free(inode);
  if (!de || de->inode < 0)
    {
      free(de);
      return NULL;
    }

  /* Abrir inodo del subdirectorio objetivo. */
  inode = iget(dev, sb, de->inode);
  free(de);

  return inode;
}




/*-
 *      Routine:       iget
 *
 *      Purpose:
 *              Obtiene el inodo correspondiente a un número de inodo.
 *      Conditions:
 *              dev debe corresponder a un gnordofs válido.
 *              sb debe apuntar a un superbloque válido.
 *              n debe ser un numero de inodo válido.
 *      Returns:
 *              Un puntero al inodo ya bloqueado.
 *              NULL on error.
 *
 */
inode_t * iget(int dev, const superblock_t * const sb, int n)
{
  int i;
  inode_t *inode;

  DEBUG_VERBOSE(">> iget(%d)", n);

  if (n < 0 || n >= sb->inode_count)
    return NULL;
  
  lseek(dev, sb->inode_zone_base + n * sizeof(struct inode), SEEK_SET);

  inode = malloc(sizeof(struct inode));
  if (!inode)
    return NULL;

  if (read(dev, inode, sizeof(struct inode)) < sizeof(struct inode))
    {
      free(inode);
      return NULL;
    }

  DEBUG_VERBOSE(">>>> n = %d", inode->n);
  DEBUG_VERBOSE(">>>> type = %x", inode->type);
  DEBUG_VERBOSE(">>>> size = %d", inode->size);
  DEBUG_VERBOSE(">>>> direct_blocks = {");
  for (i=0; i<10; i++)
    DEBUG_VERBOSE("\t%d", inode->direct_blocks[i]);
  DEBUG_VERBOSE("}\n");

  return inode;
}




/*-
 *      Routine:       iput
 *
 *      Purpose:
 *              Guarda en disco un inodo.
 *      Conditions:
 *              dev debe corresponder a un gnordofs válido.
 *              sb debe apuntar a un superbloque válido.
 *              inode debe apuntar a un inode_t válido.
 *      Returns:
 *              0 on success.
 *              -1 on error.
 *
 */
int iput(int dev, const superblock_t * const sb, inode_t * inode)
{
  int i;

  if (!inode)
    return -1;

  DEBUG_VERBOSE(">> iput()\n");
  
  lseek(dev, sb->inode_zone_base + inode->n * sizeof(struct inode), SEEK_SET);

  if (write(dev, inode, sizeof(struct inode)) < sizeof(struct inode))
    return -1;

  DEBUG_VERBOSE(">>>> n = %d", inode->n);
  DEBUG_VERBOSE(">>>> type = %x", inode->type);
  DEBUG_VERBOSE(">>>> size = %d", inode->size);
  DEBUG_VERBOSE(">>>> direct_blocks = {");
  for (i=0; i<10; i++)
    DEBUG_VERBOSE("\t%d", inode->direct_blocks[i]);
  DEBUG_VERBOSE("}");

  return 0;
}




/*-
 *      Routine:       ialloc
 *
 *      Purpose:
 *              Asigna un nuevo inodo de la lista de inodos libres.
 *      Conditions:
 *              dev debe corresponder a un gnordofs válido.
 *              sb debe apuntar a un superbloque válido.
 *      Returns:
 *              Un puntero al inodo ya bloqueado.
 *              NULL on error.
 *
 */
inode_t * ialloc(int dev, superblock_t * const sb)
{
  inode_t *inode;
  unsigned long in, i, j;

  DEBUG_VERBOSE(">> ialloc\n");

  if (!sb->free_inodes)
    {
      DEBUG_VERBOSE(">>>> NO QUEDAN INODOS LIBRES!\n");
      return NULL;
    }

  /* Si la lista está vacía, recorrer la zona de inodos en busca de inodos libres. */
  if (sb->free_inode_index == 0)
    {
      DEBUG_SHIT(">> ialloc >> Lista de inodos libres vacía. Rellenando...\n");

      in = sb->free_inode_list[0];
      /* Comienza por el último que se asignó, que probablemente haya más después de él. */
      for (j = 0, i = in;
           j < FREE_INODE_LIST_SIZE && i < sb->inode_count;
           i++)
        {
          inode = iget(dev, sb, i);
          if (!inode)
            {
              DEBUG_SHIT(">> ialloc >> ERROR al rellenar la lista de inodos libres!\n");
              return NULL;
            }
          
          if (inode->type == I_FREE)
            {
              sb->free_inode_list[j++] = i;
            }

          free(inode);
        }

      /* Si no se llenó con los que había del final, recorrer la lista hasta el último
         que se asignó
      */
      for (i=0;
           j < FREE_INODE_LIST_SIZE && i < in;
           i++)
        {
          inode = iget(dev, sb, i);
          if (!inode)
            {
              DEBUG_SHIT(">> ialloc >> ERROR al rellenar la lista de inodos libres!\n");
              return NULL;
            }
          
          if (inode->type = I_FREE)
            {
              sb->free_inode_list[j++] = i;
            }

          free(inode);
        }

      /* Sería un detalle que free_inode_index indicase cuántos inodos hay anotados en
         la dichosa lista. (¡¬¬)
      */
      sb->free_inode_index = j;

      DEBUG_SHIT(">> ialloc >> Lista de inodos libres con %d nuevas entradas...\n",
            sb->free_inode_index);
    }

  /* Obtener primer inodo libre de la lista de idem. */
  sb->free_inode_index--;
  in = sb->free_inode_list[sb->free_inode_index];

  inode = iget(dev, sb, in);

  if (inode)
    {
      /* Decrementar contador de inodos libres. */
      --(sb->free_inodes);

      /* Marcar como no asignados cada uno de los elementos de la lista de bloques. */
      for (i=0; i<10; i++)
        inode->direct_blocks[i] = BLK_UNASSIGNED;
    }

  DEBUG_VERBOSE(">> ialloc >> inode = %x\n", inode);

  return inode;
}




/*-
 *      Routine:       inode_getblk
 *
 *      Purpose:
 *              Devuelve el número de bloque absoluto a partir de
 *              un número de bloque relativo a un archivo
 *      Conditions:
 *              inode debe apuntar a un inodo válido.
 *              blk debe ser un valor no negativo y dentro del rango permitido.
 *      Returns:
 *              El número de bloque absoluto.
 *              -1 on error
 *
 */
int inode_getblk(inode_t *inode, int blk)
{
  DEBUG_VERBOSE(">> inode_getblk\n");
  DEBUG_VERBOSE(">> inode_getblk >> blk = %d\n", blk);

  if (blk < 0)
    return -1;

  if (blk < 10)
    return inode->direct_blocks[blk];

  return -1;
}




/*-
 *      Routine:       inode_allocblk
 *
 *      Purpose:
 *              Reserva un bloque de datos para el inodo dado.
 *      Conditions:
 *              dev debe corresponder a un gnordofs válido.
 *              sb debe apuntar a un superbloque válido.
 *              inode debe apuntar a un inodo válido.
 *              blk debe ser un valor no negativo y dentro del rango permitido.
 *      Returns:
 *              El número de bloque absoluto.
 *              -1 on error
 *
 */
int inode_allocblk(int dev, superblock_t * const sb,
                   inode_t * inode, int blk)
{
  int ablk;

  if (blk < 0 | blk >= 10)
    return -1;

  ablk = allocblk(dev, sb);

  if (blk < 10)
    inode->direct_blocks[blk] = ablk;

  return ablk;
}




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
  
  memset(&idummy, 0, sizeof(struct inode));

  last_inode = sb->inode_count;
  for (i=0; i < last_inode; i++)
    {
      if (write(fd, &idummy, sizeof(struct inode)) < sizeof(struct inode))
        return -1;
      idummy.n++;
    }

  return 0;
}
