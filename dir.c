/* -*- mode: C -*- Time-stamp: "2013-06-16 11:54:28 holzplatten"
 *
 *       File:         dir.c
 *       Author:       Pedro J. Ruiz Lopez (holzplatten@es.gnu.org)
 *       Date:         Sun Jun  9 19:31:52 2013
 *
 *       Manejo de inodos de directorio.
 *
 */

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <dir.h>
#include <fs.h>
#include <inode.h>
#include <misc.h>
#include <superblock.h>


/*-
 *      Routine:       add_dir_entry
 *
 *      Purpose:
 *              Añade una nueva entrada en un directorio.
 *              NOTA: Incrementa contador de enlaces pero NO lo salva a disco.
 *      Conditions:
 *              dev debe corresponder a un gnordofs válido.
 *              sb debe apuntar a un superblock válido.
 *              inode debe ser un inodo de directorio válido.
 *      Returns:
 *              -1 on error.
 *
 */
int add_dir_entry(int dev, superblock_t *sb, inode_t *dir_inode, inode_t *entry_inode,
                  const char * const entry_name)
{
  int i;
  dir_entry_t de;

  if (dir_inode->type != I_DIR)
    {
      DEBUG_VERBOSE("UH?");
      return -1;
    }

  do_lseek(dev, sb, dir_inode, 0, SEEK_SET);
  for (i=0; i*sizeof(struct dir_entry) < dir_inode->size; i++)
    {
      DEBUG_VERBOSE("%d\n", i);
      if (do_read(dev, sb, dir_inode, (void *) &de, sizeof(dir_entry_t)) < sizeof(dir_entry_t))
        {
          DEBUG_VERBOSE("Error leyendo entrada número %d del I_DIR %d", i, dir_inode->n);
          return -1;
        }

      /* Si encuentra una entrada libre (-1), dejar de buscar. */
      if (de.inode == -1)
        break;

      /* Estoy completamente seguro de que NO quiero entradas con el mismo nombre. >:( */
      if (strcmp(de.name, entry_name) == 0)
        return -1;
    }

  DEBUG_VERBOSE(">> add_dir_entry -> añadiendo entrada en posición %d\n", i);
  de.inode = entry_inode->n;
  strcpy(de.name, entry_name);

  do_lseek(dev, sb, dir_inode, i*sizeof(dir_entry_t), SEEK_SET);
  if (do_write(dev, sb, dir_inode, (void *) &de, sizeof(dir_entry_t))
                                                           < sizeof(dir_entry_t))
    return -1;

  dir_inode->size += sizeof(struct dir_entry);
  if (iput(dev, sb, dir_inode) != 0)
    return -1;

  /* Ya sólo falta incrementar en 1 el número de enlaces del inodo apuntado por la
     entrada que se acaba de insertar.
  */
  entry_inode->link_counter++;
  
  return 0;
}




/*-
 *      Routine:       del_dir_entry_by_name
 *
 *      Purpose:
 *              Elimina una entrada en un directorio según su nombre.
 *              NOTA: Decrementa el tamaño del directorio pero NO lo salva a disco.
 *              NOTA: NO decrementa el número de enlaces al inodo referenciado.
 *      Conditions:
 *              dev debe corresponder a un gnordofs válido.
 *              sb debe apuntar a un superblock válido.
 *              inode debe ser un inodo de directorio válido.
 *      Returns:
 *              -1 on error.
 *
 */
int del_dir_entry_by_name(int dev, superblock_t *sb, inode_t *inode,
                          const char * const entry_name)
{
  int i, found;
  dir_entry_t de;

  DEBUG_VERBOSE(">> del_dir_entry_by_name(entry_name = %s)\n", entry_name);

  if (inode->type != I_DIR)
    {
      DEBUG_VERBOSE("no es un directorio!\n");
      return -1;
    }

  do_lseek(dev, sb, inode, 0, SEEK_SET);
  i = 0;
  do {
    DEBUG_VERBOSE(">> del_dir_entry >> i=%d\n", i);
    if (do_read(dev, sb, inode, (void *) &de, sizeof(dir_entry_t))
                                                              < sizeof(dir_entry_t))
      {
        DEBUG_VERBOSE("Error leyendo entrada número %d del I_DIR %d", i, inode->n);
        return -1;
      }
    
    /* Las entradas libres no cuentan como el espacio ocupado. */
    if (de.inode == -1)
      continue;
    i++;

    found = strcmp(de.name, entry_name) == 0;

  } while (!found &&  i*sizeof(struct dir_entry) < inode->size);

  /* Fuera de rango. */
  if (!found)
    return -1;

  de.inode = -1;
  /* Un pasito pa'trás... */
  do_lseek(dev, sb, inode, -sizeof(dir_entry_t), SEEK_CUR);
  if (do_write(dev, sb, inode, (void *) &de, sizeof(dir_entry_t))
                                                           < sizeof(dir_entry_t))
    return -1;

  inode->size -= sizeof(dir_entry_t);
  
  return 0;
}




/*-
 *      Routine:       get_dir_entry
 *
 *      Purpose:
 *              Recupera una entrada de directorio
 *      Conditions:
 *              dev debe corresponder a un gnordofs válido.
 *              sb debe apuntar a un superblock válido.
 *              inode debe ser un inodo de directorio válido.
 *      Returns:
 *              NULL on error.
 *
 */
dir_entry_t * get_dir_entry(int dev, superblock_t *sb, inode_t *inode, int n)
{
  int i;
  dir_entry_t de = { -1, "FIN" };
  dir_entry_t *de_n;

  DEBUG_VERBOSE(">> get_dir_entry\n");

  if (inode->type != I_DIR)
    {
      DEBUG_VERBOSE("no es un directorio!\n");
      return NULL;
    }

  do_lseek(dev, sb, inode, 0, SEEK_SET);
  i = 0;
  do {
    DEBUG_VERBOSE(">> get_dir_entry >> i=%d\n", i);
    if (do_read(dev, sb, inode, (void *) &de, sizeof(dir_entry_t)) < sizeof(dir_entry_t))
      {
        DEBUG_VERBOSE("Error leyendo entrada número %d del I_DIR %d", i, inode->n);
        return NULL;
      }
    
    /* Las entradas libres no cuentan como el espacio ocupado. */
    if (de.inode == -1)
      continue;
    i++;

  } while (i <= n  &&  i*sizeof(struct dir_entry) < inode->size);

  de_n = malloc(sizeof(struct dir_entry));

  /* Fuera de rango. */
  if (i <= n)
    de_n->inode = -1;
  else
    memcpy(de_n, &de, sizeof(struct dir_entry));

  return de_n;
}




/*-
 *      Routine:       get_dir_entry_by_name
 *
 *      Purpose:
 *              Recupera una entrada de directorio según su nombre
 *      Conditions:
 *              dev debe corresponder a un gnordofs válido.
 *              sb debe apuntar a un superblock válido.
 *              inode debe ser un inodo de directorio válido.
 *      Returns:
 *              NULL on error.
 *
 */
dir_entry_t * get_dir_entry_by_name(int dev, superblock_t *sb, inode_t *inode, char * name)
{
  int i, found;
  dir_entry_t de = { -1, "FIN" };
  dir_entry_t *de_n;

  DEBUG_VERBOSE(">> get_dir_entry_by_name\n");

  if (inode->type != I_DIR)
    {
      DEBUG_VERBOSE("no es un directorio!\n");
      return NULL;
    }

  do_lseek(dev, sb, inode, 0, SEEK_SET);
  i = 0;
  do {
    DEBUG_VERBOSE(">> get_dir_entry >> i=%d\n", i);
    if (do_read(dev, sb, inode, (void *) &de, sizeof(dir_entry_t)) < sizeof(dir_entry_t))
      {
        DEBUG_VERBOSE("Error leyendo entrada número %d del I_DIR %d", i, inode->n);
        return NULL;
      }
    
    /* Las entradas libres no cuentan como el espacio ocupado. */
    if (de.inode == -1)
      continue;
    i++;

    found = strcmp(de.name, name) == 0;

  } while (!found &&  i*sizeof(struct dir_entry) < inode->size);

  de_n = malloc(sizeof(struct dir_entry));

  /* Fuera de rango. */
  if (!found)
    de_n->inode = -1;
  else
    memcpy(de_n, &de, sizeof(struct dir_entry));

  return de_n;
}
