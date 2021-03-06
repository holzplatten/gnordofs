/* -*- mode: C -*- Time-stamp: "2013-09-01 15:04:43 holzplatten"
 *
 *       File:         mkfs.gnordofs.c
 *       Author:       Pedro J. Ruiz Lopez (holzplatten@es.gnu.org)
 *       Date:         Sun Jun  1 19:30:53 2013
 *
 *       Archivo principal de mkfs.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>

#include <dir.h>
#include <inode.h>
#include <superblock.h>

int main(int argc, char **argv)
{
  int dev, i;
  char *zeros;
  unsigned long size;
  superblock_t *sb, *sb_dup;
  inode_t *rootdir;

  openlog("GNORDOFS", LOG_PID, LOG_LOCAL0);

  // Size = 10Mib
  size = 1024*1024*10;

  dev = open("gnordofs.img", O_RDWR | O_CREAT, 0666);
  if (dev < 0)
    {
      perror(NULL);
      exit(1);
    }

  /* Inicializar superbloque y zona de inodos. */
  sb = superblock_init(size);
  inode_list_init(dev, sb);
  //superblock_print_dump(sb);

  /* Poner cero toda la zona de bloques, por si acaso. */
  if (lseek(dev, sb->block_zone_base, SEEK_SET) < 0)
    {
      printf("Dude, WTF???\n");
      exit(1);
    }
  zeros = malloc(BLOCK_SIZE);
  memset(zeros, 0, BLOCK_SIZE);
  for (i=sb->block_zone_base;
       i < size - BLOCK_SIZE;
       i += BLOCK_SIZE)
    {
      write(dev, zeros, BLOCK_SIZE);
    }
  if (size > i)
    write(dev, zeros, size - i);

  /* Inicializar lista de bloques libres. */
  free_block_list_init(dev, sb);

  /* ¡Que no se me olvide salvar el maldito superbloque! */
  superblock_write(dev, sb);

  /* A partir de aquí se maneja casi como si estuviese inicializado. */

  /* Reservar el primer inodo libre, marcarlo como directorio,
     añadir las entradas . y .., salvarlo en disco y hacer que
     first_directory del superbloque apunte a dicho inodo.
  */
  rootdir = ialloc(dev, sb);
  // Modificar rootdir con entradas . y ..
  rootdir->type = I_DIR;
  rootdir->perms = S_IFDIR | 0755;
  add_dir_entry(dev, sb, rootdir, rootdir, ".");
  add_dir_entry(dev, sb, rootdir, rootdir, "..");
  rootdir->atime = rootdir->ctime = rootdir->mtime = time(NULL);
  iput(dev, sb, rootdir);

  sb->first_inode = rootdir->n;
  superblock_write(dev, sb);

  printf("rootdir->type = %d\n", rootdir->type);
  printf("rootdir->size = %d\n", rootdir->size);
  printf("rootdir->link_counter = %d\n", rootdir->link_counter);
  printf("rootdir->owner = %d\n", rootdir->owner);
  printf("rootdir->group = %d\n", rootdir->group);
  printf("rootdir->perms = %o\n", rootdir->perms);
  printf("rootdir->n = %d\n", rootdir->n);
  printf("rootdir->offset_ptr = %d\n", rootdir->offset_ptr);

  /* Comprobar que se puede leer el superbloque. */
  sb_dup = superblock_read(dev);
  if (!sb_dup
      || memcmp(sb, sb_dup, sizeof(struct persistent_superblock) != 0))
    {
      printf("WTF?\n");
      exit(1);
    }
  superblock_print_dump(sb_dup);
  print_free_block_list(dev, sb);

  free(sb);
  free(sb_dup);

  close(dev);

  return 0;
}
