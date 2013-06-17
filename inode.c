/* -*- mode: C -*- Time-stamp: "2013-06-17 11:42:58 holzplatten"
 *
 *       File:         inode.c
 *       Author:       Pedro J. Ruiz Lopez (holzplatten@es.gnu.org)
 *       Date:         Sun Jun  1 19:29:47 2013
 *
 *       Funciones para manejo de inodos.
 *
 */

/*
  Copyright (C) 2013 Pedro J. Ruiz López <holzplatten@es.gnu.org>

  This file is part of GnordoFS.

  GnordoFS is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  GnordoFS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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
inode_t *
namei(int dev, superblock_t * const sb, char * path)
{
  char *p;
  inode_t *inode;
  dir_entry_t *de;

  DEBUG_VERBOSE(">> namei(%s)", path);

  /* En FUSE, todas las rutas son absolutas, así que comenzamos cogiendo
     el inodo de / y actualizando path para que apunte al primer caracter
     de la ruta relativo a /. */
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
inode_t *
iget(int dev, const superblock_t * const sb, int n)
{
  /* int i; */
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

  /* DEBUG_VERBOSE(">>>> n = %d", inode->n); */
  /* DEBUG_VERBOSE(">>>> type = %x", inode->type); */
  /* DEBUG_VERBOSE(">>>> size = %d", inode->size); */
  /* DEBUG_VERBOSE(">>>> direct_blocks = {"); */
  /* for (i=0; i<10; i++) */
  /*   DEBUG_VERBOSE("\t%d", inode->direct_blocks[i]); */
  /* DEBUG_VERBOSE("}\n"); */

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
int
iput(int dev, const superblock_t * const sb, inode_t * inode)
{
  /* int i; */

  if (!inode)
    return -1;

  DEBUG_VERBOSE(">> iput()\n");
  
  lseek(dev, sb->inode_zone_base + inode->n * sizeof(struct inode), SEEK_SET);

  if (write(dev, inode, sizeof(struct inode)) < sizeof(struct inode))
    return -1;

  /* DEBUG_VERBOSE(">>>> n = %d", inode->n); */
  /* DEBUG_VERBOSE(">>>> type = %x", inode->type); */
  /* DEBUG_VERBOSE(">>>> size = %d", inode->size); */
  /* DEBUG_VERBOSE(">>>> direct_blocks = {"); */
  /* for (i=0; i<10; i++) */
  /*   DEBUG_VERBOSE("\t%d", inode->direct_blocks[i]); */
  /* DEBUG_VERBOSE("}"); */

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
inode_t *
ialloc(int dev, superblock_t * const sb)
{
  inode_t *inode;
  unsigned long in, i, j, *aux_list;

  DEBUG_VERBOSE(">> ialloc\n");

  if (!sb->free_inodes)
    {
      DEBUG_VERBOSE(">>>> NO QUEDAN INODOS LIBRES!\n");
      return NULL;
    }

  /* Si la lista está vacía, recorrer la zona de inodos en busca de inodos libres. */
  if (sb->free_inode_index == 0)
    {
      DEBUG_VERBOSE(">> ialloc >> Lista de inodos libres vacía. Rellenando...\n");

      aux_list = malloc(FREE_INODE_LIST_SIZE * sizeof(unsigned long));

      in = sb->free_inode_list[0];
      /* Comienza por el último que se asignó, que probablemente haya más después de él. */
      for (j = 0, i = in;
           j < FREE_INODE_LIST_SIZE && i < sb->inode_count;
           i++)
        {
          inode = iget(dev, sb, i);
          if (!inode)
            {
              DEBUG_VERBOSE(">> ialloc >> ERROR al rellenar la lista de inodos libres!\n");
              return NULL;
            }
          
          if (inode->type == I_FREE)
            {
              aux_list[j++] = i;
            }

          free(inode);
        }

      /* Si no se llenó con los que había del final, recorrer la lista hasta el último
         que se asignó. */
      for (i=0;
           j < FREE_INODE_LIST_SIZE && i < in;
           i++)
        {
          inode = iget(dev, sb, i);
          if (!inode)
            {
              DEBUG_VERBOSE(">> ialloc >> ERROR al rellenar la lista de inodos libres!\n");
              return NULL;
            }
          
          if (inode->type = I_FREE)
            {
              aux_list[j++] = i;
            }

          free(inode);
        }

      /* Sería un detalle que free_inode_index indicase cuántos inodos hay anotados en
         la dichosa lista. (¡¬¬) */
      sb->free_inode_index = j;
      /* Anotar en el superblock la lista en orden inverso. */
      for (i=0, j--; i <= j; i++)
        sb->free_inode_list[j-i] = aux_list[i];

      free(aux_list);

      DEBUG_VERBOSE(">> ialloc >> Lista de inodos libres con %d nuevas entradas...\n",
            sb->free_inode_index);
    }

  /* Obtener primer inodo libre de la lista de idem. */
  sb->free_inode_index--;
  in = sb->free_inode_list[sb->free_inode_index];

  inode = iget(dev, sb, in);

  if (inode)
    {
      /* Decrementar contador de inodos libres. */
      sb->free_inodes--;

      /* Marcar como no asignados cada uno de los elementos de la lista de bloques. */
      for (i=0; i<10; i++)
        inode->direct_blocks[i] = BLK_UNASSIGNED;
    }

  DEBUG_VERBOSE(">> ialloc >> inode = %d\n", inode->n);
  DEBUG_VERBOSE(">> ialloc >> sb->free_inode_index = %d\n", sb->free_inode_index);
  DEBUG_VERBOSE(">> ialloc >> sb->free_inodes = %d\n", sb->free_inodes);

  return inode;
}




/*-
 *      Routine:       ifree
 *
 *      Purpose:
 *              Libera un inodo y lo añade a la lista de inodos libres.
 *              Si este inodo referencia a algún bloque de datos, también
 *              lo libera y lo añade a su lista correspondiente
 *      Conditions:
 *              dev debe corresponder a un gnordofs válido.
 *              sb debe apuntar a un superbloque válido.
 *              inode debe apuntar a un inodo.
 *      Returns:
 *              -1 on error.
 *
 */
int
ifree(int dev, superblock_t * const sb, inode_t *inode)
{
  int i, block;

  DEBUG_VERBOSE(">> ifree(inode->n = %d)\n", inode->n);

  for (i=0; i < 10; i++)
    {
      block = inode_getblk(inode, i);
      if (!unassigned_p(block))
        freeblk(dev, sb, block);
    }

  if (sb->free_inode_index == FREE_INODE_LIST_SIZE)
    {
      DEBUG_VERBOSE(">> ialloc >> Lista de inodos libres llena...");

      /* Si la lista está vacía y el nuevo inodo libre es anterior al
         primero de esta lista, insertarlo (reemplazando) en dicha posición. */
      if (sb->free_inode_list[0] > inode->n)
        {
          DEBUG_VERBOSE(" reemplazando primera entrada.");
          sb->free_inode_list[0] = inode->n;
        }
    }
  else
    {
      /* Si no, añadirlo a ella e incrementar el índice. */
      sb->free_inode_list[sb->free_inode_index] = inode->n;
      sb->free_inode_index++;
    }

  inode->type = I_FREE;
  iput(dev, sb, inode);

  /* Incrementar contador de inodos libres. */
  sb->free_inodes++;

  return 0;
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
int
inode_getblk(inode_t *inode, int blk)
{
  /* DEBUG_VERBOSE(">> inode_getblk(blk = %d)\n", blk); */

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
int
inode_allocblk(int dev, superblock_t * const sb,
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
 *      Routine:       inode_freeblk
 *
 *      Purpose:
 *              Desasigna un bloque de la lista de bloques de un inodo.
 *      Conditions:
 *              inode debe apuntar a un inodo válido.
 *              blk debe ser un valor no negativo y dentro del rango permitido.
 *      Returns:
 *              -1 on error
 *
 */
int
inode_freeblk(inode_t *inode, int blk)
{
  /* DEBUG_VERBOSE(">> inode_getblk(blk = %d)\n", blk); */

  if (blk < 0)
    return -1;

  if (blk < 10)
    {
      if (unassigned_p(inode->direct_blocks[blk]))
        return -1;

      inode->direct_blocks[blk] = BLK_UNASSIGNED;
      return 0;
    }

  return -1;
}




/*-
 *      Routine:       inode_truncate
 *
 *      Purpose:
 *              Trunca un archivo a partir de su inodo.
 *      Conditions:
 *              dev debe corresponder a un gnordofs válido.
 *              sb debe apuntar a un superbloque válido.
 *              inode debe apuntar a un inodo válido.
 *              size debe ser no-negativo.
 *      Returns:
 *              -1 on error.
 *
 */
int
inode_truncate(int dev, superblock_t * const sb, inode_t *inode, int size)
{
  int blk, last_blk;
  int absolute_blk;

  if (size < 0)
    return -1;

  /* Para truncar al alza, tan sólo hay que tocar el campo size del inodo. */
  if (size >= inode->size)
    {
      if (size > inode->size)
        inode->size = size;

      return 0;
    }

  /* Para truncar de toda la vida, hay que liberar los bloques que quedan fuera
     tras meter las tijeras. */

  /* Calcular último bloque interno que hay que liberar. */
  last_blk = (inode->size-1) / sizeof(struct block);
  /* Calcular primer bloque interno que hay que liberar. */
  blk = size  ?  (size-1)/sizeof(struct block)  :  0;
  while (blk <= last_blk)
    {
      absolute_blk = inode_getblk(inode, blk);
      if (absolute_blk == -1)
        return -1;

      if (!unassigned_p(absolute_blk))
        {
          if (freeblk(dev, sb, absolute_blk) < 0)
            return -1;

          inode_freeblk(inode, blk);
        }

      blk++;
    }

  inode->size = size;

  return 0;
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
int
inode_list_init(int fd, const superblock_t * const sb)
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
