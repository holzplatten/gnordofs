#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <inode.h>
#include <superblock.h>

int main(int argc, char **argv)
{
  int dev;
  unsigned long size;
  superblock_t *sb;
  // Size = 10Mib
  size = 1024*1024*10;

  dev = open("bogniga.img", O_RDWR | O_CREAT, 0666);
  if (dev < 0)
    {
      perror(NULL);
      exit(1);
    }

  sb = superblock_init(size);

  superblock_print_dump(sb);

  superblock_write(dev, sb);
  free_block_list_init(dev, sb);
  //  inode_list_write(dev, inode_list);
  free(sb);

  sb = superblock_read(dev);
  if (!sb)
    {
      printf("WTF!\n");
      exit(1);
    }
  //superblock_print_dump(sb);
  print_free_block_list(dev, sb);

  free(sb);
  close(dev);

  return 0;
}
