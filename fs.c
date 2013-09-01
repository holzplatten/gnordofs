/* -*- mode: C -*- Time-stamp: "2013-09-01 15:04:25 holzplatten"
 *
 *       File:         fs.c
 *       Author:       Pedro J. Ruiz Lopez (holzplatten@es.gnu.org)
 *       Date:         Mon Jun 10 22:11:22 2013
 *
 *       Funciones genéricas del sistema de archivos.
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
  along with GnordoFS.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <fcntl.h>
#include <stdlib.h>

#include <fs.h>
#include <misc.h>


/*-
 *      Routine:       do_read
 *
 *      Purpose:
 *              Lee (byte a byte) un buffer de datos de tamaño n a partir
 *              del offset apuntado por el puntero (interno) del inodo.
 *      Conditions:
 *              dev debe corresponder a un gnordofs válido.
 *              sb debe apuntar a un superblock válido.
 *              inode debe ser un inodo de directorio válido.
 *              buffer debe apuntar a un bloque de memoria lo bastante grande.
 *              n debe ser mayor que cero.
 *      Returns:
 *              Un entero con el número de bytes leidos.
 *
 */
int
do_read(int dev, superblock_t *sb, inode_t *inode,
        char *buffer, int n)
{
  int old_blk = -1;
  struct block * datablock = NULL;
  int count=0;
  int byte;
  long blk, absolute_blk;

  while (n>0)
    {
      /* Calcular bloque interno al archivo. */
      blk = inode->offset_ptr / sizeof(struct block);
      /* Calcular offset dentro del bloque. */
      byte = inode->offset_ptr % sizeof(struct block);

      /* Calcular bloque absoluto (todo el fs). */
      absolute_blk = inode_getblk(dev, sb, inode, blk);
      if (absolute_blk == -1)
        return -1;
      
      /* Si el bloque no es el mismo que el de la última lectura, leerlo. */
      if (absolute_blk != old_blk)
        {
          old_blk = absolute_blk;
          if (datablock != NULL)
            free(datablock);

          datablock = getblk(dev, sb, absolute_blk);

          if (!datablock)
            return count;
        }

      /* Copiar byte al buffer de salida. */
      buffer[count] = datablock->data[byte];

      inode->offset_ptr++;
      count++;
      n--;
    }

  if (datablock)
    free(datablock);

  return count;
}




/*-
 *      Routine:       do_lseek
 *
 *      Purpose:
 *              Sitúa el puntero de lectura/escritura en la posición dada.
 *      Conditions:
 *              dev debe corresponder a un gnordofs válido.
 *              sb debe apuntar a un superblock válido.
 *              inode debe ser un inodo de directorio válido.
 *              buffer debe apuntar a un bloque de memoria lo bastante grande.
 *              n debe ser mayor que cero.
 *      Returns:
 *              -1 on error.
 *
 */
int
do_lseek(int dev __attribute__((unused)),
         superblock_t *sb __attribute__((unused)),
         inode_t *inode, off_t offset, int whence)
{
  switch(whence)
    {
    case SEEK_SET:
      inode->offset_ptr = offset;
      break;

    case SEEK_CUR:
      inode->offset_ptr += offset;
      break;

    case SEEK_END:
      inode->offset_ptr = inode->size + offset;
      break;

    default:
      return -1;
    }

  return 0;
}




/*-
 *      Routine:       do_write
 *
 *      Purpose:
 *              Escribe (byte a byte) un buffer de datos de tamaño n a partir
 *              del offset apuntado por el puntero (interno) del inodo.
 *      Conditions:
 *              dev debe corresponder a un gnordofs válido.
 *              sb debe apuntar a un superblock válido.
 *              inode debe ser un inodo de directorio válido.
 *              buffer debe apuntar a un bloque de memoria.
 *              n debe ser mayor que cero y menor o igual que el tamaño de buffer.
 *      Returns:
 *              Un entero con el número de bytes escritos.
 *
 */
int
do_write(int dev, superblock_t *sb, inode_t *inode,
         const char * const buffer, int n)
{
  int old_blk = -1;
  struct block * datablock = NULL;
  int count=0;
  int blk, byte;
  int absolute_blk;

  /* DEBUG_VERBOSE(">> do_write(n=%d)\n", n); */
  /* DEBUG_VERBOSE(">> do_write >> offset_ptr = %d\n", inode->offset_ptr); */

  while (n>0)
    {
      /* Calcular bloque interno al archivo. */
      blk = inode->offset_ptr / sizeof(struct block);
      /* Calcular offset dentro del bloque. */
      byte = inode->offset_ptr % sizeof(struct block);
      /* DEBUG_VERBOSE(">> do_write >> blk = %d, offset = %d\n", blk, byte); */

      /* Calcular bloque absoluto (todo el fs). */
      absolute_blk = inode_getblk(dev, sb, inode, blk);
      if (absolute_blk == -1)
        return -1;
      /* DEBUG_VERBOSE(">> do_write >> absolute_blk = %d\n", absolute_blk); */

      /* Si es un bloque no mapeado todavía, reservar y continuar. */
      if (unassigned_p(absolute_blk))
        {
          /* DEBUG_VERBOSE("do_write -> absolute_blk no mapeado aún\n", absolute_blk); */
          absolute_blk = inode_allocblk(dev, sb, inode, blk);
          /* DEBUG_VERBOSE("do_write -> mapeado bloque %d\n", absolute_blk); */
          if (absolute_blk == -1)
            return -1;
        }
      
      /* Si el bloque no es el mismo que el de la última lectura, primero
         salvar el antiguo (porsiaca) y entonces leer el bloque que toca ahora. */
      if (absolute_blk != old_blk)
        {
          if (datablock != NULL)
            {
              writeblk(dev, sb, old_blk, datablock);
              free(datablock);
            }
          old_blk = absolute_blk;
          /* DEBUG_VERBOSE("getblk con absolute_blk=%d\n", absolute_blk); */
          datablock = getblk(dev, sb, absolute_blk);

          if (!datablock)
            return count;
        }

      /* Escribir byte en el bloque de marras. */
      datablock->data[byte] = buffer[count];

      inode->offset_ptr++;
      count++;
      n--;
    }

  if (datablock == NULL)
    return -1;

  /* Nada de escritura retrasada por ahora. [NHH] */
  writeblk(dev, sb, absolute_blk, datablock);

  free(datablock);

  return count;
}
