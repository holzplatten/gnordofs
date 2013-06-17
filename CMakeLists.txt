cmake_minimum_required(VERSION 2.8)
project(gnordofs)

include_directories(include)
add_definitions(-g -ggdb -D_FILE_OFFSET_BITS=64)
link_libraries(fuse)

add_executable(mkfs.gnordofs mkfs.gnordofs.c block.c dir.c fs.c inode.c misc.c superblock.c)
add_executable(gnordofs gnordofs.c block.c dir.c fs.c inode.c misc.c perms.c superblock.c)

#install(TARGETS gnordofs RUNTIME DESTINATION bin))
