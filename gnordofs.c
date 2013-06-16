/* -*- mode: C -*- Time-stamp: "2013-06-16 12:50:42 holzplatten"
 *
 *       File:         gnordofs.c
 *       Author:       Pedro J. Ruiz Lopez (holzplatten@es.gnu.org)
 *       Date:         Mon May 20 20:11:25 2013
 *
 *       Archivo principal de gnordofs.
 *
 */

#define FUSE_USE_VERSION 26

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dir.h>
#include <fs.h>
#include <inode.h>
#include <misc.h>
#include <superblock.h>


static int dev;
static superblock_t *sb;

static int gnordofs_truncate(const char *path, off_t size)
{
  char *p;
  int res;
  inode_t *inode;

  DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> gnordofs_truncate(path = %s, size = %d)\n", path, size);

  p = strdup(path);
  inode = namei(dev, sb, p);
  if (!inode)
    {
      res = -ENOENT;
    }
  else
    {
      inode->size = size;
      iput(dev, sb, inode);
    }

  free(p);

  return res;
}

static int gnordofs_getattr(const char *path, struct stat *stbuf)
{
  inode_t *inode;
  char *p;
  int res = 0;

  DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> gnordofs_getattr(path = %s)\n", path);

  p = strdup(path);
  inode = namei(dev, sb, p);
  if (!inode)
    {
      res = -ENOENT;
    }
  else
    {
      if (inode->type == I_DIR)
        {
          stbuf->st_mode = S_IFDIR | 0755;
          stbuf->st_nlink = inode->link_counter;
        }
      else if (inode->type == I_FILE)
        {
          stbuf->st_mode = S_IFREG | 0644;
          stbuf->st_nlink = inode->link_counter;
          stbuf->st_size = inode->size;
        }
      else
        {
          res = -1;
        }
    }

  free(p);
  return res;
}

static int gnordofs_access(const char *path,
                           int mask __attribute__((unused)))
{
  /* No hay permisos por ahora. Ancha es Castilla. */
  return 0;
}

static int gnordofs_readdir(const char *path,
                            void *buf,
                            fuse_fill_dir_t filler,
                            off_t offset __attribute__((unused)),
                            struct fuse_file_info *fi __attribute__((unused)))
{
  inode_t *inode;
  dir_entry_t *de;
  int i;
  char *p;

  DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> gnordofs_readdir(path = %s)\n", path);

  /* La primera versión de readdir tan sólo soporta el /.
     Menuda gilipollez, ¿no? */
  if (strcmp(path, "/") != 0)
    return -ENOENT;

  p = strdup(path);
  inode = namei(dev, sb, p);
  free(p);
  
  i=1;
  de = get_dir_entry(dev, sb, inode, 0);
  if (!de)
    return -1;

  while (de->inode != -1)
    {
      struct stat status;

      status.st_ino = de->inode;
      status.st_mode = 0777;

      filler(buf, de->name, &status, 0);
      free(de);

      de = get_dir_entry(dev, sb, inode, i++);
      if (!de)
        return -1;
    }
  free(de);

  return 0;
}

static int gnordofs_mknod(const char *path,
                          mode_t mode __attribute__((unused)),
                          dev_t inputdev __attribute__((unused)))
{
  inode_t *inode, *iparent;
  int i;
  char *dirc, *basec, *dname, *bname;

  DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> gnordofs_mknod(path = %s)\n", path);

  dirc = strdup(path);
  basec = strdup(path);

  dname = dirname(dirc);
  bname = basename(basec);

  /* Reservar un nuevo inodo. */
  inode = ialloc(dev, sb);
  if (!inode)
    {
      free(dirc);
      free(basec);
      return -ENOMEM;
    }

  /* base_dir */
  
  iparent = namei(dev, sb, dname);
  if (!iparent)
    {
      free(dirc);
      free(basec);
      return -1;
    }

  if (add_dir_entry(dev, sb, iparent, inode, bname)  != 0)
    {
      free(dirc);
      free(basec);
      return -1;
    }

  inode->type = I_FILE;
  iput(dev, sb, inode);
  DEBUG_VERBOSE("mknod -> (%d) %s\n", inode->n, bname);
  
  free(dirc);
  free(basec);

  return 0;
}

static int gnordofs_open(const char *path, struct fuse_file_info *fi)
{
  DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> gnordofs_open(path = %s)\n", path);
  return 0;
}

static int gnordofs_release(const char *path, struct fuse_file_info *fi)
{
  DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> gnordofs_release(path = %s)\n", path);
  return 0;
}

static int gnordofs_read(const char *path, char *buf, size_t size, off_t offset,
                         struct fuse_file_info *fi __attribute__((unused)))
{
  inode_t *inode;
  int count;
  char *p;

  DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> gnordofs_read(path = %s, size = %d, offset = %d)\n", path, size, offset);

  p = strdup(path);

  inode = namei(dev, sb, p);
  if (!inode)
    return -1;

  free(p);

  if (offset > inode->size)
    {
      free(inode);
      return 0;
    }

  if (offset + size > inode->size)
    size = inode->size - offset;

  do_lseek(dev, sb, inode, offset, SEEK_SET);
  count = do_read(dev, sb, inode, buf, size);

  free(inode);

  return count;
}

static int gnordofs_unlink(const char *path)
{
  char *dirc, *basec, *bname, *dname;
  inode_t *idir, *inode;
  dir_entry_t *de;

  DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> gnordofs_unlink(path = %s)\n", path);

  dirc = strdup(path);
  basec = strdup(path);

  dname = dirname(dirc);
  bname = basename(basec);

  idir = namei(dev, sb, dname);
  if (!idir)
    {
      free(dirc);
      free(basec);

      return -1;
    }

  de = get_dir_entry_by_name(dev, sb, idir, bname);
  if (!de)
    {
      free(idir);
      free(dirc);
      free(basec);

      return -1;
    }

  if (del_dir_entry_by_name(dev, sb, idir, bname) < 0)
    {
      free(de);
      free(idir);
      free(dirc);
      free(basec);

      return -1;
    }

  iput(dev, sb, idir);

  free(idir);
  free(dirc);
  free(basec);

  inode = iget(dev, sb, de->inode);
  if (!inode)
    {
      free(de);
      return -1;
    }

  --(inode->link_counter);
  iput(dev, sb, inode);

  free(inode);
  free(de);

  return 0;

}

static int gnordofs_write(const char *path, const char *buf, size_t size, off_t offset,
                          struct fuse_file_info *fi __attribute__((unused)))
{
  inode_t *inode;
  int count;
  char *p;

  DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> gnordofs_write(path = %s, size = %d, offset = %d)\n", path, size, offset);

  p = strdup(path);

  inode = namei(dev, sb, p);
  if (!inode)
    return -1;

  free(p);

  do_lseek(dev, sb, inode, offset, SEEK_SET);
  count = do_write(dev, sb, inode, buf, size);

  /* Actualizar campo de tamaño si es necesario. */
  if (offset + size > inode->size)
    inode->size = offset + size;

  iput(dev, sb, inode);

  free(inode);

  return count;
}




static struct fuse_operations oper = {
  .access       = gnordofs_access,
  .getattr	= gnordofs_getattr,
  .mknod        = gnordofs_mknod,
  .open		= gnordofs_open,
  .read		= gnordofs_read,
  .readdir	= gnordofs_readdir,
  .release      = gnordofs_release,
  //  .setattr      = gnordofs_setattr,
  .truncate     = gnordofs_truncate,
  .unlink       = gnordofs_unlink,
  .write        = gnordofs_write
};




int main(int argc, char *argv[])
{
  openlog("GNORDOFS", LOG_PID, LOG_LOCAL0);

  dev = open("./bogniga.img", O_RDWR);
  sb = superblock_read(dev);

  return fuse_main(argc, argv, &oper, NULL);
}
