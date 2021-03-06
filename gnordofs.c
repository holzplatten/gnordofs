/* -*- mode: C -*- Time-stamp: "2013-09-01 15:04:06 holzplatten"
 *
 *       File:         gnordofs.c
 *       Author:       Pedro J. Ruiz Lopez (holzplatten@es.gnu.org)
 *       Date:         Mon May 20 20:11:25 2013
 *
 *       Archivo principal de gnordofs.
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


#define FUSE_USE_VERSION 26

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <dir.h>
#include <fs.h>
#include <inode.h>
#include <misc.h>
#include <superblock.h>


static int dev;
static superblock_t *sb;

static int gnordofs_access(const char *path,
                           int mask)
{
  inode_t *inode;
  char *p;
  struct fuse_context * ctxt;

  DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> gnordofs_access(path = %s, mask = %o)\n", path, mask);

  p = strdup(path);
  inode = namei(dev, sb, p);
  free(p);
  if (!inode)
    {
      free(p);
      return -ENOENT;
    }

  ctxt = fuse_get_context();
  if (ctxt->uid == 0)
    {
      free(inode);
      return 0;
    }

  if (mask & X_OK == X_OK)
    if ((inode->perms & (S_IXUSR | S_IXGRP | S_IXOTH)) == 0)
      return -EACCES;

  if (mask & W_OK == W_OK)
    if ((inode->perms & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0)
      return -EACCES;

  if (mask & R_OK == R_OK)
    if ((inode->perms & (S_IRUSR | S_IRGRP | S_IROTH)) == 0)
      return -EACCES;

  if (inode->perms == 0)
    return -EACCES;

  return 0;
}

static int gnordofs_chmod(const char *path, mode_t mode)
{
  inode_t *inode;
  char *p;
  int res = 0;

  DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> gnordofs_chmod(path = %s, mode = %o)\n", path, mode);

  p = strdup(path);
  inode = namei(dev, sb, p);
  free(p);
  if (!inode)
    {
      res = -ENOENT;
    }
  else if (can_write_p(inode))
    {
      inode->perms = mode;
      inode->ctime = time(NULL);
      iput(dev, sb, inode);
      free(inode);
    }
  else
    {
      res = -EACCES;
      free(inode);
    }

  return res;
}

static int gnordofs_chown(const char *path, uid_t uid, gid_t gid)
{
  inode_t *inode;
  char *p;
  int res = 0;

  DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> gnordofs_chown(path = %s, uid = %d, gid = %d)\n", path, uid, gid);

  p = strdup(path);
  inode = namei(dev, sb, p);
  free(p);
  if (!inode)
    {
      res = -ENOENT;
    }
  else if (can_write_p(inode))
    {
      inode->owner = uid;
      inode->group = gid;
      inode->ctime = time(NULL);
      iput(dev, sb, inode);
      free(inode);
    }
  else
    {
      res = -EACCES;
      free(inode);
    }

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
  free(p);
  if (!inode)
    {
      res = -ENOENT;
    }
  else
    {
      if (inode->type == I_DIR || inode->type == I_FILE)
        {
          stbuf->st_nlink = inode->link_counter;
          stbuf->st_size = inode->size;

          stbuf->st_uid = inode->owner;
          stbuf->st_gid = inode->group;

          stbuf->st_atime = inode->atime;
          stbuf->st_ctime = inode->ctime;
          stbuf->st_mtime = inode->mtime;

          stbuf->st_mode = inode->perms;
        }
      else
        {
          res = -1;
        }
    }

  return res;
}

static int gnordofs_mkdir(const char *path, mode_t mode)
{
  inode_t *inode, *iparent;
  int i;
  char *dirc, *basec, *dname, *bname;
  struct fuse_context * ctxt = fuse_get_context();

  DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> gnordofs_mkdir(path = %s, mode = %o)\n", path, mode);

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
  free(dirc);
  if (!iparent)
    {
      free(basec);
      return -1;
    }

  if (add_dir_entry(dev, sb, iparent, inode, bname)  != 0)
    {
      free(basec);
      return -1;
    }

  inode->type = I_DIR;
  inode->size = 0;
  inode->perms = S_IFDIR | mode;
  inode->owner = ctxt->uid;
  inode->group = ctxt->gid;
  /* Entrada . */
  if (add_dir_entry(dev, sb, inode, inode, ".")  != 0)
    {
      free(basec);
      return -1;
    }
  /* Entrada . */
  if (add_dir_entry(dev, sb, inode, iparent, "..")  != 0)
    {
      free(basec);
      return -1;
    }
  inode->atime = inode->ctime = inode->mtime = time(NULL);

  iput(dev, sb, iparent);
  iput(dev, sb, inode);
  DEBUG_VERBOSE("mkdir -> (%d) %s\n", inode->n, bname);
  
  superblock_write(dev, sb);

  free(basec);

  return 0;
}

static int gnordofs_mknod(const char *path,
                          mode_t mode,
                          dev_t inputdev __attribute__((unused)))
{
  inode_t *inode, *iparent;
  int i;
  char *dirc, *basec, *dname, *bname;
  struct fuse_context * ctxt = fuse_get_context();

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
  free(dirc);
  if (!iparent)
    {
      free(basec);
      return -1;
    }

  if (add_dir_entry(dev, sb, iparent, inode, bname)  != 0)
    {
      free(basec);
      return -1;
    }

  inode->type = I_FILE;
  inode->size = 0;
  inode->perms = mode;
  inode->owner = ctxt->uid;
  inode->group = ctxt->gid;
  inode->atime = inode->ctime = inode->mtime = time(NULL);
  
  iput(dev, sb, inode);
  DEBUG_VERBOSE("mknod -> (%d) %s\n", inode->n, bname);
  
  superblock_write(dev, sb);

  free(basec);

  return 0;
}

static int gnordofs_open(const char *path, struct fuse_file_info *fi)
{
  DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> gnordofs_open(path = %s)\n", path);
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
  free(p);
  if (!inode)
    return -1;

  if (!can_read_p(inode))
    {
      free(inode);
      return -EACCES;
    }

  if (offset > inode->size)
    {
      free(inode);
      return 0;
    }

  if (offset + size > inode->size)
    size = inode->size - offset;

  do_lseek(dev, sb, inode, offset, SEEK_SET);
  count = do_read(dev, sb, inode, buf, size);

  inode->atime = time(NULL);
  iput(dev, sb, inode);

  free(inode);

  return count;
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

  p = strdup(path);
  inode = namei(dev, sb, p);
  free(p);
  if (!inode)
    {
      return -1;
    }

  if (!can_read_p(inode))
    {
      free(inode);
      return -EACCES;
    }

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

  inode->atime = time(NULL);
  iput(dev, sb, inode);

  free(de);
  free(inode);

  return 0;
}

static int gnordofs_release(const char *path, struct fuse_file_info *fi)
{
  DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> gnordofs_release(path = %s)\n", path);
  return 0;
}

static int gnordofs_rmdir(const char *path)
{
  char *dirc, *basec, *bname, *dname;
  inode_t *idir, *inode;
  dir_entry_t *de;

  DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> gnordofs_rmdir(path = %s)\n", path);

  dirc = strdup(path);
  basec = strdup(path);

  dname = dirname(dirc);
  bname = basename(basec);

  idir = namei(dev, sb, dname);
  free(dirc);
  if (!idir)
    {
      free(basec);
      return -1;
    }

  de = get_dir_entry_by_name(dev, sb, idir, bname);
  if (!de)
    {
      free(idir);
      free(basec);

      return -1;
    }

  inode = iget(dev, sb, de->inode);
  if (!inode)
    {
      free(de);
      free(idir);
      free(dirc);
      free(basec);

      return -1;
    }

  if (!can_write_p(inode))
    {
      free(inode);
      free(de);
      free(idir);
      free(basec);

      return -EACCES;
    }

  /* ¡Si el directorio no está vacío, no se puede borrar! */
  if (inode->size > 2*sizeof(struct dir_entry))
    {
      free(inode);
      free(de);
      free(idir);
      free(basec);

      return -ENOTEMPTY;
    }

  if (del_dir_entry_by_name(dev, sb, idir, bname) < 0)
    {
      free(inode);
      free(de);
      free(idir);
      free(basec);

      return -1;
    }

  idir->link_counter--;
  iput(dev, sb, idir);

  free(idir);
  free(basec);


  inode->link_counter--;
  iput(dev, sb, inode);

  if (inode->link_counter == 0)
    {
      ifree(dev, sb, inode);
    }

  superblock_write(dev, sb);

  free(inode);
  free(de);

  return 0;
}

static int gnordofs_truncate(const char *path, off_t size)
{
  char *p;
  inode_t *inode;

  DEBUG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> gnordofs_truncate(path = %s, size = %d)\n", path, size);

  p = strdup(path);
  inode = namei(dev, sb, p);
  free(p);
  if (!inode)
    {
      return -ENOENT;
    }

  if (!can_write_p(inode))
    {
      free(inode);
      return -EACCES;
    }

  if (inode_truncate(dev, sb, inode, size) < 0)
    {
      free(inode);
      return -1;
    }

  inode->mtime = time(NULL);
  iput(dev, sb, inode);
  superblock_write(dev, sb);

  return 0;
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
  free(dirc);
  if (!idir)
    {
      free(basec);
      return -1;
    }

  de = get_dir_entry_by_name(dev, sb, idir, bname);
  if (!de)
    {
      free(idir);
      free(basec);

      return -1;
    }

  inode = iget(dev, sb, de->inode);
  if (!inode)
    {
      free(de);
      free(idir);
      free(basec);

      return -1;
    }

  if (!can_write_p(inode))
    {
      free(inode);
      free(de);
      free(idir);
      free(basec);

      return -EACCES;
    }


  if (del_dir_entry_by_name(dev, sb, idir, bname) < 0)
    {
      free(inode);
      free(de);
      free(idir);
      free(basec);

      return -1;
    }

  iput(dev, sb, idir);

  free(idir);
  free(basec);

  inode->link_counter--;
  iput(dev, sb, inode);

  if (inode->link_counter == 0)
    {
      ifree(dev, sb, inode);
    }

  superblock_write(dev, sb);

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
  free(p);
  if (!inode)
    {
      return -1;
    }

  if (!can_write_p(inode))
    {
      free(inode);
      return -EACCES;
    }

  do_lseek(dev, sb, inode, offset, SEEK_SET);
  count = do_write(dev, sb, inode, buf, size);
  if (count < 0)
    {
      free(inode);
      return -ENOSPC;
    }

  /* Actualizar campo de tamaño si es necesario. */
  if (offset + count > inode->size)
    inode->size = offset + count;

  iput(dev, sb, inode);
  superblock_write(dev, sb);

  free(inode);

  return count;
}




static struct fuse_operations oper = {
  .access       = gnordofs_access,
  .chmod        = gnordofs_chmod,
  .chown        = gnordofs_chown,
  .getattr	= gnordofs_getattr,
  .mkdir        = gnordofs_mkdir,
  .mknod        = gnordofs_mknod,
  .open		= gnordofs_open,
  .read		= gnordofs_read,
  .readdir	= gnordofs_readdir,
  .release      = gnordofs_release,
  .rmdir        = gnordofs_rmdir,
  .truncate     = gnordofs_truncate,
  .unlink       = gnordofs_unlink,
  .write        = gnordofs_write
};




int main(int argc, char *argv[])
{
  openlog("GNORDOFS", LOG_PID, LOG_LOCAL0);
  DEBUG("#########################################################################\n");
  DEBUG("########################## GNORDOFS v0.0.1 beta #########################\n");
  DEBUG("#########################################################################\n");

  dev = open("./gnordofs.img", O_RDWR);
  sb = superblock_read(dev);

  return fuse_main(argc, argv, &oper, NULL);
}
