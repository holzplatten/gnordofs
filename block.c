/* -*- mode: C -*- Time-stamp: "2013-06-16 22:26:22 holzplatten"
 *
 *       File:         block.c
 *       Author:       Pedro J. Ruiz Lopez (holzplatten@es.gnu.org)
 *       Date:         Sun Jun  1 19:31:18 2013
 *
 *       Manejo de los bloques de bajo nivel.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <block.h>
#include <inode.h>
#include <superblock.h>




/*-
 *      Routine:       allocblk
 *
 *      Purpose:
 *              Reserva un nuevo bloque de datos.
 *      Conditions:
 *              dev debe corresponder a un gnordofs válido.
 *              sb debe apuntar a un superblock válido.
 *      Returns:
 *              Un entero con el número de bloque absoluto.
 *              -1 on error.
 *
 */
int
allocblk(int dev, superblock_t * const sb)
{
  block_t * buff;
  int block;

  block = sb->free_block_list[sb->free_block_index];

  /* Si la lista parcial de bloques libres sólo contiene un bloque, anotar
     su número y cargar la lista con lo que haya en el bloque al que apunta. */
  if (sb->free_block_index == 0)
    {
      buff = getblk(dev, sb, block);
      if (!buff)
        return -1;

      memcpy(sb->free_block_list, buff, sizeof(sb->free_block_list));
      sb->free_block_index = FREE_BLOCK_LIST_SIZE;

      free(buff);
    }

  sb->free_block_index--;

  return block;
}




/*-
 *      Routine:       freeblk
 *
 *      Purpose:
 *              Añade a la lista de libres un bloque de datos.
 *      Conditions:
 *              dev debe corresponder a un gnordofs válido.
 *              sb debe apuntar a un superblock válido.
 *              block debe apuntar a un bloque absoluto.
 *      Returns:
 *              -1 on error.
 *
 */
int
freeblk(int dev, superblock_t * const sb, int block)
{
  block_t buff;

  sb->free_block_index++;

  /* Si la lista parcial de bloques libres está llena, guardarla en el nuevo
     bloque libre.  */
  if (sb->free_block_index == FREE_BLOCK_LIST_SIZE)
    {
      memset(&buff, 0, sizeof(block_t));
      memcpy(sb->free_block_list, &buff, FREE_BLOCK_LIST_SIZE*sizeof(unsigned long));

      if (writeblk(dev, sb, block, &buff) < 0)
        return -1;
      
      sb->free_block_index = 0;
    }
  else
    {
      sb->free_block_list[sb->free_block_index] = block;
    }

  return 0;
}




/*-
 *      Routine:       getblk
 *
 *      Purpose:
 *              Lee un bloque de datos de disco.
 *      Conditions:
 *              dev debe corresponder a un gnordofs válido.
 *              sb debe apuntar a un superblock válido.
 *              n debe ser un número de bloque no negativo y VÁLIDO.
 *      Returns:
 *              Un bloque de datos.
 *              NULL on error.
 *
 */
block_t *
getblk(int dev, superblock_t *sb, int n)
{
  int offset;
  block_t *datablock;

  if (n<0)
    return NULL;

  offset = sb->block_zone_base + n * sizeof(struct block);

  if (lseek(dev, offset, SEEK_SET) < 0)
    return NULL;

  datablock = malloc(sizeof(struct block));
  if (datablock == NULL)
    return NULL;

  memset(datablock, 0, sizeof(struct block));

  if (read(dev, datablock, sizeof(struct block)) < sizeof(struct block))
    {
      free(datablock);
      return NULL;
    }

  return datablock;
}




/*-
 *      Routine:       writeblk
 *
 *      Purpose:
 *              Escribe un bloque de datos de disco.
 *      Conditions:
 *              dev debe corresponder a un gnordofs válido.
 *              sb debe apuntar a un superblock válido.
 *              n debe ser un número de bloque no negativo y VÁLIDO.
 *              datablock debe apuntar a un block_t válido.
 *      Returns:
 *              -1 on error.
 *
 */
int
writeblk(int dev, superblock_t *sb, int n, block_t *datablock)
{
  int offset;

  if (n<0 || datablock==NULL)
    return -1;

  offset = sb->block_zone_base + n * sizeof(struct block);

  if (lseek(dev, offset, SEEK_SET) < 0)
    return -1;

  if (write(dev, datablock, sizeof(struct block)) < sizeof(struct block))
    return -1;

  return 0;
}




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
int
free_block_list_init(int fd, const superblock_t * const sb)
{
  unsigned int i, acc;
  unsigned int offset, block_count;
  unsigned int sublist[FREE_BLOCK_LIST_SIZE];

  offset = sb->block_zone_base + sb->free_block_list[0] * sizeof(struct block);

  acc = sb->free_block_list[0];
  block_count = sb->block_count;
  while(acc < block_count)
    {
      for (i=1; i<=FREE_BLOCK_LIST_SIZE; i++)
        {
          acc++;
          sublist[FREE_BLOCK_LIST_SIZE-i] = acc;
        }

      lseek(fd, offset , SEEK_SET);
      
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
void
print_free_block_list(int fd, const superblock_t * const sb)
{
  unsigned int i;
  unsigned int offset, block_count, next;
  unsigned int block_zone_base;
  unsigned int sublist[FREE_BLOCK_LIST_SIZE];

  block_zone_base = sb->block_zone_base;

  printf("Superblock free list: ");
  for (i=0; i<FREE_BLOCK_LIST_SIZE/8-1; i++)
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

      for (i=0; i<FREE_BLOCK_LIST_SIZE/8-1; i++)
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
